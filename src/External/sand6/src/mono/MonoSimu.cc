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

#include "MonoSimu.hh"

#include "Phase.hh"
#include "simu/RigidBody.hh"

#include "utils/Log.hh"
#include "utils/File.hh"
#include "utils/Config.hh"

#include "geo/MeshImpl.hh"

#include "geo/Meshes.io.hh"

#include "utils/serialization.hh"

#include <boost/archive/binary_oarchive.hpp>

#include <bogus/Core/Utils/Timer.hpp>

namespace d6
{

MonoSimu::MonoSimu(const Config &config, const char *base_dir)
	: Simu( config, base_dir ),
	  m_meshes{ std::unique_ptr<PrimalMesh>(new PrimalMesh( m_config.box, m_config.res, &m_particles.geo() )),
				std::unique_ptr<  DualMesh>(new   DualMesh( m_config.box, m_config.res, &m_particles.geo() ))
			   },
	  m_grains( new Phase( meshes() ) ),
	  m_solver( m_particles )
{
	m_particles.generate( config, meshes().primal(), *m_scenario );

	for( unsigned i = 0 ; i < m_rigidBodies.size() ; ++i ) {
		m_rbStresses.emplace_back( meshes().primal() );
		m_rbStresses.back().set_zero() ;
	}

	m_grains->serializeAllFields( m_config.exportAllFields ) ;

	m_grains->fraction.set_zero();
	m_grains->stresses.set_zero();
	m_grains->velocity.set_zero();
	m_grains->sym_grad.set_zero();
	m_grains->spi_grad.set_zero();
	m_grains->geo_proj.set_zero();
}

MonoSimu::~MonoSimu()
{

}

void MonoSimu::adapt_meshes()
{
	m_meshes.adapt( m_particles, m_grains );
}

void MonoSimu::update_fields(const Scalar dt)
{
	m_stats.nNodes = meshes().primal().nNodes() ;

	//! Compute new grid velocities
	m_solver.step( m_config, dt, *m_grains, m_stats, m_rigidBodies, m_rbStresses ) ;
}

void MonoSimu::move_particles(const Scalar dt)
{
	m_particles.update( m_config, dt, *m_grains ) ;
}

void MonoSimu::dump_fields( unsigned frame ) const
{
	FileInfo dir( FileInfo(m_base_dir).filePath( arg("frame-%1", frame ) ) ) ;
	dir.makePath() ;
	if( ! dir.exists() )
		dir.makeDir() ;

	// Grid
	{
		std::ofstream ofs( dir.filePath("meshes") );
		boost::archive::binary_oarchive oa(ofs);
		oa << meshes() ;
	}
	// Velocity, Stress, Phi
	{
		std::ofstream ofs( dir.filePath("fields") );
		boost::archive::binary_oarchive oa(ofs);
		oa << *m_grains ;
	}
}


} //d6
