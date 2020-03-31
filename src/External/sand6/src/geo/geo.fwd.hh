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

#ifndef D6_GEO_FWD_HH
#define D6_GEO_FWD_HH

#define D6_MESH_GRID     0
#define D6_MESH_TET_GRID 1
#define D6_MESH_OCTREE   2

#ifndef D6_MESH_IMPL
#define D6_MESH_IMPL D6_MESH_GRID
#endif

namespace d6 {

template < typename M > class MeshBase ;
template < typename S > class ShapeFuncBase ;


template< typename ValueType > struct Expr ;

template< typename Derived > class FieldBase ;
template< typename Derived > struct FieldTraits ;
template < typename ShapeFunc > class AbstractScalarField ;
template < typename ShapeFunc > class AbstractVectorField ;
template < typename ShapeFunc > class AbstractTensorField ;
template < typename ShapeFunc > class AbstractSkewTsField ;


class Grid ;
class Octree ;
class TetGrid ;

#if( D6_MESH_IMPL == D6_MESH_TET_GRID )
typedef TetGrid MeshImpl ;
#elif( D6_MESH_IMPL == D6_MESH_OCTREE )
typedef Octree  MeshImpl ;
#else
typedef Grid    MeshImpl ;
#endif

template < typename MeshT > struct Linear ;
template < typename MeshT > struct DGLinear ;
template < typename MeshT > struct DGConstant ;
template < typename MeshT > struct P2 ;

struct UnstructuredDOFs ;
struct UnstructuredShapeFunc ;

typedef MeshBase< MeshImpl > MeshType ;

template < typename CellT, int Order >
struct QuadraturePoints ;

}

#endif
