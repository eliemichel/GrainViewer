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

#ifndef D6_BLOCK_MAT_HH
#define D6_BLOCK_MAT_HH

#include "utils/alg.hh"
#include <bogus/Core/Block.hpp>

namespace d6 {

template < Index Rows, Index Cols >
struct FormMat {
	typedef Eigen::Matrix< Scalar, Rows, Cols > BlockT ;
	typedef bogus::SparseBlockMatrix< BlockT > Type ;
	typedef bogus::SparseBlockMatrix< BlockT, bogus::UNCOMPRESSED > UncompressedType ;
	typedef bogus::SparseBlockMatrix< BlockT, bogus::SYMMETRIC > SymType ;
};

} //d6

#endif
