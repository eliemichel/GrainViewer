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

#ifndef D6_MESH_IMPL_HH
#define D6_MESH_IMPL_HH

#include "geo.fwd.hh"

#if( D6_MESH_IMPL == D6_MESH_TET_GRID )
#include "TetGrid.hh"
#elif( D6_MESH_IMPL == D6_MESH_OCTREE )
#include "Octree.hh"
#else
#include "Grid.hh"
#endif

#include "geo/MeshShapeFunction.hh"
#include "geo/P2ShapeFunction.hh"
#include "geo/UnstructuredShapeFunction.hh"

#endif
