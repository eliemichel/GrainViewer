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

#ifndef D6_MESH_SHAPE_FUNCTION_HH
#define D6_MESH_SHAPE_FUNCTION_HH

#include "ShapeFunctionBase.hh"
#include <iostream>

namespace d6 {

// MeshShapeFunction
template< template< typename  > class Interp, typename MeshT >
struct MeshShapeFunc ;

template< template< typename > class Interp, typename MeshT >
struct ShapeFuncTraits< MeshShapeFunc< Interp, MeshT>  >
{
	typedef MeshBase<MeshT> MeshType ;
	typedef typename MeshType::Location Location ;

	typedef MeshType DOFDefinition ;
	static bool constexpr is_mesh_based = true ;
} ;

template< template< typename  > class Interp, typename MeshT >
struct MeshShapeFunc : public ShapeFuncBase< Interp<MeshT> >
{
	typedef ShapeFuncBase< Interp<MeshT> > Base ;
	typedef MeshBase<MeshT> MeshType ;
	typedef typename Base::Traits Traits ;

	MeshShapeFunc ( const MeshType & mesh )
		: m_mesh( mesh )
	{}

	// ~ lumped mass matrix
	void compute_lumped_mass( DynVec& volumes ) const
	{
		volumes.resize( Base::nDOF() ) ;
		volumes.setZero() ;

		typename Base::Location loc ;
		typename Base::Interpolation itp ;

		for( auto qpIt = Base::qpBegin() ; qpIt != Base::qpEnd() ; ++qpIt  ) {
			qpIt.locate(loc) ;
			Base::interpolate(loc, itp);

			for( Index k = 0 ; k < itp.nodes.size() ; ++k ) {
				volumes[itp.nodes[k]] += itp.coeffs[k]*qpIt.weight() ;
			}
		}
	}

	// ~ lumped mass matrix
	void compute_tpz_mass( DynVec& volumes ) const
	{
		volumes.resize( Base::nDOF() ) ;
		volumes.setZero() ;

		typename MeshType::CellGeo cellGeo ;

		typename Base::NodeList nodes ;
		for( auto it = m_mesh.cellBegin() ; it !=  m_mesh.cellEnd() ; ++it )
		{
			list_nodes( *it, nodes ) ;

			m_mesh.get_geo( *it, cellGeo );
			const Scalar vol = cellGeo.volume() ;

			for ( int k = 0 ; k < Base::NI ; ++k ) {
				volumes[nodes[k]] += vol * dof_volume_fraction( k ) ;
			}
		}
	}


	void build_visu_mesh( DynMatW& vertices, DynMati& indices ) const
	{
		constexpr Index NV = MeshType::NV ;

		indices.resize( NV, m_mesh.nCells() ) ;
		vertices.resize( WD, Base::nDOF() );

		typename MeshType::CellGeo cellGeo ;
		typename Base::NodeList cellNodes ;
		for( typename MeshType::CellIterator it = m_mesh.cellBegin() ; it != m_mesh.cellEnd() ; ++it )
		{
			m_mesh.get_geo( *it, cellGeo ) ;
			list_nodes( *it, cellNodes );

			indices.col( it.index() ) = cellNodes ;

			for( int k = 0 ; k < NV ; ++k ) {
				vertices.col( cellNodes[k] ) = cellGeo.vertex( k ) ;
			}
		}
	}

	void list_nodes( const typename MeshType::Cell& cell, typename Base::NodeList& list ) const {
		typename Base::Location loc ;
		loc.cell = cell ;
		Base::derived().list_nodes( loc, list ) ;
	}

	void locate_by_pos_or_id( const Vec&x, const Index, typename Base::Location & loc ) const {
		m_mesh.locate( x, loc ) ;
	}
	void locate( const Vec&x, typename Base::Location & loc ) const {
		m_mesh.locate( x, loc ) ;
	}
	typename Base::Location locate( const Vec& x ) const {
		typename Base::Location loc ;
		m_mesh.locate( x, loc ) ;
		return loc ;
	}

	const MeshT& mesh()  const {
		return m_mesh.derived() ;
	}

	const typename Base::DOFDefinition& dofDefinition() const
	{ return mesh() ;  }

	typename Traits::template QPIterator<>::Type qpBegin() const {
		return typename Traits::template QPIterator<>::Type( m_mesh, m_mesh.cellBegin() ) ;
	}

	typename Traits::template QPIterator<>::Type qpEnd() const {
		return typename Traits::template QPIterator<>::Type( m_mesh, m_mesh.cellEnd() ) ;
	}

	template <typename CellIterator>
	typename Traits::template QPIterator<CellIterator>::Type qpIterator( const CellIterator &it ) const {
		return typename Traits::template QPIterator<CellIterator>::Type( m_mesh, it ) ;
	}

	Scalar dof_volume_fraction( Index k ) const
	{ return Base::derived().dof_volume_fraction(k) ; }

protected:
	const MeshType& m_mesh ;

};

// Iterator


template< typename MeshT, typename CellIterator = typename MeshT::CellIterator, Index Order = 2 >
struct MeshQPIterator {

	typedef MeshBase< MeshT > MeshType ;
	typedef QuadraturePoints< typename MeshT::CellGeo, Order > QP ;

	explicit MeshQPIterator( const MeshType &mesh_, const CellIterator& cIt )
		: mesh(mesh_), cellIt( cIt ), qp(0), cache_valid( false )
	{
	}

	bool operator==( const MeshQPIterator& o ) const
	{ return o.cellIt == cellIt && qp == o.qp ;}
	bool operator!=( const MeshQPIterator& o ) const
	{ return o.cellIt != cellIt || qp != o.qp ; }

	MeshQPIterator& operator++() {
		if( ++qp == QP::NQ ) {
			qp = 0 ;
			++cellIt ;
			cache_valid = false ;
		}
		return *this ;
	}

	Vec pos() const {
		if(! cache_valid ) update_cache();
		typename MeshType::Coords coords ;
		QP::get( cached_geo, qp, coords ) ;
		return cached_geo.pos( coords ) ;
	}
	Scalar weight() const {
		if(! cache_valid ) update_cache();
		return QP::weight( cached_geo, qp ) ;
	}
	void locate( typename MeshType::Location &loc ) const {
		if(! cache_valid ) update_cache();
		loc.cell = *cellIt ;
		QP::get( cached_geo, qp, loc.coords ) ;
	}

private:
	const MeshType& mesh ;

	CellIterator cellIt ;
	Index qp ;

	mutable bool cache_valid ;
	mutable typename MeshType::CellGeo cached_geo ;

	void update_cache() const {
		mesh.get_geo( *cellIt, cached_geo ) ;
		cache_valid = true ;
	}
};




// Linear shape func

template< typename MeshT >
struct ShapeFuncTraits< Linear<MeshT>  > : public ShapeFuncTraits< MeshShapeFunc< Linear, MeshT >  >
{
	typedef ShapeFuncTraits< MeshShapeFunc< Linear, MeshT > > Base ;

	template <typename CellIterator = typename Base::MeshType::CellIterator >
	struct QPIterator {
		typedef MeshQPIterator< MeshT, CellIterator > Type ;
	};

	enum {
		NI = Base::MeshType::NV
	};
} ;

template <typename MeshT>
struct Linear : public MeshShapeFunc< Linear, MeshT >
{
	typedef MeshShapeFunc< Linear, MeshT > Base ;
	typedef MeshBase<MeshT> MeshType ;
	typedef typename Base::Location Location ;

	Linear ( const MeshType & mesh )
		: Base( mesh )
	{}

	Index nDOF() const { return Base::mesh().nNodes() ; }

	void interpolate( const Location& loc, typename Base::Interpolation& itp ) const
	{
		dof_coeffs( loc.coords, itp.coeffs ) ;
		list_nodes( loc, itp.nodes ) ;
	}

	void dof_coeffs( const typename MeshType::Coords& coords, typename Base::CoefList& coeffs ) const ;
	using Base::list_nodes ;
	void list_nodes( const Location& loc, typename Base::NodeList& list ) const ;

	void get_derivatives( const Location& loc, typename Base::Derivatives& dc_dx ) const ;

	void locate_dof( typename Base::Location& loc, Index dofIndex ) const {
		typename MeshType::CellGeo geo ;
		Base::mesh().get_geo( loc.cell, geo ) ;
		geo.vertexCoords( dofIndex, loc.coords ) ;
	}

	void interpolate_tpz( const Location& loc, typename Base::Interpolation& itp ) const
	{ interpolate(loc,itp) ; }

	Scalar dof_volume_fraction( Index ) const	{ return 1./Base::NI ; }
} ;

// DG Linear shape func

template< typename MeshT >
struct ShapeFuncTraits< DGLinear<MeshT>  > : public ShapeFuncTraits< MeshShapeFunc< DGLinear, MeshT >  >
{
	typedef ShapeFuncTraits< MeshShapeFunc< DGLinear, MeshT > > Base ;
	enum {
		NI = Base::MeshType::NV
	};

	template <typename CellIterator = typename Base::MeshType::CellIterator >
	struct QPIterator {
		typedef MeshQPIterator< MeshT, CellIterator > Type ;
	};

} ;

template <typename MeshT>
struct DGLinear : public MeshShapeFunc< DGLinear, MeshT >
{
	typedef MeshShapeFunc< DGLinear, MeshT > Base ;
	typedef MeshBase<MeshT> MeshType ;
	typedef typename Base::Location Location ;

	DGLinear ( const MeshType & mesh )
		: Base( mesh )
	{}

	Index nDOF() const { return Base::mesh().nCells() * MeshType::NV ; }

	void interpolate( const Location& loc, typename Base::Interpolation& itp ) const {
		const Linear<MeshT> lin ( Base::mesh() ) ;
		lin.dof_coeffs( loc.coords, itp.coeffs ) ;
		list_nodes( loc, itp.nodes ) ;
	}
	void get_derivatives( const Location& loc, typename Base::Derivatives& dc_dx ) const {
		const Linear<MeshT> lin ( Base::mesh() ) ;
		lin.get_derivatives( loc, dc_dx ) ;
	}

	void locate_dof( typename Base::Location& loc, Index dofIndex ) const {
		typename MeshType::CellGeo geo ;
		Base::mesh().get_geo( loc.cell, geo ) ;
		geo.vertexCoords( dofIndex, loc.coords ) ;
	}

	// Orderin:g all nodes from cell 0, all nodes from cell, ... etc
	using Base::list_nodes ;
	void list_nodes( const Location& loc, typename Base::NodeList& list ) const {
		const Index cellIdx = Base::mesh().cellIndex( loc.cell ) ;
		for( Index k = 0 ; k < MeshType::NV ; ++ k ) {
			list[k] = cellIdx * MeshType::NV + k ;
		}
	}

	void interpolate_tpz( const Location& loc, typename Base::Interpolation& itp ) const
	{ interpolate(loc,itp) ; }

	Scalar dof_volume_fraction( Index ) const	{ return 1./Base::NI ; }

} ;

// Piecesize-constant shape func

template< typename MeshT >
struct ShapeFuncTraits< DGConstant<MeshT>  > : public ShapeFuncTraits< MeshShapeFunc< DGConstant, MeshT >  >
{
	typedef ShapeFuncTraits< MeshShapeFunc< DGConstant, MeshT > > Base ;
	enum {
		NI = 1
	};

	template <typename CellIterator = typename Base::MeshType::CellIterator >
	struct QPIterator {
		typedef MeshQPIterator< MeshT, CellIterator, 1 > Type ;
	};

} ;

template <typename MeshT>
struct DGConstant : public MeshShapeFunc< DGConstant, MeshT >
{
	typedef MeshShapeFunc< DGConstant, MeshT > Base ;
	typedef MeshBase<MeshT> MeshType ;
	typedef typename Base::Location Location ;

	DGConstant ( const MeshType & mesh )
		: Base( mesh )
	{}

	Index nDOF() const { return Base::mesh().nCells() ; }

	void interpolate( const Location& loc, typename Base::Interpolation& itp ) const {
		itp.coeffs[0] = 1. ;
		list_nodes( loc, itp.nodes ) ;
	}
	void get_derivatives( const Location&, typename Base::Derivatives& dc_dx ) const {
		dc_dx.setZero() ;
	}

	void locate_dof( typename Base::Location& loc, Index ) const {
		typename MeshType::CellGeo geo ;
		Base::mesh().get_geo( loc.cell, geo ) ;
		QuadraturePoints< typename MeshType::CellGeo, 1>::get( geo, 0, loc.coords ) ;
	}

	using Base::list_nodes ;
	void list_nodes( const Location& loc, typename Base::NodeList& list ) const {
		list[0] = Base::mesh().cellIndex( loc.cell ) ;
	}

	void interpolate_tpz( const Location& loc, typename Base::Interpolation& itp ) const
	{ interpolate(loc,itp) ; }

	Scalar dof_volume_fraction( Index ) const	{ return 1. ; }

} ;


}

#endif
