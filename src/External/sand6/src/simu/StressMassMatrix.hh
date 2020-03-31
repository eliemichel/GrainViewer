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

#ifndef D6_STRESS_MASS_MATRIX_HH
#define D6_STRESS_MASS_MATRIX_HH

#include "geo/MeshShapeFunction.hh"

#include "utils/block_mat.hh"

namespace d6 {

struct Active ;

template< typename Shape >
struct AbstractStressMassMatrix
{
	static constexpr bool use_identity = true ;
	typename FormMat<SD, SD>::SymType  inv_sqrt ;

	void compute( const Shape& shape , const Active& nodes, const Index totNodes )  ;
};

template < typename MeshT>
struct AbstractStressMassMatrix<DGLinear<MeshT>>
{
	typedef DGLinear<MeshT> Shape ;

	static constexpr bool use_identity = false ;
	typename FormMat<SD, SD>::SymType  inv_sqrt ;

	void compute( const Shape& shape, const Active& nodes, const Index totNodes )  ;

} ;

} //d6

#endif
