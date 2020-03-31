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

#ifndef D6_SIMU_HH
#define D6_SIMU_HH

#include "simu/DynParticles.hh"

#include "utils/Stats.hh"

#include <memory>

namespace d6 {

struct Config ;
struct Phase ;
class RigidBody ;
class Scenario ;

class Simu {

public:

	explicit Simu( const Config& config, const char* base_dir ) ;
	virtual ~Simu() ;

	//! Runs the simulation
	void run() ;

	std::vector< RigidBody > &rigidBodies ()
	{
		return m_rigidBodies ;
	}
	const std::vector< RigidBody > &rigidBodies () const
	{
		return m_rigidBodies ;
	}

	DynParticles& particles() { return m_particles ; }
	const DynParticles& particles() const { return m_particles ; }

private:
	//! Advances the simulation with time step \p dt
	void step( const Scalar dt ) ;

	virtual void adapt_meshes() = 0 ;
	virtual void update_fields( const Scalar dt ) = 0 ;
	virtual void move_particles( const Scalar dt ) = 0 ;

	// Output
	virtual void dump_fields(unsigned frame) const = 0 ;
	void dump_particles(unsigned frame) const ;

	Simu( const Simu& ) = delete ;
	Simu& operator=( const Simu& ) = delete ;

protected:

	const Config& m_config ;
	const char* m_base_dir ;

	Stats		  m_stats ;
	DynParticles  m_particles ;
	std::unique_ptr<Scenario>  m_scenario ;

	std::vector< RigidBody   > m_rigidBodies ;

};

} //d6

#endif
