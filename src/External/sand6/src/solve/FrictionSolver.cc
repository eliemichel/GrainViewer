/*
 * This file is part of Sand6, a C++ continuum-based granular simulator.
 *
 * Copyright 2016 Gilles Daviet <gilles.daviet@inria.fr> (Inria - Université Grenoble Alpes)
 *
 * Sand6 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Sand6 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Sand6.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FrictionSolver.hh"
#include "PrimalData.hh"

#include "utils/Log.hh"

#include <bogus/Core/Block.impl.hpp>
#include <bogus/Core/BlockSolvers.impl.hpp>
#include <bogus/Extra/SecondOrder.impl.hpp>

#include <bogus/Core/Utils/Timer.hpp>
#include <bogus/Interfaces/Cadoux.hpp>

namespace d6 {

//! Either log directly residual, or re-evaluate it using another complemntarity func
/** (for objective benchmarking purposes) */
template <typename MatrixT >
struct CallbackProxy {

	CallbackProxy( FrictionSolver::Stats& stats, bogus::Timer &timer,
	               const MatrixT& W, const DynVec& mu, const DynVec &b, const DynVec &x )
	    : m_stats( stats ), m_timer( timer ),
	      m_W(W), m_mu(mu), m_b(b), m_x(x)
	{
	}

	void ackResidual( unsigned iter, Scalar err )
	{
		if( m_stats.shouldLogAC ) err = evalAC() ;
		m_stats.log( iter, err, m_timer.elapsed() );
	}

	Scalar evalAC() const {
		const DynVec y = m_W*m_x + m_b ;
		const Index n = m_W.rowsOfBlocks() ;

		Scalar err = 0 ;
#pragma omp parallel for reduction( + : err )
		for( Index i = 0 ; i < n ; ++i ) {
			const VecS lx = m_x.segment<SD>(SD*i) ;
			VecS  ac = lx -   y.segment<SD>(SD*i) ;
			// Normal part
			ac[0] = std::max(0., ac[0])  ;
			//Tangential part
			const Scalar nT = ac.segment<SD-1>(1).norm() ;
			if( nT > lx[0]*m_mu[i] ) {
				ac.segment<SD-1>(1) *= lx[0]*m_mu[i]/nT ;
			}
			//Error
			ac -= lx ;
			const Scalar lerr = ac.squaredNorm() ;
			err += lerr ;
		}
		return err / (1+n) ;
	}

private:
	FrictionSolver::Stats& m_stats ;
	bogus::Timer& m_timer ;
	bool m_evalAC ;

	const MatrixT & m_W ;
	const DynVec  & m_mu ;
	const DynVec  & m_b ;
	const DynVec  & m_x ;

};

void FrictionSolver::Stats::log( unsigned iter, Scalar res, Scalar time )
{
	m_log.emplace_back(FrictionSolver::Stats::Entry{res,time,iter})  ;
	d6::Log::Debug() << "Primal " << iter << " =\t " << res << std::endl ;
	if( time > timeOut ) throw TimeOutException() ;
}



FrictionSolver::Options::Options()
    : algorithm( GaussSeidel ),
      maxIterations(250), maxOuterIterations( 15 ),
      projectedGradientVariant( -1  ),
      useInfinityNorm( false ), tolerance( 1.e-6 )
{}


FrictionSolver::FrictionSolver(const PrimalData &data)
    : m_data( data )
{}

Scalar FrictionSolver::solve( const Options& options, DynVec &lambda, Stats &stats) const
{
//	m_data.dump("last.d6") ;

	if( m_data.mass_matrix_mode != PrimalData::Lumped ) {
		Log::Error() << " FrictionSolver::solve implemented only for lumped mass matrix " << std::endl ;
		return -1 ;
	}


	// Build W expression
	typedef bogus::Product< PrimalData::JacobianType,
	        bogus::Product< PrimalData::InvInertiaType, bogus::Transpose< PrimalData::JacobianType > > >
	        JMJtProd ;

	bogus::NarySum< JMJtProd > rbSum( m_data.n() * SD, m_data.n() * SD ) ;
	// Add rigid bodies jacobians
	for( unsigned i = 0 ; i < m_data.jacobians.size() ; ++i ) {
		const PrimalData::JacobianType &JM = m_data.jacobians[i] ;
		rbSum +=  JM * ( m_data.inv_inertia_matrices[i] * JM.transpose() ) ;
	}

	typedef bogus::Product< PrimalData::HType, bogus::Transpose< PrimalData::HType > > HHtProd ;
	typedef bogus::Addition< HHtProd, bogus::NarySum< JMJtProd > > Wexpr ;

	const Wexpr W = m_data.H * m_data.H.transpose() + rbSum ;


	bogus::Timer timer ;
	CallbackProxy< Wexpr > callbackProxy( stats, timer, W, m_data.mu, m_data.w, lambda ) ;

	Scalar res = -1 ;

	if(  options.algorithm == Options::GaussSeidel_NoAssembly ) {

		// Put all rigid body jacobian inside a single matrix
		typedef bogus::SparseBlockMatrix< MatS, bogus::UNCOMPRESSED > JType ;
		JType J ;
		J.setRows( m_data.n() ) ;
		J.setCols( m_data.jacobians.size() );

		Index Jnnz = 0 ; // Compute non-zeros so we can avoid resizing, and insert blocks in parallel
		for( unsigned i = 0 ; i < m_data.jacobians.size() ; ++i ) {
			Jnnz += m_data.jacobians[i].nBlocks() ;
		}
		J.reserve( Jnnz );

		for( unsigned i = 0 ; i < m_data.jacobians.size() ; ++i ) {
			const PrimalData::JacobianType &JM = m_data.jacobians[i] ;
			Eigen::SelfAdjointEigenSolver<MatS>
			        es( m_data.inv_inertia_matrices[i].block(0) ) ;
			const MatS MiSqrt
			        = es.eigenvectors().transpose() * es.eigenvalues().cwiseSqrt().asDiagonal()
			        * es.eigenvectors() ;

#pragma omp parallel for
			for (Index row = 0 ; row < m_data.n() ; ++row ) {
				typename PrimalData::JacobianType::InnerIterator it( JM.majorIndex(), row ) ;
				if( it ) J.insert( row, i ) = JM.block(it.ptr()) * MiSqrt ;
			}

		}
		J.finalize();

		typedef bogus::CompoundBlockMatrix< true, PrimalData::HType, JType > HType ;
		const HType H ( m_data.H, J ) ;

		// Direct Gauss-Seidel solver
		bogus::SOCLaw< SD, Scalar, true > law( m_data.n(), m_data.mu.data() ) ;

		bogus::ProductGaussSeidel< HType > gs( H ) ;
		gs.setTol( options.tolerance );
		gs.setMaxIters( options.maxIterations );
		gs.useInfinityNorm( options.useInfinityNorm );
		gs.callback().connect( callbackProxy, &CallbackProxy<Wexpr>::ackResidual );

		res = gs.solve( law, m_data.w, lambda, false ) ;

	} else
	if( options.algorithm == Options::PG_NoAssembly ||
	        options.algorithm == Options::Cadoux_PG_NoAssembly  ) {

		bogus::ProjectedGradient< Wexpr > pg( W ) ;

		if( options.projectedGradientVariant < 0 ) {
			pg.setDefaultVariant( bogus::projected_gradient::SPG );
		} else {
			pg.setDefaultVariant( (bogus::projected_gradient::Variant) options.projectedGradientVariant );
		}

		pg.setTol( options.tolerance );
		pg.setMaxIters( options.maxIterations );
		pg.useInfinityNorm( options.useInfinityNorm );

		if( options.algorithm == Options::PG_NoAssembly ) {

			bogus::SOCLaw< SD, Scalar, true > law( m_data.n(), m_data.mu.data() ) ;
			pg.callback().connect( callbackProxy, &CallbackProxy<Wexpr>::ackResidual );
			res = pg.solve( law, m_data.w, lambda ) ;

		} else {

			bogus::Signal<unsigned, Scalar> callback ;
			callback.connect( callbackProxy, &CallbackProxy<Wexpr>::ackResidual );
			res = bogus::solveCadoux<SD>( W, m_data.w.data(), m_data.mu.data(), pg,
			                              lambda.data(), options.maxOuterIterations, &callback ) ;
		}

	} else {

		// Explicit assembly of W

		typedef typename FormMat<SD,SD>::SymType WType ;
		//typedef bogus::SparseBlockMatrix< Eigen::Matrix< Scalar, 6, 6, Eigen::RowMajor >,
	//										bogus::SYMMETRIC > WType ;
		WType Wmat = W ;

		if( options.algorithm == Options::GaussSeidel ) {
			// Direct Gauss-Seidel solver

			bogus::SOCLaw< SD, Scalar, true > law( m_data.n(), m_data.mu.data() ) ;

			bogus::GaussSeidel< WType > gs( Wmat ) ;
			gs.setTol( options.tolerance );
			gs.setMaxIters( options.maxIterations );
			gs.useInfinityNorm( options.useInfinityNorm );
			gs.callback().connect( callbackProxy, &CallbackProxy<Wexpr>::ackResidual );

			res = gs.solve( law, m_data.w, lambda, false ) ;

		} else  {

			// Cadoux fixed-point algorithm

			bogus::Signal<unsigned, Scalar> callback ;
			callback.connect( callbackProxy, &CallbackProxy<Wexpr>::ackResidual );

			if( options.algorithm == Options::Cadoux_GS ) {

				// Gauss-Seidel inner solver

				bogus::GaussSeidel< WType > gs( Wmat ) ;
				gs.setTol( options.tolerance );
				gs.setMaxIters( options.maxIterations );
				gs.useInfinityNorm( options.useInfinityNorm );

				res = bogus::solveCadoux<SD>( Wmat, m_data.w.data(), m_data.mu.data(), gs,
				                              lambda.data(), options.maxOuterIterations, &callback ) ;
			} else {

				// Projected Gradient inner solver

				bogus::ProjectedGradient< WType > pg( Wmat ) ;

				if( options.projectedGradientVariant < 0 ) {
					pg.setDefaultVariant( bogus::projected_gradient::SPG );
				} else {
					pg.setDefaultVariant( (bogus::projected_gradient::Variant) options.projectedGradientVariant );
				}

				pg.setTol( options.tolerance );
				pg.setMaxIters( options.maxIterations );
				pg.useInfinityNorm( options.useInfinityNorm );

				res = bogus::solveCadoux<SD>( Wmat, m_data.w.data(), m_data.mu.data(), pg,
				                              lambda.data(), options.maxOuterIterations, &callback ) ;
			}
		}
	}

	return res ;
}

}
