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

#ifndef D6_TENSOR_FIELD_HH
#define D6_TENSOR_FIELD_HH

#include "FieldBase.hh"
#include "FieldFuncs.hh"

namespace d6 {

// Symmetric tensor

template < typename ShapeFuncT >
struct FieldTraits< AbstractTensorField< ShapeFuncT > >  {

	typedef ShapeFuncT  ShapeFuncImpl ;
	typedef VecS ValueType ;
	static constexpr Index Dimension = SD ;
};

template < typename ShapeFuncT >
class AbstractTensorField : public FieldBase< AbstractTensorField< ShapeFuncT > >
{
	typedef FieldBase< AbstractTensorField > Base ;

public:
	typedef ShapeFuncBase< ShapeFuncT > ShapeFuncType ;
	typedef typename ShapeFuncType::Location Location ;

	D6_MAKE_FIELD_CONSTRUCTORS_AND_ASSIGNMENT_OPERATORS( AbstractTensorField )

	DeviatoricPart< ShapeFuncT > deviatoricPart() const {
		return DeviatoricPart< ShapeFuncT >( *this ) ;
	}
	FieldTrace< ShapeFuncT > trace() const {
		return FieldTrace< ShapeFuncT >( *this ) ;
	}
	FieldNorm< d6::AbstractTensorField, ShapeFuncT > norm() const {
		return FieldNorm< d6::AbstractTensorField, ShapeFuncT >( *this ) ;
	}

	void add_sym_tensor( const Location& x, Mat& tensor ) const ;
	void get_sym_tensor( const Location& x, Mat& tensor ) const ;

};

// Skew symmetric tensor

template < typename ShapeFuncT >
struct FieldTraits< AbstractSkewTsField< ShapeFuncT > >  {

	typedef ShapeFuncT  ShapeFuncImpl ;
	typedef VecR ValueType ;
	static constexpr Index Dimension = RD ;
};

template < typename ShapeFuncT >
class AbstractSkewTsField : public FieldBase< AbstractSkewTsField< ShapeFuncT > >
{
	typedef FieldBase< AbstractSkewTsField > Base ;

public:
	typedef ShapeFuncBase< ShapeFuncT > ShapeFuncType ;
	typedef typename ShapeFuncType::Location Location ;

	D6_MAKE_FIELD_CONSTRUCTORS_AND_ASSIGNMENT_OPERATORS( AbstractSkewTsField )

	void get_spi_tensor( const Location& x, Mat& tensor ) const ;
	void add_spi_tensor( const Location& x, Mat& tensor ) const ;

};


} //d6

#endif

