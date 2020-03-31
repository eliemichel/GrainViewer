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

#include "ActiveIndices.hh"

#include "geo/ScalarField.hh"
#include "geo/VectorField.hh"
#include "geo/TensorField.hh"

#include "geo/MeshImpl.hh"

namespace d6 {

const Index Active::s_Inactive = -1 ;

void Active::computeRevIndices()
{
	revIndices.resize( nNodes );

#pragma omp parallel for
	for( int i = 0 ; i < indices.size() ; ++i  ) {
		const Index idx = indices[ i ] ;
		if( idx != Active::s_Inactive ) {
			revIndices[idx-offset] = static_cast<Index>(i) ;
		}
	}
}

void Active::setOffset(const Index o)
{
	offset = o ;

#pragma omp parallel for
	for( int i = 0 ; i < indices.size() ; ++i  ) {
		if( indices[i] != Active::s_Inactive )
			indices[i] += o ;
	}
}

} //d6
