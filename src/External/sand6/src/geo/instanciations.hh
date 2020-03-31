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

#ifndef D6_INSTANTIATIONS_HH
#define D6_INSTANTIATIONS_HH

#include "Grid.hh"
#include "Octree.hh"
#include "TetGrid.hh"

#include "MeshShapeFunction.hh"
#include "P2ShapeFunction.hh"
#include "UnstructuredShapeFunction.hh"


#define EXPAND_INSTANTIATIONS \
	INSTANTIATE(     Linear<Grid   >  ) \
	INSTANTIATE(   DGLinear<Grid   >  ) \
	INSTANTIATE( DGConstant<Grid   >  ) \
	INSTANTIATE(     Linear<Octree >  ) \
	INSTANTIATE(   DGLinear<Octree >  ) \
	INSTANTIATE( DGConstant<Octree >  ) \
	INSTANTIATE(     Linear<TetGrid>  ) \
	INSTANTIATE(   DGLinear<TetGrid>  ) \
	INSTANTIATE( DGConstant<TetGrid>  ) \
	INSTANTIATE( UnstructuredShapeFunc  ) \
	EXPAND_INSTANTIATIONS_DIM

#define EXPAND_INSTANTIATIONS_2D \
	INSTANTIATE( P2<TetGrid> ) \

#define EXPAND_INSTANTIATIONS_3D \


#if D6_DIM == 2
	#define EXPAND_INSTANTIATIONS_DIM EXPAND_INSTANTIATIONS_2D
#else
	#define EXPAND_INSTANTIATIONS_DIM EXPAND_INSTANTIATIONS_3D
#endif

#endif
