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

#include "LinearSolver.hh"

#include "FormBuilder.hh"

#include "utils/Log.hh"

#include <bogus/Core/Block.impl.hpp>
#include <bogus/Core/BlockSolvers.impl.hpp>

//#define USE_EIGEN
#ifdef USE_EIGEN
#include <bogus/Core/Block.io.hpp>
#include <Eigen/SparseCholesky>
#include <Eigen/Cholesky>
#endif

namespace d6 {

template < typename Derived, typename OtherDerived >
Scalar solveSDP( const bogus::SparseBlockMatrixBase< Derived >& M,
						  const bogus::SparseBlockMatrixBase< OtherDerived >& P,
						  const DynVec &rhs,
						  DynVec &res  )
{

#ifdef USE_EIGEN
	(void) P ;

	typedef Eigen::SparseMatrix< Scalar > SM ;
	SM csr ;
	bogus::convert( M, csr ) ;

	csr.prune(1.) ;

	Eigen::SimplicialLDLT< SM > ldlt( csr ) ;
	std::cerr << "DLLD " << ldlt.info() << std::endl ;
	res = ldlt.solve( rhs ) ;

	return 0 ;
#else
	typedef bogus::MatrixPreconditioner<  OtherDerived                   > Precond ;

	Scalar cgres = 0. ;

	bogus::Krylov< Derived,	Precond::template Type > cg ( M.derived() ) ;
	cg.preconditioner().setPreconditionerMatrix( P.derived() ) ;

	cgres = cg.asCG().solve( rhs, res ) ;
	Log::Debug() << "CG res: " << cgres << std::endl ;


	return cgres ;
#endif
}

template Scalar solveSDP( const bogus::SparseBlockMatrixBase< typename FormMat<WD,WD>::Type >& ,
const bogus::SparseBlockMatrixBase< typename FormMat<WD,WD>::SymType >&,
const DynVec &,
DynVec &) ;

} //d6
