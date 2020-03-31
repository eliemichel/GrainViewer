/*
 * This file is part of So-bogus, a C++ sparse block matrix library and
 * Second Order Cone solver.
 *
 * Copyright 2016 Gilles Daviet <gdaviet@gmail.com>
 *
 * So-bogus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * So-bogus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with So-bogus.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FrictionProblem.impl.hpp"

#include "../Core/BlockSolvers/ProductGaussSeidel.impl.hpp"
#include "../Core/BlockSolvers/ADMM.impl.hpp"


namespace bogus {


template< unsigned Dimension >
void PrimalFrictionProblem< Dimension >::computeMInv( )
{
	// M^-1
	MInv.cloneStructure( M ) ;
#ifndef BOGUS_DONT_PARALLELIZE
#pragma omp parallel for
#endif
	for( std::ptrdiff_t i = 0 ; i < (std::ptrdiff_t) M.nBlocks()  ; ++ i )
	{
		MInv.block(i).compute( M.block(i) ) ;
	}
}

template< unsigned Dimension >
double PrimalFrictionProblem< Dimension >::solveWith( ProductGaussSeidelType &pgs, double * r, const bool staticProblem ) const
{
	// b = E' w - H M^-1 f
	const Eigen::VectorXd b = E.transpose() * Eigen::VectorXd::Map( w, H.rows())
	        - H * ( MInv * Eigen::VectorXd::Map( f, H.cols() ) );

	Eigen::VectorXd::MapType r_map( r, H.rows() ) ;

	pgs.setMatrix( H ) ;
	pgs.setDiagonal( MInv ) ;

	if( staticProblem ) {
		bogus::SOCLaw< Dimension, double, false > law( H.rowsOfBlocks(), mu ) ;
		return pgs.solve( law, b, r_map ) ;
	} else {
		bogus::SOCLaw< Dimension, double, true  > law( H.rowsOfBlocks(), mu ) ;
		return pgs.solve( law, b, r_map ) ;
	}

}


template< unsigned Dimension >
double PrimalFrictionProblem< Dimension >::solveWith( ADMMType &admm, double lambda, double* v, double * r ) const
{
	const Eigen::VectorXd f = Eigen::VectorXd::Map( this->f, M.rows() ) ;
	const Eigen::VectorXd w = E.transpose() * Eigen::VectorXd::Map( this->w, H.rows() ) ;

	Eigen::VectorXd::MapType r_map( r, H.rows() ) ;
	Eigen::VectorXd::MapType v_map( v, H.cols() ) ;

	bogus::QuadraticProxOp< MInvType > prox( MInv, lambda, f ) ;

	Eigen::ArrayXd inv_mu = 1./Eigen::ArrayXd::Map( mu, H.rowsOfBlocks()  );
	bogus::SOCLaw< Dimension, double, false > law( inv_mu.rows(), inv_mu.data() ) ;

	admm.setMatrix( H ) ;
	return admm.solve( law, prox, w, v_map, r_map ) ;
}


template< unsigned Dimension >
double PrimalFrictionProblem< Dimension >::solveWith( DualAMAType &dama, double* v, double * r, const bool staticProblem ) const
{
	const Eigen::VectorXd f = Eigen::VectorXd::Map( this->f, M.rows() ) ;
	const Eigen::VectorXd w = E.transpose() * Eigen::VectorXd::Map( this->w, H.rows() ) ;

	Eigen::VectorXd::MapType r_map( r, H.rows() ) ;
	Eigen::VectorXd::MapType v_map( v, H.cols() ) ;

	Eigen::ArrayXd inv_mu = 1./Eigen::ArrayXd::Map( mu, H.rowsOfBlocks()  );
	bogus::SOCLaw< Dimension, double, false > law( inv_mu.rows(), inv_mu.data() ) ;

	dama.setMatrix( H ) ;

	if( staticProblem ) {
		bogus::SOCLaw< Dimension, double, false > law( H.rowsOfBlocks(), mu ) ;
		return dama.solve( law, M, f, w, v_map, r_map ) ;
	} else {
		bogus::SOCLaw< Dimension, double, true  > law( H.rowsOfBlocks(), mu ) ;
		return dama.solve( law, M, f, w, v_map, r_map ) ;
	}

}

#ifdef BOGUS_INSTANTIATE_2D_SOC
template struct PrimalFrictionProblem< 2u > ;
#endif

#ifdef BOGUS_INSTANTIATE_3D_SOC
template struct PrimalFrictionProblem< 3u > ;
#endif

#ifdef BOGUS_INSTANTIATE_DYNAMIC_SOC
template struct PrimalFrictionProblem< Eigen::Dynamic > ;
#endif

} //bogus
