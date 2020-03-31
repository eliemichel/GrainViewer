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

#include "PhaseStepData.hh"

#include "Phase.hh"
#include "simu/DynParticles.hh"
#include "simu/RigidBody.hh"
#include "RigidBodyData.hh"

#include "simu/FormBuilder.impl.hh"

#include "geo/MeshImpl.hh"

#include "utils/Config.hh"
#include "utils/Log.hh"

#include <bogus/Core/Block.impl.hpp>
#include <bogus/Core/Utils/Timer.hpp>

#include <utility>

//#define FULL_FEM        // Ignore particles, just solve FEM system
//#define CONSTANT_VISC   // Assumes eta(phi) = 1./Re instead of eta(phi) = phi/Re
//#define INTEGRATE_PARTICLES_SEQUENTIAL

namespace d6 {


void PhaseStepData::computeProjectors(const Config&config,
									  const PrimalShape& pShape, const DualShape &dShape,
									  const std::vector<RigidBodyData> &rbData, const PrimalScalarField &lumped_mass,
									  Projectors& mats) const
{
	const Scalar discard_empty = 0. ;
#ifdef D6_UNSTRUCTURED_DUAL
	(void) dShape ;
#endif

	const Index m  = nPrimalNodes() ;
	const Index n  = nDualNodes() ;
	const Index nc = nSuppNodes() ;

	mats.vel.setRows( m );
	mats.vel.setIdentity() ;

	mats.stress.setRows( n+nc );
	mats.stress.setIdentity() ;

	typedef typename PrimalShape::MeshType MeshType ;

	StrBoundaryMapper bdMapper ( config.boundary ) ;

	for( const typename MeshType::Cell& cell : primalNodes.cells )
	{
		if( ! pShape.mesh().onBoundary(cell) ) continue ;

		typename PrimalShape::Location ploc ;
		typename PrimalShape::NodeList pnodes ;
		ploc.cell = cell ;
		pShape.list_nodes( ploc, pnodes ) ;

		for( unsigned k = 0 ; k < PrimalShape::NI ; ++k ) {
			const Index i = primalNodes.indices[ pnodes[k] ] ;
			if( lumped_mass[pnodes[k]] <= discard_empty ) {
				mats.vel.block( i ).setZero() ;
				continue ;
			}

			BoundaryInfo info ;
			pShape.locate_dof( ploc, k ) ;
			pShape.mesh().boundaryInfo( ploc, bdMapper, info ) ;
			info.velProj( mats.vel.block( i ) ) ;
		}

		if( config.weakStressBC )  continue ;

#ifndef D6_UNSTRUCTURED_DUAL
		typename DualShape::Location dloc ;
		typename DualShape::NodeList dnodes ;
		dShape.locate( pShape.qpIterator(&cell).pos(), dloc ) ;
		dShape.list_nodes( dloc, dnodes ) ;

		for( unsigned k = 0 ; k < DualShape::NI ; ++k ) {
			const Index i = dualNodes.indices[ dnodes[k] ] ;
			BoundaryInfo info ;
			dShape.locate_dof( dloc, k ) ;
			dShape.mesh().boundaryInfo( dloc, bdMapper, info ) ;
			info.stressProj( mats.stress.block( i ) ) ;
		}
#endif

	}

	// Additional nodes for frictional boundaries
	for( const RigidBodyData& rb: rbData ) {
		for( const typename MeshType::Cell& cell : rb.nodes.cells )	{
			if( ! pShape.mesh().onBoundary(cell) ) continue ;

			typename PrimalShape::Location ploc ;
			typename PrimalShape::NodeList pnodes ;
			ploc.cell = cell ;
			pShape.list_nodes( ploc, pnodes ) ;

			// Ignore RB-boundary constraints on Dirichlet boundaries
			for( unsigned k = 0 ; k < PrimalShape::NI ; ++k ) {
				pShape.locate_dof( ploc, k ) ;
				BoundaryInfo info ;
				pShape.mesh().boundaryInfo( ploc, bdMapper, info ) ;
				if( info.bc == BoundaryInfo::Stick ) {
					const Index i = rb.nodes.indices[ pnodes[k] ] ;
					mats.stress.block( i ).setZero() ;
				}
			}
		}
	}

}

void PhaseStepData::assembleMatrices(const Particles &particles,
		const Config &config, const Scalar dt, const DualShape &dShape,
									 const PrimalScalarField &phiInt,
		std::vector< RigidBodyData >&rbData
		)
{
	const PrimalShape& pShape = phiInt.shape() ;


	typedef typename PrimalShape::Interpolation P_Itp ;
	typedef typename PrimalShape::Derivatives   P_Dcdx ;
	typedef typename DualShape::Interpolation   D_Itp ;
	typedef typename DualShape::Derivatives     D_Dcdx ;

	bogus::Timer timer;

	const Index m  = nPrimalNodes() ;
	const Index n  = nDualNodes() ;
	const Index nc = nSuppNodes() ;

	const Scalar mass_regul = 1.e-8 ;

	computeProjectors( config, pShape, dShape, rbData, phiInt, proj ) ;

	// Lumped mass matrix
	{
		forms.M_lumped.setRows( m );
		forms.M_lumped.setIdentity() ;
		forms.M_lumped_inv.setRows( m );
		forms.M_lumped_inv.setIdentity() ;
		forms.M_lumped_inv_sqrt.setRows( m );
		forms.M_lumped_inv_sqrt.setIdentity() ;

	#pragma omp parallel for
		for( Index i = 0 ; i < m ; ++i ) {
			forms.M_lumped.block( i ) *= phiInt[ primalNodes.revIndices[i] ] ;
		}
	}

	{
		timer.reset() ;
		//A

		typedef FormBuilder< PrimalShape, PrimalShape > Builder ;
		Builder builder( pShape, pShape ) ;
		builder.reset( m );
		builder.addToIndex( primalNodes.cells.begin(), primalNodes.cells.end(), primalNodes.indices, primalNodes.indices );
		builder.makeCompressed();

		// A
		forms.A.clear();
		forms.A.setRows( m );
		forms.A.setCols( m );
		forms.A.cloneIndex( builder.index() ) ;
		forms.A.setBlocksToZero() ;

		Log::Debug() << "A Index computation: " << timer.elapsed() << std::endl ;

#if defined(FULL_FEM) || defined(CONSTANT_VISC)
		builder.integrate_cell<form::Left>( primalNodes.cells.begin(), primalNodes.cells.end(),
								[&]( Scalar w, const Vec&, const P_Itp& l_itp, const P_Dcdx& l_dc_dx, const P_Itp& r_itp, const P_Dcdx& r_dc_dx )
			{
				Builder:: addDuDv( forms.A, w, l_itp, l_dc_dx, r_itp, r_dc_dx, primalNodes.indices, primalNodes.indices ) ;
			}
		);
		Log::Debug() << "A Integrate grid: " << timer.elapsed() << std::endl ;
#else
		//TODO parallel integration
		builder.integrate_particle( particles, [&]( Index, Scalar w, const P_Itp& l_itp, const P_Dcdx& l_dc_dx, const P_Itp& r_itp, const P_Dcdx& r_dc_dx )
			{
				Builder:: addDuDv( forms.A, w, l_itp, l_dc_dx, r_itp, r_dc_dx, primalNodes.indices, primalNodes.indices ) ;
			}
		);
		Log::Debug() << "A Integrate particle: " << timer.elapsed() << std::endl ;
#endif

		timer.reset() ;

	}

	// Other bilinear forms
	{
		timer.reset() ;


		typedef FormBuilder< DualShape, PrimalShape > Builder ;

		Builder builder( dShape, pShape ) ;
		builder.reset( n );
		builder.addToIndex( dualNodes.indices, primalNodes.indices );
		builder.makeCompressed();

		Log::Debug() << "Index computation: " << timer.elapsed() << std::endl ;

		// J
		forms.J.clear();
		forms.J.setRows( builder.rows() );
		forms.J.setCols( m );
		forms.J.cloneIndex( builder.index() ) ;
		forms.J.setBlocksToZero() ;

		// C
		if( config.enforceMaxFrac ) {
			// TODO enforceMaxFrac cannot work w/ discontinuous stress approx
			forms.C.clear();
			forms.C.setRows( builder.rows() );
			forms.C.setCols( m );
			forms.C.cloneIndex( builder.index() ) ;
			forms.C.setBlocksToZero() ;
		}

		builder.addRows(nc) ;

		// B
		forms.B.clear();
		forms.B.setRows( builder.rows() );
		forms.B.setCols( m );
		forms.B.cloneIndex( builder.index() ) ;
		forms.B.setBlocksToZero() ;

		timer.reset() ;

#if !( defined(INTEGRATE_PARTICLES_SEQUENTIAL) || defined(UNSTRUCTURED_DUAL) )
		{
			// Integrating over particles can be slow and not directly parallelizable
			// To regain some parallelism, we first associate particles to nodes,
			// then compute separately each row of he form matrices

			const size_t np = particles.count() ;

			Eigen::Matrix< Scalar,   DualShape::NI, Eigen::Dynamic > coeffs ;
			Eigen::Matrix<  Index,   DualShape::NI, Eigen::Dynamic > nodeIds  ;

			coeffs .resize( DualShape::NI, np) ;
			nodeIds.resize( DualShape::NI, np) ;

			std::vector< std::vector< std::pair< size_t, Index > > > nodeParticles ( n ) ;

			typedef typename PrimalShape::Location P_Loc ;
			typedef typename   DualShape::Location D_Loc ;


#pragma omp parallel
			for ( size_t i = 0 ; i < np ; ++i )
			{
				D_Itp itp ; D_Loc loc ;

				dShape.locate_by_pos_or_id( particles.centers().col(i), i, loc );
				dShape.interpolate( loc, itp );

				nodeIds.col(i) = itp.nodes ;
				coeffs .col(i) = itp.coeffs ;
			}

			for ( size_t i = 0 ; i < np ; ++i ) {
				for( Index k = 0 ; k <   DualShape::NI ; ++k ) {
					nodeParticles[ dualNodes.indices[nodeIds(k,i)] ].push_back( std::make_pair(i,k) ) ;
				}
			}


#pragma omp parallel for
			for( Index nidx = 0 ; nidx < n ; ++ nidx )
			{
				P_Itp r_itp ; P_Dcdx r_dcdx ; P_Loc r_loc ;
				D_Itp l_itp ; D_Dcdx l_dcdx ; D_Loc l_loc ;

				for( unsigned i = 0 ; i < nodeParticles[nidx].size() ; ++i ) {
					const size_t pid = nodeParticles[nidx][i].first ;
					const Index k0   = nodeParticles[nidx][i].second ;

					const auto& pos = particles.centers().col(pid) ;
					dShape.locate_by_pos_or_id( pos, pid, l_loc );
					dShape.get_derivatives( l_loc, l_dcdx );
					l_itp.nodes  = nodeIds.col(pid) ;
					l_itp.coeffs = coeffs.col(pid) ;

					pShape.locate_by_pos_or_id( pos, pid, r_loc );
					pShape.interpolate( r_loc, r_itp );
					pShape.get_derivatives( r_loc, r_dcdx );

					const Scalar w = particles.volumes()[pid] ;
					const Scalar m = l_itp.coeffs[k0] * w ;
					const D_Dcdx& const_dcdx = l_dcdx ;

					Builder::addTauDu( forms.B, m, nidx, r_itp, r_dcdx, primalNodes.indices ) ;
					Builder::addTauWu( forms.J, m, nidx, r_itp, r_dcdx, primalNodes.indices ) ;

					if( config.enforceMaxFrac ) {
						Builder::addDpV( forms.C, w, nidx, const_dcdx.row(k0), r_itp, primalNodes.indices ) ;
					}

				}
			}


		}
#else
		builder.integrate_particle( particles, [&]( Index, Scalar w, const D_Itp& l_itp, const D_Dcdx& l_dc_dx, const P_Itp& r_itp, const P_Dcdx& r_dc_dx )
			{
				Builder::addTauDu( forms.B, w, l_itp, r_itp, r_dc_dx, dualNodes.indices, primalNodes.indices ) ;
				Builder::addTauWu( forms.J, w, l_itp, r_itp, r_dc_dx, dualNodes.indices, primalNodes.indices ) ;
				if( config.enforceMaxFrac ) {
					Builder::addDpV  ( forms.C, w, l_itp, l_dc_dx, r_itp, dualNodes.indices, primalNodes.indices ) ;
				}
			}
		);
#endif
		Log::Debug() << "Integrate particle: " << timer.elapsed() << std::endl ;


		// S
		forms.S.compute( dShape, dualNodes, n+nc );

	}

	// Rigid bodies
	timer.reset() ;
#pragma omp parallel for if( rbData.size() > 1)
	for( int k = 0 ; k < rbData.size() ; ++k )
	{
		rbData[k].assemble_matrices( pShape, dShape, primalNodes, dualNodes, n+nc ) ;
	}
	Log::Debug() << "Integrate rbs: " << timer.elapsed() << std::endl ;


	// A = mass + viscosity
	forms.A *= 2 * config.viscosity ;
#ifndef FULL_FEM
	forms.M_lumped *= config.volMass / dt  ;
	forms.A += forms.M_lumped ;
#else
	(void) dt ;
#endif


	// Projections
	const typename FormMat<WD,WD>::SymType IP = proj.vel.Identity() - proj.vel ;
	forms.A = proj.vel * ( forms.A * proj.vel ) + IP ;

#pragma omp parallel for
	for( Index i = 0 ; i < m ; ++i ) {
		const Scalar mass = forms.M_lumped.block(i).trace() / WD ;
		forms.M_lumped         .block(i) = proj.vel.block(i) * mass
				+ Mat::Identity() - proj.vel.block(i) ;
		forms.M_lumped_inv     .block(i) = proj.vel.block(i) * 1./(mass + mass_regul )
				+ Mat::Identity() - proj.vel.block(i) ;
		forms.M_lumped_inv_sqrt.block(i) = proj.vel.block(i) * 1./std::sqrt( mass + mass_regul ) ;
	}


}

void PhaseStepData::computeAnisotropy(const DynVec &orientation, const Config& config,
									 typename FormMat<SD,SD>::SymType &Aniso ) const
{
	// Compute anisotropy matrix from interpolated orientation distributions

	Aniso = proj.stress.Identity() ;

	if( config.anisotropy <= 0 )
		return ;

	const Index n  = nDualNodes() ;

#pragma omp parallel for
	for( Index i = 0 ; i < n ; ++i ) {

		Mat ori ;
		tensor_view( Segmenter<SD>::segment( orientation, i ) ).get( ori ) ;

		ori = (1. - config.anisotropy) * Mat::Identity() + WD * config.anisotropy * ori ;

		compute_anisotropy_matrix( ori, Aniso.block(i) );

	}

}

void PhaseStepData::compute(const DynParticles& particles,
		const Config &config, const Scalar dt, Phase &phase,
		std::vector< RigidBody   >& rigidBodies,
		std::vector< RBStresses > &rbStresses , std::vector<RigidBodyData> & rbData)
{
	const PrimalShape& pShape = phase.velocity.shape() ;
	const   DualShape& dShape = phase.stresses.shape() ;

	// Transfer particles quantities to grid
	PrimalScalarField intPhiPrimal  ( pShape ) ;
	PrimalVectorField intPhiVel     ( pShape ) ;

	DualScalarField intPhiDual    ( dShape ) ;
	DualScalarField intPhiInertia ( dShape ) ;
	DualScalarField intPhiCohesion( dShape ) ;
	DualTensorField intPhiOrient  ( dShape ) ;

	std::vector< bool > activeCells ;

#if defined(FULL_FEM)
	pShape.compute_tpz_mass   ( intPhiPrimal.flatten() ) ;
	dShape.compute_lumped_mass( intPhiDual  .flatten() ) ;
	intPhiVel.set_zero() ;
	intPhiInertia.set_zero() ;
	intPhiCohesion.set_zero() ;
	intPhiOrient.set_zero() ;
	activeCells.assign( pShape.mesh().nCells(), true ) ;
#else
	particles.integratePrimal( activeCells, intPhiPrimal, intPhiVel ) ;
	particles.integrateDual( intPhiDual, intPhiInertia, intPhiOrient, intPhiCohesion ) ;
#endif

	// Compute phi and grad_phi (for visualization purposes )
	computePhiAndGradPhi( intPhiPrimal, phase.fraction, phase.grad_phi ) ;

	// Active nodes
	computeActiveNodes( activeCells, pShape, dShape ) ;
	Log::Verbose() << "Active nodes: " << nPrimalNodes() << " / " << pShape.nDOF() << std::endl;
	Log::Verbose() << "  Dual nodes: " <<   nDualNodes() << " / " << dShape.nDOF() << std::endl;

	//Rigid bodies and frictional boundaries
	computeActiveBodies( rigidBodies, rbStresses, rbData );
	Log::Debug() << "Tot coupling nodes: " << nSuppNodes() << std::endl ;

	// Bilinear forms matrices
	assembleMatrices( particles.geo(), config, dt, dShape, intPhiPrimal, rbData );

	primalNodes.field2var( intPhiVel , forms.phiu ) ;
	dualNodes  .field2var( intPhiDual, forms.fraction ) ;

	// Cohesion, inertia, orientation -- trapezoidal approx
	DynVec orientation ;
	intPhiCohesion.divide_by_positive( intPhiDual ) ;
	intPhiInertia .divide_by_positive( intPhiDual ) ;
	intPhiOrient  .divide_by_positive( intPhiDual ) ;

	dualNodes.field2var( intPhiCohesion, cohesion ) ;
	dualNodes.field2var( intPhiInertia , inertia  ) ;
	dualNodes.field2var( intPhiOrient  , orientation  ) ;

	computeAnisotropy( orientation, config, Aniso );

	// Volumes
	{
		DualScalarField volumes ( dShape ) ;
		dShape.compute_lumped_mass( volumes.flatten() );
		dualNodes.field2var( volumes, forms.volumes ) ;
	}

	// External forces
	PrimalVectorField gravity ( intPhiPrimal.shape() ) ;
	gravity.set_constant( config.gravity );
	gravity.multiply_by( intPhiPrimal ) ;
	gravity.flatten() *= config.volMass ;
	primalNodes.field2var( gravity, forms.externalForces ) ;
}


void PhaseStepData::computePhiAndGradPhi(const PrimalScalarField &intPhi, PrimalScalarField& fraction, PrimalVectorField &grad_phi ) const
{
	const PrimalShape& shape = intPhi.shape() ;

	// Compute volumes of cells
	PrimalScalarField volumes(shape) ;
	intPhi.shape().compute_tpz_mass( volumes.flatten() );

	fraction = intPhi;
	fraction.divide_by_positive( volumes ) ;

	grad_phi.set_zero() ;

	// FXIME other approxes
	typedef FormBuilder< PrimalShape, PrimalShape > Builder ;
	Builder builder( shape, shape ) ;

	typedef const typename PrimalShape::Interpolation& Itp ;
	typedef const typename PrimalShape::Derivatives& Dcdx ;

	builder.integrate_qp( [&]( Scalar w, const Vec&, Itp itp, Dcdx dc_dx, Itp , Dcdx )
	{
		for( Index j = 0 ; j < PrimalShape::NI ; ++j ) {
			for( Index k = 0 ; k < PrimalShape::NI ; ++k ) {
				grad_phi[ itp.nodes[j] ] += w * dc_dx.row(k) * itp.coeffs[k] * fraction[ itp.nodes[k] ] ;
			}
		}
	} ) ;

	grad_phi.divide_by( volumes ) ;
}

void PhaseStepData::computeActiveNodes(const std::vector<bool> &activeCells ,
									 const PrimalShape& pShape, const DualShape& dShape )
{
	typedef typename PrimalShape::MeshType MeshType ;

	const MeshType& mesh = pShape.derived().mesh() ;

	primalNodes.reset( pShape.nDOF() );
	  dualNodes.reset( dShape.nDOF() ) ;

	std::vector< int > activePrimalNodes( pShape.nDOF(), 0 ) ;
	std::vector< int >   activeDualNodes( dShape.nDOF(), 0 ) ;
#ifdef D6_UNSTRUCTURED_DUAL
	activeDualNodes.assign( dShape.nDOF(), 1 ) ;
#endif

	Eigen::Matrix< Scalar, WD, Eigen::Dynamic > vecs( WD, mesh.nNodes() ) ;
	vecs.setZero() ;


	for( typename MeshType::CellIterator it = mesh.cellBegin() ; it != mesh.cellEnd() ; ++it )
	{
		if (!activeCells[ it.index() ] ) continue ;
		primalNodes.cells.push_back( *it ) ;

		// Primal
		typename PrimalShape::NodeList pnodes ;
		pShape.list_nodes( *it, pnodes );

		for( int k = 0 ; k < PrimalShape::NI ; ++ k ) {
			++activePrimalNodes[ pnodes[k] ] ;
		}

		// Dual
#ifndef D6_UNSTRUCTURED_DUAL
		typename DualShape::Location dloc ;
		typename DualShape::NodeList dnodes ;
		dShape.locate( pShape.qpIterator( it ).pos(), dloc );
		dShape.list_nodes( dloc, dnodes );

		for( int k = 0 ; k < DualShape::NI ; ++ k ) {
			++activeDualNodes[ dnodes[k] ] ;
		}
#endif
	}
	for( size_t i = 0 ; i < activePrimalNodes.size() ; ++i ) {
		if( activePrimalNodes[i] > 0 ) {
			primalNodes.indices[i] = primalNodes.nNodes++ ;
		}
	}
	for( size_t i = 0 ; i < activeDualNodes.size() ; ++i ) {
		if( activeDualNodes[i] > 0 ) {
			dualNodes.indices[i] = dualNodes.nNodes++ ;
		}
	}

	primalNodes.computeRevIndices();
	  dualNodes.computeRevIndices();


}

void PhaseStepData::computeActiveBodies( std::vector<RigidBody> &rigidBodies,
										 std::vector<RBStresses> &rbStresses,
										 std::vector< RigidBodyData > &rbData
										 )
{
	rbData.clear();
	for( int i = 0 ; i < rigidBodies.size() ; ++i ) {
		rbData.emplace_back( rigidBodies[i], rbStresses[i] );
	}

#pragma omp parallel for
	for( int i = 0 ; i < rigidBodies.size() ; ++i ) {
		rbData[i].compute_active( primalNodes ) ;
		rbData[i].nodes.computeRevIndices() ;
	}

	m_totRbNodes = 0 ;
	for( unsigned i = 0 ; i < rigidBodies.size() ; ++i )
	{
		rbData[i].nodes.setOffset( m_totRbNodes + dualNodes.count() ) ;
		m_totRbNodes += rbData[i].nodes.count() ;
	}


}


} //d6
