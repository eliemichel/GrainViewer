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

#ifndef D6_FIELD_BASE_HH
#define D6_FIELD_BASE_HH

#include "FieldFuncBase.hh"
#include "ShapeFunctionBase.hh"

namespace d6 {

template< typename Derived >
struct FieldTraits
{

};



template< typename Derived >
class FieldBase : public FieldFuncBase< Derived,
		FieldTraits< Derived >::Dimension,
		typename FieldTraits< Derived >::ShapeFuncImpl >
{

public:

	typedef FieldTraits< Derived > Traits ;

	typedef typename Traits::ShapeFuncImpl ShapeFuncImpl ;
	typedef ShapeFuncBase< ShapeFuncImpl > ShapeFuncType ;
	typedef typename ShapeFuncType::Location Location ;

	static constexpr Index D = Traits::Dimension ;

	typedef FieldFuncBase< Derived, D, ShapeFuncImpl > Base ;
	using Base::m_shape ;
	using Base::derived ;

	typedef typename Segmenter< D >::ValueType ValueType ;
	typedef typename Segmenter< D >::Seg Seg  ;
	typedef typename Segmenter< D >::ConstSeg ConstSeg  ;

	typedef AbstractScalarField< ShapeFuncImpl > ScalarField ;

	explicit FieldBase( const ShapeFuncType& shape, bool doFit = true )
		: Base( shape )
	{
		if(doFit) fit_shape() ;
	}

	void fit_shape() {
		m_size = m_shape.nDOF() ;
		m_data.resize( D * m_size );
	}
	void fit_conservative( Index n ) {
		m_size = n ;
		m_data.conservativeResize( D * m_size );
	}

	const DynVec& flatten() const { return m_data ; }
	DynVec& flatten() { return m_data ; }

	const ShapeFuncImpl& shape() const { return m_shape ; }

	//Global setters

	Derived& set_zero() ;
	Derived& set_constant( const ValueType& val ) ;

	Derived& operator= ( const FieldBase& f )
	{
		assert( f.size() == size() ) ;
		flatten() = f.flatten() ;
		return derived() ;
	}

	Derived& from_rvalue ( Derived&& f )
	{
		m_size = f.size() ;
		m_data.swap( f.m_data ) ;
		return derived() ;
	}

	template < typename Func >
	Derived& operator= ( const FieldFuncBase< Func, D, ShapeFuncImpl > & f )
	{
		assert( f.size() == size() ) ;
		#pragma omp parallel for
		for( Index  i = 0 ; i < size() ; ++i ) {
			f.template eval_at_node< DynVec >( i, segment(i) );
		}
		return derived();
	}

	// Accumulation, interpolation

	void  add_at( const Location& loc, const ValueType& val ) ;
	void  add_at( const typename ShapeFuncType::Interpolation &itp, const ValueType& val ) ;

	template < typename Func, typename OtherShape >
	typename std::enable_if<ShapeFuncType::is_mesh_based || OtherShape::is_mesh_based, Derived& >::type
	from_interpolation( const FieldFuncBase< Func, D, OtherShape > &f ) ;

	template < typename Func, typename OtherShape >
	typename std::enable_if<!ShapeFuncType::is_mesh_based && !OtherShape::is_mesh_based, Derived& >::type
	from_interpolation( const FieldFuncBase< Func, D, OtherShape > &f ) {
		//Degenerated case Unstructrued vs Unstructured -- simple copy
		return derived() = f.derived() ;
	}

	// Value at node
	Seg      segment( const Index i ) { return Segmenter< D >::segment( m_data, i) ; }
	ConstSeg segment( const Index i ) const { return Segmenter< D >::segment(m_data, i) ; }

	template < typename Agg >
	void eval_at_node( Index i, typename Segmenter<D, Agg>::Seg v ) const {	v = segment(i) ; }

	Index size() const { return m_size ; }

	// Operators

	ConstSeg  operator[] ( const Index i ) const { return segment(i) ; }
	Seg       operator[] ( const Index i )       { return segment(i) ; }

	// Info
	Scalar max_abs() const { return m_data.lpNorm< Eigen::Infinity >() ; }

	//Operations

	Derived& multiply_by( const ScalarField& field ) ;
	Derived&   divide_by( const ScalarField& field ) ;
	Derived&   divide_by_positive( const ScalarField& field, Scalar min = 1.e-16 ) ;

	// Serialization

	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) {
		ar & m_size ;
		ar & m_data ;
	}


protected:

	template < typename Func, typename OtherShape >
	typename std::enable_if< OtherShape::is_mesh_based>::type
	integrate( const FieldFuncBase< Func, D, OtherShape > &f ) ;

	template < typename Func, typename OtherShape >
	typename std::enable_if<!OtherShape::is_mesh_based >::type
	integrate( const FieldFuncBase< Func, D, OtherShape > &f ) ;

	Index m_size ;
	DynVec   m_data ;

} ;

#define D6_MAKE_FIELD_CONSTRUCTORS_AND_ASSIGNMENT_OPERATORS( FieldName ) \
	explicit FieldName( const typename Base::ShapeFuncType& shape ) \
	: Base( shape ) \
	{} \
	explicit FieldName( const typename Base::ShapeFuncType::DOFDefinition& mesh ) \
	: Base( typename Base::ShapeFuncImpl( mesh ) ) \
	{} \
	/*! Copy constructor */\
	FieldName( const FieldName& o ) : Base( o.shape() ) { Base::operator=( o ) ; } \
	FieldName& operator=( const FieldName& o ) { return Base::operator=( o ) ; } \
	/*! Move constructor */\
	FieldName( FieldName&& o ) : Base( o.shape(), false ) { Base::from_rvalue(std::move(o)) ; } \
	FieldName& operator=( FieldName&& o ) { return Base::from_rvalue(std::move(o)) ; } \
	/*! Assignment from field func */\
	template <typename Func> \
	FieldName( const FieldFuncBase< Func, Base::D, typename Base::ShapeFuncImpl > & func ) \
		: Base( func.shape() ) \
	{ Base::operator=( func ); } \
	/*! Constructor from field func */\
	template <typename Func> \
	FieldName& operator= ( const FieldFuncBase< Func, Base::D, typename Base::ShapeFuncImpl> & func ) \
	{ return Base::operator=( func ); } \
	/*! Assignment from non-trivial interpolation */\
	template < typename Func, typename OtherShape > \
	FieldName& operator= ( const NonTrivialInterpolation< Func, Base::D, OtherShape, typename Base::ShapeFuncImpl > & f ) \
	{ return Base::from_interpolation ( f.src ) ; } \
	/*! Constructors from non-trivial interpolation */\
	template < typename Func, typename OtherShape > \
	FieldName ( const NonTrivialInterpolation< Func, Base::D, OtherShape, typename Base::ShapeFuncImpl > & f ) \
	: Base( typename Base::ShapeFuncImpl( f.dest ) ) \
	{ Base::from_interpolation ( f.src ) ; } \


} //d6

#endif

