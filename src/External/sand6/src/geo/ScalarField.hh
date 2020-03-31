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

#ifndef D6_SCALAR_FIELD_HH
#define D6_SCALAR_FIELD_HH

#include "FieldBase.hh"

namespace d6 {

template < typename ShapeFuncT >
struct FieldTraits< AbstractScalarField< ShapeFuncT > >  {

	typedef ShapeFuncT ShapeFuncImpl ;
	typedef Scalar ValueType ;
	static constexpr Index Dimension = 1 ;
};

template < typename ShapeFuncT >
class AbstractScalarField : public FieldBase< AbstractScalarField< ShapeFuncT > >
{
	typedef FieldBase< AbstractScalarField > Base ;

public:
	typedef ShapeFuncBase< ShapeFuncT > ShapeFuncType ;
	typedef typename ShapeFuncType::Location Location ;

	D6_MAKE_FIELD_CONSTRUCTORS_AND_ASSIGNMENT_OPERATORS( AbstractScalarField )

	Vec grad_at( const typename ShapeFuncType::Location& loc ) const ;

};


} //d6

#endif
