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

#ifndef D6_PRIMAL_DATA_HH
#define D6_PRIMAL_DATA_HH

#include "utils/block_mat.hh"

namespace  d6 {

/*!
 *  Problem defined as
 *  v = H' r
 *  v_rb[j] = inv_im[j] jac[j]' r   \forall j
 *  u = H  v + w + \sum_j{ jac[j] * v_rb[j] }
 *
 *  (u_i, r_i) \in C(mu[i])
 */
struct PrimalData {

	enum MassMatrixMode {
		Lumped,
		General
	} ;

	typedef typename FormMat<SD,WD>::Type HType ;
	typedef typename FormMat<SD,SD>::Type JacobianType ;
	typedef typename FormMat<SD,SD>::SymType InvInertiaType ;

	HType H   ;
	DynVec w  ;

	DynVec mu ;

	std::vector< JacobianType > jacobians ;
	std::vector< InvInertiaType > inv_inertia_matrices ;

	// Mass matrix

	typedef typename FormMat<WD,WD>::SymType MType ;
	MassMatrixMode mass_matrix_mode ;

	// For massMatrixMode != Lumped
	MType  M ;
	DynVec f ;


	Index n() const { return H.rowsOfBlocks() ; }

	template <typename Archive>
	void serialize(Archive &ar, const unsigned int ) ;

	bool load( const char * file ) ;
	bool dump( const char * file ) const ;
};


} //d6

#endif
