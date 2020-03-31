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

#ifndef D6_DYN_PARTICLES_HH
#define D6_DYN_PARTICLES_HH

#include "mono/PhaseFields.hh"

#include "geo/Particles.hh"

namespace d6 {

struct Config ;
struct Phase ;
class Scenario ;

class DynParticles {

public:
	DynParticles() ;

	void generate( const Config &c, const MeshType& mesh, const Scenario &scenario ) ;

	void update( const Config&c, const Scalar dt, const Phase& phase ) ;

	void integratePrimal( std::vector< bool > &activeCells,
			   PrimalScalarField &phi,    PrimalVectorField &phiVel ) const ;

	void integrateDual(
			   DualScalarField &phi,      DualScalarField &phiInertia,
			   DualTensorField &phiOrient,DualScalarField &phiCohesion
			   ) const ;

	const Particles &geo() const { return m_geo ; }
	Particles &geo() { return m_geo ; }

	size_t count() const { return m_geo.count() ; }

	void remove( size_t particleId ) ;

	const Particles::EventLog& events() const {
		return m_events ;
	}
	Particles::EventLog& events() {
		return m_events ;
	}

	const Particles::Data< 1 >::Type&  inertia() const { return m_inertia ; }
	const Particles::Data< 1 >::Type& cohesion() const { return m_cohesion ; }

private:

	void clamp_particle( size_t i, const MeshType &mesh ) ;

	void splitMerge( const MeshType & mesh ) ;

	void resize( size_t n ) ;

	Particles m_geo ;
	Particles::EventLog m_events ;
	Scalar m_meanVolume ;

	Particles::Data< WD*WD >::Type m_affine ;
	Particles::Data< 1 >::Type m_inertia ;
	Particles::Data< 1 >::Type m_cohesion ;

};

} //d6

#endif
