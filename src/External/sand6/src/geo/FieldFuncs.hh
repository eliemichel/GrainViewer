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

#ifndef D6_FIELD_FUNCS_IMPL_HPP
#define D6_FIELD_FUNCS_IMPL_HPP

#include "FieldFuncBase.hh"

namespace d6 {

template< typename Derived >
struct FuncTraits {
};

template < typename Derived, Index D,
		   typename ShapeFuncT = typename FieldTraits< typename FuncTraits<Derived>::FieldType >::ShapeFuncImpl >
struct UnaryFieldFunc : public FieldFuncBase< Derived, D, ShapeFuncT >
{
	typedef FieldFuncBase< Derived, D, ShapeFuncT > Base ;
	typedef FuncTraits< Derived > Traits;
	typedef typename Traits::FieldType FieldType;

	explicit UnaryFieldFunc( const FieldType & field )
		: Base( field.shape() ), m_field(field)
	{}

	Index size( ) const {
		return m_field.size() ;
	}

protected:
	const FieldType& m_field  ;
};

// Trace

template <typename ShapeFuncT> struct FieldTrace ;

template< typename ShapeFuncT >
struct FuncTraits< FieldTrace< ShapeFuncT > > {
	typedef AbstractTensorField< ShapeFuncT > FieldType ;
};

template <typename ShapeFuncT>
struct FieldTrace : public UnaryFieldFunc<FieldTrace<ShapeFuncT>, 1>
{
	typedef UnaryFieldFunc<FieldTrace<ShapeFuncT>, 1> Base ;
	typedef typename Base::FieldType FieldType ;

	using Base::m_field ;
	explicit FieldTrace( const FieldType & field ) : Base(field) {}

	template < typename Agg >
	void eval_at_node( Index i, typename Segmenter<1, Agg>::Seg v ) const
	{
		v = m_field[i][0] ;
	}

} ;

// Deviatoric part

template <typename ShapeFuncT> struct DeviatoricPart ;

template< typename ShapeFuncT >
struct FuncTraits< DeviatoricPart< ShapeFuncT > > {
	typedef AbstractTensorField< ShapeFuncT > FieldType ;
};

template <typename ShapeFuncT>
struct DeviatoricPart : public UnaryFieldFunc<DeviatoricPart<ShapeFuncT>, SD>
{
	typedef UnaryFieldFunc<DeviatoricPart<ShapeFuncT>, SD> Base ;
	typedef typename Base::FieldType FieldType ;

	using Base::m_field ;
	explicit DeviatoricPart( const FieldType & field ) : Base(field) {}

	template < typename Agg >
	void eval_at_node( Index i, typename Segmenter<SD, Agg>::Seg v ) const
	{
		v = m_field[i] ;
		v[0] = 0 ;
	}

} ;

// Norm

template <template <typename> class Field, typename ShapeFuncT >
struct FieldNorm ;

template <template <typename> class Field, typename ShapeFuncT >
struct FuncTraits< FieldNorm< Field, ShapeFuncT > > {
	typedef Field< ShapeFuncT > FieldType ;
};

template <template <typename> class Field, typename ShapeFuncT >
struct FieldNorm : public UnaryFieldFunc< FieldNorm<Field, ShapeFuncT>, 1>
{
	typedef UnaryFieldFunc<FieldNorm<Field, ShapeFuncT>, 1> Base ;
	typedef typename Base::FieldType FieldType ;

	using Base::m_field ;
	explicit FieldNorm( const FieldType & field ) : Base(field) {}

	template < typename Agg >
	void eval_at_node( Index i, typename Segmenter<1, Agg>::Seg v ) const
	{
		v = m_field[i].norm() ;
	}

} ;


} //d6

#endif
