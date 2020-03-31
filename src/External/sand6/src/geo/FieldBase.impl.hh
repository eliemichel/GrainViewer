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

#ifndef D6_FIELD_BASE_IMPL_HH
#define D6_FIELD_BASE_IMPL_HH

#include "FieldBase.hh"

#include "ShapeFunctionBase.hh"
#include "ScalarField.hh"

namespace d6 {


template< typename Derived >
void FieldBase< Derived >::add_at( const Location& x, const ValueType& val )
{
	typename ShapeFuncType::Interpolation itp ;
	m_shape.interpolate( x, itp );
	add_at( itp, val ) ;
}

template< typename Derived >
void FieldBase< Derived >::add_at( const typename ShapeFuncType::Interpolation &itp, const ValueType& val )
{
	for( Index k = 0 ; k < itp.nodes.rows() ; ++k ) {
		segment( itp.nodes[k] ) += itp.coeffs[k] * val ;
	}

}

template< typename Derived >
Derived& FieldBase< Derived >::set_zero() {
	m_data.setZero() ;
	return derived() ;
}

template< typename Derived >
Derived& FieldBase< Derived >::set_constant(const ValueType &val) {
#pragma omp parallel for
	for( Index i = 0 ; i < m_size ; ++i ) {
		segment(i) = val ;
	}
	return derived() ;
}
template< typename Derived >
Derived& FieldBase< Derived >::multiply_by(const ScalarField &field) {
	mul_compwise< D >( m_data, field.flatten() ) ;
	return derived() ;
}
template< typename Derived >
Derived& FieldBase< Derived >::divide_by(const ScalarField &field) {
	div_compwise< D >( m_data, field.flatten() ) ;
	return derived() ;
}
template< typename Derived >
Derived& FieldBase< Derived >::divide_by_positive(const ScalarField &field, Scalar min) {
	div_compwise< D >( m_data, field.flatten().cwiseMax(min) ) ;
	return derived() ;
}


template< typename Derived >
template< typename Func, typename OtherShape >
typename std::enable_if< OtherShape::is_mesh_based>::type
FieldBase< Derived >::integrate( const FieldFuncBase< Func, D, OtherShape > &f )
{
	typename OtherShape::Location src_loc ;
	Location dst_loc ;

	for( auto qpIt = m_shape.qpBegin() ; qpIt != m_shape.qpEnd() ; ++qpIt ) {
		qpIt.locate( dst_loc ) ;
		f.shape().derived().locate( qpIt.pos(), src_loc ) ;
		add_at( dst_loc, qpIt.weight() * f.eval_at( src_loc ) ) ;
	}
}

template< typename Derived >
template< typename Func, typename OtherShape >
typename std::enable_if<!OtherShape::is_mesh_based>::type
FieldBase< Derived >::integrate( const FieldFuncBase< Func, D, OtherShape > &f )
{
	typename OtherShape::Location src_loc ;
	Location dst_loc ;

	for( auto qpIt = f.shape().qpBegin() ; qpIt != f.shape().qpEnd() ; ++qpIt ) {
		qpIt.locate( src_loc ) ;
		m_shape.locate( qpIt.pos(), dst_loc ) ;
		add_at( dst_loc, qpIt.weight() * f.eval_at( src_loc ) ) ;
	}

}

template< typename Derived >
template< typename Func, typename OtherShape >
#ifdef WIN32 // MSVC does not seem able to follow typdefs correctly...
typename std::enable_if< ShapeFuncBase< typename FieldTraits<Derived>::ShapeFuncImpl >::is_mesh_based || OtherShape::is_mesh_based, Derived& >::type
#else // WIN32
typename std::enable_if< FieldBase<Derived>::ShapeFuncType::is_mesh_based || OtherShape::is_mesh_based, Derived& >::type
#endif // WIN32
FieldBase< Derived >::from_interpolation( const FieldFuncBase< Func, D, OtherShape > &f )
{
	ScalarField volumes( shape() );
	shape().compute_lumped_mass( volumes.flatten() );

	set_zero() ;

	integrate( f ) ;

	// TODO : solve with consistent mass matrix ?
	divide_by_positive( volumes ) ;

	return derived();
}

} //ns d6

#endif
