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

#ifndef D6_MESH_TRAITS_HH
#define D6_MESH_TRAITS_HH


#include "geo/Voxel.hh"
#include "geo/Tet.hh"

#include <vector>

namespace d6 {

template <typename Derived >
struct MeshTraits {
};

// Grid

class  Grid ;
struct GridIterator ;

template < >
struct MeshTraits< Grid > {
	typedef GridIterator CellIterator ;

	typedef ArrWi Cell    ;
	typedef Voxel CellGeo ;
	static constexpr Index NV = CellGeo::NV ;
	static constexpr Index NC = CellGeo::NC ;

	typedef std::vector<Cell> Cells ;
};

// TetGrid

class  TetGrid ;
struct TetGridIterator ;

template < >
struct MeshTraits< TetGrid > {
	typedef TetGridIterator CellIterator ;

	typedef Eigen::Array< int, WD+1, 1 > Cell  ;
	typedef Tet CellGeo ;
	static constexpr Index NV = CellGeo::NV ;
	static constexpr Index NC = CellGeo::NC ;

	typedef std::vector<Cell> Cells ;
};


// Octree

class  Octree ;
struct OctreeIterator ;

template < >
struct MeshTraits< Octree > {
	typedef OctreeIterator CellIterator ;

	typedef Eigen::Array< Index, WD+1, 1 > Cell    ;
	typedef Voxel CellGeo ;
	static constexpr Index NV = CellGeo::NV ;
	static constexpr Index NC = CellGeo::NC ;

	typedef std::vector<Cell> Cells ;
};

} //d6

#endif
