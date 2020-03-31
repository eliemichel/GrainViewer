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

#include "PhaseSolver.hh"

#include "Phase.hh"
#include "PhaseStepData.hh"
#include "RigidBodyData.hh"

#include "simu/RigidBody.hh"

#include "simu/FormBuilder.hh"
#include "simu/LinearSolver.hh"

#include "solve/PrimalData.hh"
#include "solve/FrictionSolver.hh"
#include "solve/LCP.hh"

#include "utils/Log.hh"
#include "utils/Config.hh"
#include "utils/Stats.hh"

#include <bogus/Core/Block.impl.hpp>
#include <bogus/Core/Utils/Timer.hpp>

namespace d6 {


PhaseSolver::PhaseSolver(const DynParticles &particles)
    : m_particles(particles)
{

}

void PhaseSolver::step(const Config &config, const Scalar dt, Phase &phase, Stats& stats,
                       std::vector< RigidBody   >& rigidBodies,
                       std::vector<RBStresses > &rbStresses) const
{
	bogus::Timer timer ;

	PhaseStepData stepData ;
	std::vector< RigidBodyData > rbData ;

	// Read data from particles, assemble matrices, etc
	stepData.compute( m_particles, config, dt, phase, rigidBodies, rbStresses, rbData );

	stats.nPrimalNodes = stepData.nPrimalNodes() ;
	stats.nDualNodes = stepData.nPrimalNodes() ;
	stats.nCouplingNodes = stepData.nSuppNodes() ;
	stats.assemblyTime = timer.elapsed() ;
	Log::Debug() << "Step assembled  at " << timer.elapsed() << std::endl ;

	solve( config, dt, stepData, phase, rbData, stats ) ;

}

void PhaseSolver::solve(
    const Config& config, const Scalar dt, const PhaseStepData& stepData ,
    Phase& phase, std::vector< RigidBodyData > &rbData, Stats& stats ) const
{
	bogus::Timer timer ;


	// Compute rhs of momentum conservation -- gravity + u(t)
	DynVec rhs ;
	{
		DynVec forces = stepData.forms.externalForces ;

		// Inertia
#ifndef  FULL_FEM
		forces += config.volMass/dt * stepData.forms.phiu ;
#endif

		rhs = stepData.proj.vel * forces ;
	}

	// Solve unconstrained momentum equation
	DynVec u = stepData.forms.M_lumped_inv * rhs ;
	solveSDP( stepData.forms.A, stepData.forms.M_lumped_inv, rhs, u ) ;

	stats.linSolveTime = timer.elapsed() ;
	Log::Debug() << "Linear solve: " << stats.linSolveTime << std::endl ;

	// Optional : Maximum fraction projection
	if( config.enforceMaxFrac){
		Eigen::VectorXd depl ;
		enforceMaxFrac( config, stepData, rbData, depl );

		stepData.primalNodes.var2field( depl, phase.geo_proj ) ; //Purely geometric
		// u += depl/dt ; // Includes proj into inertia

		stats.lcpSolveTime = timer.elapsed() -  stats.linSolveTime ;
	}

	// Friction solve
	if (rbData.size() > 0) {
		solveComplementarity(config, dt, stepData, rbData, u, phase, stats);
	}

	// Output
	stepData.primalNodes.var2field( u, phase.velocity ) ;

	{
		// Velocities gradient D(u) and W(u)
		DynVec int_phiDu = .5 * ( stepData.proj.stress * stepData.forms.B * u ).head( SD * stepData.nDualNodes() ) ;
		DynVec int_phiWu = .5 * stepData.forms.J * u ;
		const DynVec int_phi = stepData.forms.fraction.max( 1.e-16 ) ;

		div_compwise<SD>( int_phiDu, int_phi ) ;
		div_compwise<RD>( int_phiWu, int_phi ) ;

		stepData.dualNodes.var2field( int_phiWu, phase.spi_grad ) ;
		stepData.dualNodes.var2field( int_phiDu, phase.sym_grad ) ;
	}
}


void PhaseSolver::addRigidBodyContrib( const Config &c, const Scalar dt, const PhaseStepData &stepData,
                                       const DynVec &u, const RigidBodyData &rb,
                                       PrimalData& pbData, DynArr &rbIntFraction ) const
{

	typename FormMat<SD,WD>::Type J = stepData.forms.S.inv_sqrt * stepData.Aniso * ( stepData.proj.stress * ( rb.jacobian ) ) ;

	pbData.H -= J * stepData.forms.M_lumped_inv_sqrt ;

	const DynVec delta_u = u - rb.projection.transpose() * rb.rb.velocities() ;

	pbData.w -= J * delta_u  ;

	// Add volume fraction taken by rb
	for( Index i = 0 ; i < rb.nodes.count() ; ++i ) {
		rbIntFraction += rb.intFraction ;
	}

	pbData.mu.segment( rb.nodes.offset, rb.nodes.count() ).setConstant( c.muRigid ) ;

	// Two-ways coupling
	PrimalData::InvInertiaType inv_inertia( 1, 1 ) ;
	rb.rb.inv_inertia( inv_inertia.insertBack(0,0) ) ;
	inv_inertia.finalize();

	pbData.inv_inertia_matrices.emplace_back( inv_inertia * dt ) ;
	pbData.jacobians.emplace_back( J * rb.projection.transpose() );
}

void PhaseSolver::addCohesionContrib (const Config&c, const PhaseStepData &stepData,
                                      PrimalData& pbData, DynVec &u ) const
{
	//Cohesion : add \grad{ c phi } to rhs
	DynVec cohe_force ;
	{
		const Scalar cohe_start = .999 * c.phiMax ;
		const DynArr contact_zone = (
		            ( stepData.forms.fraction/stepData.forms.volumes - cohe_start )
		            / (c.phiMax - cohe_start) ).max(0).min(1) ;

		DynVec cohe_stress  = DynVec::Zero( pbData.H.rows() ) ;
		component< SD >( cohe_stress, 0 ).head( stepData.cohesion.rows()).array() =
		        c.cohesion * stepData.cohesion * contact_zone ;
		cohe_force = pbData.H.transpose() * cohe_stress ;
	}

	pbData.w -= pbData.H * cohe_force ;
	u -= stepData.forms.M_lumped_inv_sqrt * cohe_force ;

}

void PhaseSolver::solveComplementarity(const Config &c, const Scalar dt, const PhaseStepData& stepData,
                                       std::vector<RigidBodyData> &rbData,
                                       DynVec &u, Phase& phase, Stats &simuStats ) const
{
	// Step counter, only useful for dumping friction problem pbData
	static unsigned s_stepId = 0 ;

	PrimalData	pbData ;
	pbData.mass_matrix_mode = PrimalData::Lumped ;

	pbData.H = stepData.forms.S.inv_sqrt * stepData.Aniso *
	        ( stepData.proj.stress * ( stepData.forms.B * stepData.forms.M_lumped_inv_sqrt ) ) ;
	pbData.w = stepData.forms.S.inv_sqrt * stepData.Aniso *
	        ( stepData.proj.stress * ( stepData.forms.B * u ) ) ;

	pbData.mu.resize( pbData.n() ) ;

	// Inertia, mu(I) = \delta_mu * (1./ (1 + I0/I) ), I = dp * sqrt( rho ) * inertia, inertia = |D(U)|/sqrt(p)
	const Scalar I0bar = c.I0 / ( c.grainDiameter * std::sqrt( c.volMass )) ;
	pbData.mu.segment(0,stepData.nDualNodes()).array() = c.mu +
	        c.delta_mu / ( 1. + I0bar / stepData.inertia.max(1.e-12) ) ;

	DynArr rbIntFraction( stepData.nDualNodes() ) ;
	rbIntFraction.setZero() ;

	// Rigid bodies
	pbData.jacobians.reserve( rbData.size() ) ;
	pbData.inv_inertia_matrices.reserve( rbData.size() ) ;
	std::vector< unsigned > coupledRbIndices ;

	for( unsigned k = 0 ; k < rbData.size() ; ++k ) {
		const RigidBodyData& rb = rbData[k] ;
		if( rb.nodes.count() == 0 )
			continue ;

		addRigidBodyContrib( c, dt, stepData, u, rb, pbData, rbIntFraction );

		// Do not use 2-ways coupling for bodies with very high inertias ( fixed boundaries)
		if( pbData.inv_inertia_matrices.back().block(0).squaredNorm() < 1.e-16 ) {
			pbData.inv_inertia_matrices.pop_back();
			pbData.jacobians.pop_back();
		} else {
			coupledRbIndices.push_back( k );
		}
	}

	//Cohesion : add \grad{ c phi } to rhs
	addCohesionContrib( c, stepData, pbData, u );

	// Compressability beta(phi)
	{
		const DynArr intBeta = ( c.phiMax*stepData.forms.volumes
		                         - stepData.forms.fraction - rbIntFraction );

		DynVec intBeta_s ( DynVec::Zero( pbData.n() * SD ) ) ;
		component< SD >( intBeta_s, 0 ).head( stepData.nDualNodes() ).array() = intBeta  * s_sqrt_2_d / dt  ;

		pbData.w += ( stepData.forms.S.inv_sqrt * intBeta_s ).cwiseMax(0) ;
	}

	// Warm-start stresses
	DynVec x( pbData.w.rows() ), y( pbData.w.rows() ) ;

	stepData.dualNodes.field2var( phase.stresses, x, false ) ;
	for( unsigned k = 0 ; k < rbData.size() ; ++k ) {
		RigidBodyData& rb = rbData[k] ;
		rb.nodes.field2var( rb.stresses, x, false ) ;
	}

	// Dump problem data if requested
	if( c.dumpPrimalData > 0 && (++s_stepId % c.dumpPrimalData) == 0 ) {
		pbData.dump( arg("primal-%1.d6", s_stepId).c_str() ) ;
	}

	// Proper solving
	FrictionSolver::Options options ;
	if( c.usePG ) {
		options.algorithm = FrictionSolver::Options::Cadoux_PG_NoAssembly ;
		options.projectedGradientVariant = 2 ;
	} else {
		options.algorithm = FrictionSolver::Options::GaussSeidel_NoAssembly ;
	}
	if( c.useInfNorm ) {
		// PG without infinity norm leads to creeping at end of simulation
		options.useInfinityNorm = true ;
		options.maxIterations = 1000 ;
	}

	FrictionSolver::Stats stats ;
	FrictionSolver( pbData ).solve( options, x, stats ) ;

	Log::Verbose() << arg3( "Primal: %1 iterations,\t err= %2,\t time= %3 ",
	                       stats.nIterations(), stats.residual(), stats.time() ) << std::endl ;
	simuStats.frictionError      = stats.residual() ;
	simuStats.frictionTime       = stats.time() ;
	simuStats.frictionIterations = stats.nIterations() ;

	if (simuStats.frictionError < 1.e-16)
		std::exit(1) ;

	// Update velocity
	u += stepData.forms.M_lumped_inv_sqrt * pbData.H.transpose() * x  ;

	// Contact forces -- useless, debug only
	{
		const DynVec fcontact = stepData.proj.vel * stepData.forms.B.transpose() *
		                        stepData.proj.stress * stepData.Aniso * stepData.forms.S.inv_sqrt * x ;
		stepData.primalNodes.var2field( fcontact, phase.fcontact ) ;
	}

	// Save stresses for warm-starting next step
	stepData.dualNodes.var2field( x, phase.stresses ) ;
	for( unsigned k = 0 ; k < rbData.size() ; ++k ) {
		RigidBodyData& rb = rbData[k] ;
		rb.nodes.var2field( x, rb.stresses ) ;
	}

	// Add contact forces to rigid bodies
	for( unsigned k = 0 ; k < coupledRbIndices.size() ; ++k ) {
		RigidBodyData& rb = rbData[ coupledRbIndices[k] ] ;
		const VecS forces = pbData.jacobians[k].transpose() * x ;
		rb.rb.integrate_forces( dt, forces );
	}

}

void PhaseSolver::enforceMaxFrac(const Config &c, const PhaseStepData &stepData,
                                 const std::vector<RigidBodyData> &rbData,
                                 DynVec& depl ) const
{

	LCPData pbData ;

	pbData.H = stepData.forms.C * stepData.proj.vel ;
	pbData.w.setZero( pbData.n() );

	DynArr rbIntFraction ( stepData.nDualNodes() ) ;
	rbIntFraction.setZero() ;

	for( const RigidBodyData& rb: rbData ) {

		if( rb.nodes.count() == 0 )
			continue ;

		for( Index i = 0 ; i < rb.nodes.count() ; ++i ) {
			rbIntFraction += rb.intFraction ;
		}
	}

	pbData.w.segment(0, stepData.nDualNodes() ) =
	        ( c.phiMax*stepData.forms.volumes - stepData.forms.fraction - rbIntFraction ) ;

	DynVec x = DynVec::Zero( pbData.n() ) ;
	LCP lcp( pbData ) ;
	lcp.solve( x ) ;

	depl = -( pbData.H.transpose() * x );

}


} //d6
