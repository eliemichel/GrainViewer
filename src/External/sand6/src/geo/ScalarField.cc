
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

#include "ScalarField.hh"
#include "FieldBase.impl.hh"

#include "geo/instanciations.hh"

namespace d6
{


template<typename ShapeFuncT >
Vec AbstractScalarField< ShapeFuncT >::grad_at( const typename ShapeFuncType::Location& loc ) const
{
	typename ShapeFuncType::NodeList nodes ;
	typename ShapeFuncType::Derivatives dc_dx ;

	Base::shape().list_nodes( loc, nodes );
	Base::shape().get_derivatives( loc, dc_dx ) ;

	// v(x) = sum c_k(x) v_k

	Vec grad = Vec::Zero() ;
	for( Index k = 0 ; k < nodes.rows() ; ++k ) {
		grad += dc_dx.row(k) * Base::segment( nodes[k] ) ;
	}

	return grad ;
}

#define INSTANTIATE( Shape ) \
	template class FieldBase< AbstractScalarField< Shape > > ; \
	template class AbstractScalarField< Shape > ;

EXPAND_INSTANTIATIONS
#undef INSTANTIATE

}
