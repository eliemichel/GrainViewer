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

#ifndef D6_RIGID_BODY_DATA_HH
#define D6_RIGID_BODY_DATA_HH

#include "PhaseFields.hh"
#include "simu/ActiveIndices.hh"

#include "geo/BoundaryInfo.hh"

#include "utils/block_mat.hh"

namespace d6 {

class RigidBody ;

struct RigidBodyData
{
	typedef RBStresses TensorField ;

	RigidBodyData( RigidBody& rb_, TensorField &s ) ;

	//! Volume fraction taken by the rigid-body at a given point
	Scalar    phi( const Vec &x ) const ;
	//! Gradient of the volume fraction taken by the rigid-body at a given point
	void grad_phi( const Vec &x, Vec &grad ) const ;

	//! Computes nodes that are influenced by the rigid-body
	void compute_active( const Active& phaseNodes ) ;
	//! Assembles projection and jacobian matrices
	void assemble_matrices(const PrimalShape &primalShape, const DualShape &dualShape,
						   const Active &primalNodes, const Active& dualNodes,
						   Index totNodes ) ;

	RigidBody&   rb ;
	TensorField& stresses ;

	Active	    nodes ;

	FormMat<SD,WD>::Type	jacobian ;     //!< int( (u grad phi):tau )
	FormMat<SD,WD>::Type	projection ;   //!< Linear operator giving rb velocities at mesh nodes

	//! Integrate volume fraction at dual nodes
	DynArr intFraction ;
private:
	static const Scalar s_splatRad ;

	void integrate( const PrimalShape& primalShape, const DualShape& dualShape,
					const Active &primalNodes, const Active& dualNodes,
					Index totNodes  ) ;

};

} //d6

#endif
