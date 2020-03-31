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

#include "MeshBase.hh"

#include "Grid.hh"
#include "TetGrid.hh"
#include "Octree.hh"

namespace d6 {

template <typename Derived>
Vec MeshBase< Derived>::pos( const Location& loc ) const
{
	CellGeo geo ;
	get_geo( loc.cell, geo ) ;
	return geo.pos( loc.coords ) ;
}

template class MeshBase< Grid > ;
template class MeshBase< Octree > ;
template class MeshBase< TetGrid > ;


}  //d6
