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

#include "MeshShapeFunction.hh"

#include "Grid.hh"
#include "geo/Voxel.hh"

#include "TetGrid.hh"
#include "geo/Tet.hh"

#include "Octree.hh"

namespace d6 {

template<>
void Linear<Grid>::dof_coeffs( const typename MeshType::Coords& coords, typename Base::CoefList& coeffs ) const
{
	for( int k = 0 ; k < Voxel::NV ; ++k )
	{
		coeffs[ k ] = Voxel::cornerCoeff( Voxel::corner( k ), coords ) ;
	}
}

template<>
void Linear<Grid>::list_nodes( const Location& loc, typename Base::NodeList& nodes ) const
{
	for( int k = 0 ; k < Voxel::NV ; ++k )
	{
		nodes[ k ] = mesh().nodeIndex( loc.cell + Voxel::corner( k ) ) ;
	}
}

template<>
void Linear<Grid>::get_derivatives( const Location& loc, typename Base::Derivatives& dc_dx ) const
{

	for( int k = 0 ; k < Voxel::NV ; ++k )
	{
		Voxel::getCornerDerivatives( Voxel::corner( k ), loc.coords, dc_dx.row( k ) );
	}

	for (int k = 0 ; k < WD ; ++k)
		dc_dx.col( k ) /= mesh().dx()[k] ;
}

template<>
void Linear<Octree>::dof_coeffs( const typename MeshType::Coords& coords, typename Base::CoefList& coeffs ) const
{
	for( int k = 0 ; k < Voxel::NV ; ++k )
	{
		coeffs[ k ] = Voxel::cornerCoeff( Voxel::corner( k ), coords ) ;
	}
}

template<>
void Linear<Octree>::list_nodes( const Location& loc, typename Base::NodeList& nodes ) const
{
	mesh().list_nodes( loc.cell, nodes ) ;
}

template<>
void Linear<Octree>::get_derivatives( const Location& loc, typename Base::Derivatives& dc_dx ) const
{

	for( int k = 0 ; k < Voxel::NV ; ++k )
	{
		Voxel::getCornerDerivatives( Voxel::corner( k ), loc.coords, dc_dx.row( k ) );
	}

	const int r = mesh().cellRes(loc.cell) ;
	for (int k = 0 ; k < WD ; ++k)
		dc_dx.col( k ) *= r/mesh().dx()[k] ;
}

template<>
void Linear<TetGrid>::dof_coeffs( const typename MeshType::Coords& coords, typename Base::CoefList& coeffs ) const
{
	coeffs = coords ;
}


template<>
void Linear<TetGrid>::list_nodes( const Location& loc, typename Base::NodeList& nodes ) const
{
	Tet geo ;
	mesh().get_geo(  loc.cell, geo ) ;

	for( Index k = 0 ; k < NI ; ++k ) {
		const VecWi v = ( geo.vertex(k).array() / mesh().dx().array()  + .5 ).cast<Index>() ;
		nodes[k] = mesh().nodeIndex( v ) ;
	}

}

template<>
void Linear<TetGrid>::get_derivatives( const Location& loc, typename Base::Derivatives& dc_dx ) const
{
	Tet geo ;
	mesh().get_geo(  loc.cell, geo ) ;

	geo.compute_derivatives( loc.coords, dc_dx ) ;
}


} // d6
