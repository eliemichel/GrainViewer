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

#ifndef D6_OFFLINE_HH
#define D6_OFFLINE_HH

#include "mono/PhaseMeshes.hh"

#include "geo/geo.fwd.hh"
#include "geo/Particles.hh"

#include "utils/Config.hh"

#include <memory>

namespace d6 {

struct Phase ;
class LevelSet ;

class Offline
{
public:

	explicit Offline( const char* base_dir ) ;
	~Offline() ;

	bool load_frame( unsigned frame_nb ) ;

	const Particles &particles() const { return m_particles ; }

	const PhaseMeshes &meshes() const { return m_meshes ; }
	const Phase &grains() const { return *m_grains ; }

	const char* base_dir() const
	{
		return m_base_dir ;
	}

	const std::vector< std::unique_ptr< LevelSet > >& levelSets() const
	{
		return m_levelSets ;
	}

	Scalar frame_dt () const {
		return 1./ m_config.fps ;
	}

	const Particles::EventLog& events() const {
		return m_events ;
	}

	const Config& config() const {
		return m_config ;
	}

	const Vec& box() const { return config().box ; }

private:
	const char* m_base_dir ;
	Config m_config ;

	Particles  m_particles ;
	Particles::EventLog  m_events ;

	PhaseMeshes  m_meshes ;
	std::unique_ptr< Phase >     m_grains ;

	std::vector< std::unique_ptr< LevelSet > > m_levelSets ;
} ;

} //d6

#endif
