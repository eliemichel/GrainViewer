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

#ifndef D6_PHASE_SOLVE_HH
#define D6_PHASE_SOLVE_HH

#include "PhaseFields.hh"

#include "simu/ActiveIndices.hh"

#include "geo/BoundaryInfo.hh"
#include "geo/MeshBase.hh"

#include "utils/scalar.hh"

#include <vector>

namespace d6 {

class DynParticles ;
struct Phase ;
struct PhaseStepData ;

class RigidBody ;
struct RigidBodyData ;
struct PrimalData ;

struct Config ;
class Stats ;

class PhaseSolver {

public:
	explicit PhaseSolver(
			const DynParticles& particles
			) ;

	//! Solve for end-of-steps velocities, reading initial quantities from the particles
	void step(const Config &config, const Scalar dt,
			  Phase& phase, Stats &stats,
			  std::vector<RigidBody> &rigidBodies,
			  std::vector<RBStresses> &rbStresses
			  ) const ;

private:
	//! Two-steps solve of the momentum balance w/ frictional rheology
	void solve(const Config& config, const Scalar dt,
			   const PhaseStepData &stepData,
			   Phase& phase, std::vector< RigidBodyData > &rbData, Stats& stats ) const ;

	//! Assemble and solve the friction problem
	void solveComplementarity(const Config&c, const Scalar dt,
							  const PhaseStepData& stepData ,
							  std::vector< RigidBodyData >& rbData,
							  DynVec &u, Phase &phase, Stats &stats ) const ;

	//! Add contbutions from a rigid body to the friction problem
	void addRigidBodyContrib(const Config &c, const Scalar dt, const PhaseStepData &stepData,
							 const DynVec &u, const RigidBodyData &rb,
							 PrimalData& primalData, DynArr &rbIntFraction ) const ;
	//! Add contribution from chesive forces to the friction problem
	void addCohesionContrib (const Config&c, const PhaseStepData &stepData,
							  PrimalData& primalData, DynVec &u ) const ;

	//! Compute displacement for volume correction
	void enforceMaxFrac(const Config &c, const PhaseStepData &stepData,
									   const std::vector<RigidBodyData> &rbData,
									   DynVec &depl ) const ;


	const DynParticles& m_particles ;

};


} //d6


#endif
