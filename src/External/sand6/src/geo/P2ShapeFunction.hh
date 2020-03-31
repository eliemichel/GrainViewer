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

#ifndef D6_P2_SHAPE_FUNCTION_HH
#define D6_P2_SHAPE_FUNCTION_HH

#include "MeshShapeFunction.hh"

namespace d6 {

// P2 shape func -- for triangle/tet meshes

template< typename MeshT >
struct ShapeFuncTraits< P2<MeshT>  > : public ShapeFuncTraits< MeshShapeFunc< P2, MeshT >  >
{
	typedef ShapeFuncTraits< MeshShapeFunc< P2, MeshT > > Base ;

	template <typename CellIterator = typename Base::MeshType::CellIterator >
	struct QPIterator {
		typedef MeshQPIterator< MeshT, CellIterator, 4 > Type ;
	};
	enum {
		NI = WD == 2 ? 6 : 10 ,
	};
} ;

template <typename MeshT>
struct P2 : public MeshShapeFunc< P2, MeshT >
{
	typedef MeshShapeFunc< P2, MeshT > Base ;
	typedef MeshBase<MeshT> MeshType ;
	typedef typename Base::Location Location ;

	P2 ( const MeshType & mesh )
		: Base( mesh )
	{
		static_assert( MeshT::NV == WD+1, "P2 only available for triangular (2D) and tet (3d) meshes" ) ;
	}

	Index nDOF() const { return Base::mesh().nNodes() + Base::mesh().nEdges() ; }

	void interpolate( const Location& loc, typename Base::Interpolation& itp ) const
	{ interpolate( loc, itp.nodes, itp.coeffs ); }
	void get_derivatives( const Location& loc, typename Base::Derivatives& dc_dx ) const ;

	using Base::list_nodes ;
	void list_nodes( const Location& loc, typename Base::NodeList& list ) const ;

	void locate_dof( typename Base::Location& loc, Index dofIndex ) const {
		typename MeshType::CellGeo geo ;
		Base::mesh().get_geo( loc.cell, geo ) ;
		dof_coords( geo, dofIndex, loc.coords );
	}

	void interpolate( const Location& loc,
					  typename Base::NodeList& nodes, typename Base::CoefList& coeffs ) const
	{
		dof_coeffs( loc.coords, coeffs ) ;
		list_nodes( loc, nodes ) ;
	}

	void build_visu_mesh( DynMatW& vertices, DynMati& indices ) const ;

	void interpolate_tpz( const Location& loc, typename Base::Interpolation& itp ) const
	{
		dof_coeffs_tpz( loc.coords, itp.coeffs ) ;
		list_nodes( loc, itp.nodes ) ;
	}

	Scalar dof_volume_fraction( Index ) const	{ return 1./Base::NI ; }

private:

	void dof_coeffs( const typename MeshType::Coords& coords, typename Base::CoefList& coeffs ) const ;
	void dof_coeffs_tpz( const typename MeshType::Coords& coords, typename Base::CoefList& coeffs ) const ;

	void dof_coords( const typename MeshType::CellGeo& geo, Index dofIndex, typename MeshType::Coords& coords  ) const
	{
		if( dofIndex < MeshT::NV )
			geo.vertexCoords( dofIndex, coords ) ;
		else
			geo.edgeCenterCoords( dofIndex-MeshT::NV, coords ) ;
	}
} ;

} //d6

#endif
