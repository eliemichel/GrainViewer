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

#include "Simu.hh"

#include "Scenario.hh"
#include "RigidBody.hh"

#include "utils/Log.hh"
#include "utils/File.hh"
#include "utils/Config.hh"

#include "geo/LevelSet.io.hh"
#include "geo/Particles.io.hh"

#include "utils/serialization.hh"

#include <boost/archive/binary_oarchive.hpp>

#include <bogus/Core/Utils/Timer.hpp>

namespace d6 {


Simu::Simu(const Config &config, const char *base_dir)
	: m_config(config), m_base_dir( base_dir ),
	  m_stats( m_base_dir ),
	  m_scenario( Scenario::parse( config ) )
{

	// Rigid bodies
	m_scenario->add_rigid_bodies( m_rigidBodies ) ;
}

Simu::~Simu()
{
}

void Simu::run()
{

	if( m_config.output ) {
		dump_particles( 0 ) ;
		dump_fields( 0 ) ;
	}
	m_particles.events().start();

	for( unsigned frame = 0 ; frame < m_config.nFrames ; ++ frame ) {
		bogus::Timer timer ;
		Log::Info() << "Starting frame " << (frame+1) << std::endl ;

		unsigned substeps = m_config.substeps ;
		if( substeps == 0 ) { //Adaptative timestepping
			substeps = std::ceil( std::max(1., m_stats.maxVelocity) / m_config.fps ) ;
		}

		for( unsigned s = 0 ; s < substeps ; ++ s ) {

			m_stats.frameId = frame ;
			m_stats.delta_t = 1./(m_config.fps * substeps) ;

			const Scalar t = m_config.time( frame ) + s * m_stats.delta_t ;
			Log::Verbose() << arg3( "Step %1/%2 \t t=%3 s",
									s+1, substeps,
									t * m_config.units().toSI( Units::Time ) ) << std::endl ;
			// Update external objects (moving boundaries,...)
			m_scenario->update( *this, t, m_stats.delta_t ) ;

			adapt_meshes();

			// Dump particles at last substep of each frame
			if( m_config.output && substeps == s+1 ) {
				dump_particles( frame+1 ) ;
				m_particles.events().clear();
			}
			m_particles.events().start();

			// Proper simulation step
			step( m_stats.delta_t ) ;

			// Log max particle velocity (useful for adaptative timestep )
			m_stats.maxVelocity = m_particles.geo().velocities().leftCols(m_particles.count()).lpNorm< Eigen::Infinity >() ;
			Log::Debug() << "Max particle vel: " << m_stats.maxVelocity << std::endl ;

			m_stats.dump();
		}

		Log::Info() << arg( "Frame done in %1 s", timer.elapsed() ) << std::endl ;

		if( m_config.output ) {
			dump_fields( frame+1 ) ;
		}
	}

	Log::Info() << "All done." << std::endl ;

}

void Simu::step(const Scalar dt)
{
	bogus::Timer timer ;

	m_stats.nParticles = m_particles.count() ;

	//! Compute new grid velocities
	update_fields( dt ) ;

	const Scalar solverTime = timer.elapsed() ;

	//! Advance particles
	move_particles( dt ) ;

	for( RigidBody& rb: m_rigidBodies ) {
		rb.move( dt );
	}

	m_stats.totalTime = timer.elapsed() ;
	m_stats.advectionTime = timer.elapsed() - solverTime ;

	Log::Verbose() << arg( "Step done in %1 s", m_stats.totalTime ) << std::endl ;
}


void Simu::dump_particles( unsigned frame ) const
{
	FileInfo dir( FileInfo(m_base_dir).filePath( arg("frame-%1", frame ) ) ) ;
	dir.makePath() ;
	if( ! dir.exists() )
		dir.makeDir() ;

	// Particles
	{
		std::ofstream ofs( dir.filePath("particles") );
		boost::archive::binary_oarchive oa(ofs);
		oa << m_particles.geo() ;
	}
	// Objects
	{
		std::ofstream ofs( dir.filePath("objects") );
		boost::archive::binary_oarchive oa(ofs);

		unsigned n = m_rigidBodies.size() ;
		oa << n ;
		LevelSet::register_derived(oa) ;
		for( unsigned i = 0 ; i < n ; ++i ) {
			const LevelSet* ptr = m_rigidBodies[i].levelSetPtr() ;
			oa << ptr ;
		}
	}

	// Log -- save at prevbious frame
	if( frame > 0 ) {
		FileInfo dir( FileInfo(m_base_dir).filePath( arg("frame-%1", frame-1 ) ) );
		std::ofstream ofs( dir.filePath("log") );
		boost::archive::binary_oarchive oa(ofs);
		oa << m_particles.events() ;
	}
}


} //d6
