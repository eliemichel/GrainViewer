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

#ifndef D6_EXPR_HH
#define D6_EXPR_HH

#include "utils/alg.hh"
#include "geo.fwd.hh"

namespace d6 {

template< typename ValueType >
struct Expr {
	virtual ValueType operator() ( const Vec&  x ) const = 0 ;
};

typedef Expr< Scalar > ScalarExpr ;

template < typename Functor, typename ValueType = typename std::result_of< Functor( const Vec& ) >::type >
struct LambdaExpr : public Expr< ValueType > {
	const Functor& expr ;

	explicit LambdaExpr( const Functor& xpr ) : expr(xpr) {}
	ValueType operator() ( const Vec&  x ) const  { return expr(x) ; }
};

template < typename Functor >
LambdaExpr< Functor > make_expr( const Functor& expr )
{
	return LambdaExpr< Functor >{ expr } ;
}


} //d6

#endif
