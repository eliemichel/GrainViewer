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

#ifndef D6_PARTICLES_IO_HH
#define D6_PARTICLES_IO_HH

#include "Particles.hh"

#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>

namespace d6 {

template < typename Archive >
void Particles::serialize( Archive &ar, unsigned int ) {
	using boost::serialization::make_array ;

	ar & m_count ;
	ar & make_array( m_volumes.data(),       m_count ) ;
	ar & make_array( m_centers.data(),    WD*m_count ) ;
	ar & make_array( m_velocities.data(), WD*m_count ) ;
	ar & make_array( m_frames.data(),     SD*m_count ) ;
	ar & make_array( m_orient.data(),     SD*m_count ) ;
}

template < typename Archive >
void Particles::Event::serialize( Archive &ar, unsigned int ) {
	ar & type ;
	ar & first ;
	ar & second ;
	ar & dx ;
}

}//d6

#endif

