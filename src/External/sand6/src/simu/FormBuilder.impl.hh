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

#ifndef D6_FORM_BUILDER_IMPL_HH
#define D6_FORM_BUILDER_IMPL_HH

#include "FormBuilder.hh"

#include "ActiveIndices.hh"

#include "geo/MeshImpl.hh"
#include "geo/Particles.hh"

namespace d6 {

namespace form {

	template< typename LhsShape, typename RhsShape>
	struct IntSide {
		static constexpr Side side = Side::Right ;
	} ;
	template< typename MeshT >
	struct IntSide< UnstructuredShapeFunc, P2<MeshT> > {
		static constexpr Side side = Side::Left ;
	} ;
	template< typename MeshT, typename RhsShape >
	struct IntSide< P2<MeshT>, RhsShape > {
		static constexpr Side side = Side::Left ;
	} ;
	template< typename RhsShape >
	struct IntSide< UnstructuredShapeFunc, RhsShape > {
		static constexpr Side side = Side::Left ;
	} ;

	template< typename LhsShape, typename RhsShape, Side side_ = IntSide<LhsShape, RhsShape>::side >
	struct GetSide {
		static constexpr Side side = side_ ;

		typedef LhsShape Shape ;
		typedef RhsShape Other ;

		static Shape& shape( FormBuilder<LhsShape, RhsShape> &b ) { return b.m_lhsShape ; }
		static Other& other( FormBuilder<LhsShape, RhsShape> &b ) { return b.m_rhsShape ; }
		static const Shape& shape( const FormBuilder<LhsShape, RhsShape> &b ) { return b.m_lhsShape ; }
		static const Other& other( const FormBuilder<LhsShape, RhsShape> &b ) { return b.m_rhsShape ; }

		template< typename T1, typename T2 >
		static T1& lhs( T1& l, T2& ) { return l ; }
		template< typename T1, typename T2 >
		static T2& rhs( T1&, T2& r ) { return r ; }
		template< typename T1, typename T2 >
		static const T1& lhs( const T1& l, const T2& ) { return l ; }
		template< typename T1, typename T2 >
		static const T2& rhs( const T1&, const T2& r ) { return r ; }
	};
	template< typename LhsShape, typename RhsShape>
	struct GetSide<LhsShape, RhsShape, Right> {
		static constexpr Side side = Side::Right ;

		typedef RhsShape Shape ;
		typedef LhsShape Other ;

		static Shape& shape( FormBuilder<LhsShape, RhsShape> &b ) { return b.m_rhsShape ; }
		static Other& other( FormBuilder<LhsShape, RhsShape> &b ) { return b.m_lhsShape ; }
		static const Shape& shape( const FormBuilder<LhsShape, RhsShape> &b ) { return b.m_rhsShape ; }
		static const Other& other( const FormBuilder<LhsShape, RhsShape> &b ) { return b.m_lhsShape ; }

		template< typename T1, typename T2 >
		static T2& lhs( T1&, T2& l ) { return l ; }
		template< typename T1, typename T2 >
		static T1& rhs( T1& r, T2& ) { return r ; }
		template< typename T1, typename T2 >
		static const T2& lhs( const T1&, const T2& l ) { return l ; }
		template< typename T1, typename T2 >
		static const T1& rhs( const T1& r, const T2& ) { return r ; }
	};

} //form

template < typename LhsShape, typename RhsShape >
template < form::Side side, typename QPIterator, typename Func >
void FormBuilder<LhsShape, RhsShape>::addToIndexIf(
		const QPIterator& qpBegin, const QPIterator& qpEnd,
		Indices rowIndices, Indices colIndices,	const Func &f )
{
	typedef form::GetSide<LhsShape, RhsShape, side > Get ;
	const typename Get::Shape& shape = Get::shape(*this) ;
	const typename Get::Other& other = Get::other(*this) ;

	typename Get::Shape::Location shapeLoc ;
	typename Get::Other::Location otherLoc ;
	typename Get::Shape::NodeList shapeNodes ;
	typename Get::Other::NodeList otherNodes ;

	for( auto qpIt = qpBegin ; qpIt != qpEnd ; ++qpIt ) {
		const Vec& pos = qpIt.pos() ;
		if( !f(pos) ) continue ;

		 qpIt.locate( shapeLoc );
		other.locate( pos, otherLoc );

		shape.list_nodes( shapeLoc, shapeNodes );
		other.list_nodes( otherLoc, otherNodes );

		for( int k = 0 ; k < shapeNodes.rows() ; ++ k ) {
			for( int j = 0 ; j < otherNodes.rows() ; ++ j ) {
				const Index col = colIndices[ Get::rhs(shapeNodes[k], otherNodes[j]) ] ;
				const Index row = rowIndices[ Get::lhs(shapeNodes[k], otherNodes[j]) ] ;
				if( row != Active::s_Inactive && col != Active::s_Inactive )
					m_data[ row ].push_back( col ) ;
			}
		}
	}

}

template < typename LhsShape, typename RhsShape >
template < typename Func>
void FormBuilder<LhsShape, RhsShape>::addToIndexIf(
		Indices rowIndices,	Indices colIndices,	const Func &f )
{

	typedef form::GetSide<LhsShape, RhsShape > Get ;
	const typename Get::Shape& shape = Get::shape(*this) ;
	addToIndexIf< Get::side >( shape.qpBegin(), shape.qpEnd(), rowIndices, colIndices, f ) ;
}

template < typename LhsShape, typename RhsShape >
template < typename CellIterator >
void FormBuilder<LhsShape, RhsShape>::addToIndex(
		const CellIterator& cellBegin, const CellIterator& cellEnd,
		Indices rowIndices,	Indices colIndices	)
{
	typedef form::GetSide<LhsShape, RhsShape > Get ;
	const typename Get::Shape& shape = Get::shape(*this) ;
	addToIndexIf< Get::side >( shape.qpIterator(cellBegin), shape.qpIterator(cellEnd), rowIndices, colIndices,
				  [](const Vec&){ return true ; } ) ;
}

template < typename LhsShape, typename RhsShape >
template < form::Side side, typename QPIterator, typename Func >
void FormBuilder<LhsShape, RhsShape>::integrate_qp( const QPIterator& qpBegin, const QPIterator& qpEnd, Func func ) const
{
	typedef form::GetSide<LhsShape, RhsShape, side > Get ;
	const typename Get::Other& other = Get::other(*this) ;

	typename LhsShape::Location      lhs_loc ;
	typename LhsShape::Interpolation lhs_itp ;
	typename LhsShape::Derivatives   lhs_dc_dx ;
	typename RhsShape::Location      rhs_loc ;
	typename RhsShape::Interpolation rhs_itp ;
	typename RhsShape::Derivatives   rhs_dc_dx ;


	for ( auto qpIt = qpBegin ; qpIt != qpEnd ; ++qpIt )
	{
		qpIt.locate(Get::lhs( lhs_loc, rhs_loc )) ;
		const Vec &pos = qpIt.pos() ;
		other.locate( pos, Get::rhs( lhs_loc, rhs_loc ) ) ;

		m_lhsShape.interpolate    ( lhs_loc, lhs_itp );
		m_rhsShape.interpolate    ( rhs_loc, rhs_itp );
		m_lhsShape.get_derivatives( lhs_loc, lhs_dc_dx );
		m_rhsShape.get_derivatives( rhs_loc, rhs_dc_dx );

		func( qpIt.weight(), qpIt.pos(), lhs_itp, lhs_dc_dx, rhs_itp, rhs_dc_dx ) ;
	}
}


template < typename LhsShape, typename RhsShape >
template < form::Side side, typename CellIterator, typename Func >
void FormBuilder<LhsShape, RhsShape>::integrate_cell( const CellIterator& cellBegin, const CellIterator& cellEnd, Func func ) const
{
	typedef form::GetSide<LhsShape, RhsShape, side > Get ;
	const typename Get::Shape& shape = Get::shape(*this) ;
	integrate_qp<side>( shape.qpIterator(cellBegin), shape.qpIterator(cellEnd), func ) ;
}

template < typename LhsShape, typename RhsShape >
template < typename Func >
void FormBuilder<LhsShape, RhsShape>::integrate_qp( Func func ) const
{
	typedef form::GetSide<LhsShape, RhsShape > Get ;
	const typename Get::Shape& shape = Get::shape(*this) ;
	integrate_qp<Get::side>( shape.qpBegin(), shape.qpEnd(), func ) ;
}


template < typename LhsShape, typename RhsShape >
template < typename CellIterator, typename Func >
void FormBuilder<LhsShape, RhsShape>::integrate_node( const CellIterator& cellBegin, const CellIterator& cellEnd, Func func ) const
{
	static_assert( std::is_same<typename LhsShape::MeshType, typename RhsShape::MeshType >::value,
				   "integrate_node only works with shapes based on the same mesh" ) ;
	assert( &m_lhsShape.mesh() == &m_rhsShape.mesh() ) ;

	typedef typename LhsShape::MeshType MeshType;
	typename LhsShape::MeshType::Location loc ;
	typename LhsShape::Interpolation lhs_itp ;
	typename RhsShape::Interpolation rhs_itp ;

	typename MeshType::CellGeo cellGeo ;

	for( CellIterator it = cellBegin ; it !=  cellEnd ; ++it )
	{
		const typename MeshType::Cell& cell = *it ;
		loc.cell = cell ;

		m_lhsShape.mesh().get_geo( cell, cellGeo );
		const Scalar w = cellGeo.volume() / LhsShape::NI ;

		for ( int k = 0 ; k < LhsShape::NI ; ++k ) {

			m_lhsShape.locate_dof(loc, k) ;
			const Vec &pos = cellGeo.pos( loc.coords ) ;

			m_lhsShape.interpolate ( loc, lhs_itp );
			m_rhsShape.interpolate ( loc, rhs_itp );

			func( w, pos, lhs_itp, rhs_itp ) ;

		}
	}
}


template < typename LhsShape, typename RhsShape >
template < typename Func >
void FormBuilder<LhsShape, RhsShape>::integrate_particle( const Particles& particles, Func func ) const
{
	const size_t n = particles.count() ;

	typename LhsShape::Location      lhs_loc ;
	typename LhsShape::Interpolation lhs_itp ;
	typename LhsShape::Derivatives   lhs_dc_dx ;
	typename RhsShape::Location      rhs_loc ;
	typename RhsShape::Interpolation rhs_itp ;
	typename RhsShape::Derivatives   rhs_dc_dx ;

	for ( size_t i = 0 ; i < n ; ++i ) {
		const Vec& pos = particles.centers().col(i) ;

		m_lhsShape.locate_by_pos_or_id( pos, i, lhs_loc );
		m_lhsShape.interpolate( lhs_loc, lhs_itp );
		m_lhsShape.get_derivatives( lhs_loc, lhs_dc_dx );
		m_rhsShape.locate_by_pos_or_id( pos, i, rhs_loc );
		m_rhsShape.interpolate( rhs_loc, rhs_itp );
		m_rhsShape.get_derivatives( rhs_loc, rhs_dc_dx );

		func( i, particles.volumes()[i], lhs_itp, lhs_dc_dx, rhs_itp, rhs_dc_dx ) ;
	}

}



} //d6

#endif
