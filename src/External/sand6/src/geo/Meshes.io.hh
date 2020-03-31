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

#ifndef D6_MESHES_IO_HH
#define D6_MESHES_IO_HH

#include "Octree.hh"
#include "UnstructuredShapeFunction.hh"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>

namespace d6 {

template < typename Archive >
void Octree::save( Archive &ar, unsigned int ) const
{
	ar << m_dim ;
	ar << m_dx ;
	ar << m_maxDepth ;
	ar << m_trees ;
}

template < typename Archive >
void Octree::load( Archive &ar, unsigned int )
{
	ar >> m_dim ;
	ar >> m_dx ;
	ar >> m_maxDepth ;
	ar >> m_trees ;

	rebuild() ;
}


template < typename Archive >
void Octree::serialize( Archive &ar, unsigned int file_version )
{
	boost::serialization::split_member(ar, *this, file_version);
}


template < typename Archive >
void UnstructuredDOFs::save( Archive &ar, unsigned int ) const
{
	ar << m_count ;
	ar << m_box ;
	ar << m_res ;
}

template < typename Archive >
void UnstructuredDOFs::load( Archive &ar, unsigned int )
{
	ar >> m_count ;
	ar >> m_box ;
	ar >> m_res ;

	rebuild();
}


template < typename Archive >
void UnstructuredDOFs::serialize( Archive &ar, unsigned int file_version )
{
	boost::serialization::split_member(ar, *this, file_version);
}



}

#endif
