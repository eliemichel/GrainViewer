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

#ifndef D6_PHASE_DESCR_HH
#define D6_PHASE_DESCR_HH

#include "mono/PhaseMeshes.hh"

#include "geo/ScalarField.hh"
#include "geo/VectorField.hh"
#include "geo/TensorField.hh"

#include "geo/MeshImpl.hh"

namespace d6 {

struct Phase
{


	PrimalScalarField fraction ;
	PrimalVectorField velocity ;

	DualTensorField stresses ;
	DualTensorField sym_grad ;
	DualSkewTsField spi_grad ;

	PrimalVectorField grad_phi ;
	PrimalVectorField geo_proj ;
	PrimalVectorField fcontact ;

	Phase( const PhaseMeshes & meshes )
		: fraction(meshes.primal()), velocity(meshes.primal()),
		  stresses(meshes.  dual()), sym_grad(meshes.  dual()),
		  spi_grad(meshes.  dual()), grad_phi(meshes.primal()),
		  geo_proj(meshes.primal()), fcontact(meshes.primal()),
		  m_serializeAllFields( true )
	{}

	Phase( const PhaseMeshes & meshes, const Phase& src ) ;

	void serializeAllFields( bool doSerialize )
	{ m_serializeAllFields = doSerialize ; }

	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) {
		ar & m_serializeAllFields ;
		ar & fraction ;
		ar & velocity ;
		ar & grad_phi ;

		if (m_serializeAllFields) {
			ar & stresses ;
			ar & sym_grad ;
			ar & spi_grad ;
			ar & fcontact ;
			ar & geo_proj ;
		}
	}

private:
	bool m_serializeAllFields ;
};

} //d6

#endif
