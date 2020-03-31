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

#include "LCP.hh"

#include "utils/Log.hh"

#include <Eigen/Eigenvalues>
#include <bogus/Core/Block.impl.hpp>
#include <bogus/Core/BlockSolvers.impl.hpp>

namespace d6 {

LCP::LCP(const LCPData &data)
	: m_data( data )
{}

static void ackResidual( unsigned iter, Scalar res ) {
	Log::Debug() << "LCP " << iter << " =\t " << res << std::endl ;
}


Scalar LCP::solve(DynVec &x) const
{
	typedef typename bogus::SparseBlockMatrix< Scalar, bogus::SYMMETRIC > WType ;

	WType W = m_data.H * m_data.H.transpose() ;

	bogus::GaussSeidel< WType > gs( W ) ;
	gs.callback().connect( ackResidual );

	return gs.solve( bogus::LCPLaw< Scalar>(), m_data.w, x ) ;
}

} //d6
