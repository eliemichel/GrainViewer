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

#include "Phase.hh"
#include "PhaseMeshes.hh"

#include "geo/instanciations.hh"

#include "simu/DynParticles.hh"

#include "geo/FieldBase.impl.hh" //interpolate

namespace d6 {

Phase::Phase( const PhaseMeshes & meshes, const Phase& src )
	: fraction(src.fraction.interpolate<PrimalShape>(meshes.primal())),
	  velocity(src.velocity.interpolate<PrimalShape>(meshes.primal())),
	  stresses(src.stresses.interpolate<  DualShape>(meshes.  dual())),
	  sym_grad(src.sym_grad.interpolate<  DualShape>(meshes.  dual())),
	  spi_grad(src.spi_grad.interpolate<  DualShape>(meshes.  dual())),
	  grad_phi(src.grad_phi.interpolate<PrimalShape>(meshes.primal())),
	  geo_proj(src.geo_proj.interpolate<PrimalShape>(meshes.primal())),
	  fcontact(src.fcontact.interpolate<PrimalShape>(meshes.primal()))

{
}

template <typename MeshT>
static bool adaptPrimal( const DynParticles &, const Phase &, MeshT& )
{ return false ; }

template <typename MeshT>
static bool adaptDual( const DynParticles &, const Phase &, MeshT& )
{ return false ; }

template <>
bool adaptPrimal( const DynParticles &, const Phase &grains, Octree& primal )
{
	(void) grains ;

	bool adapted = false ;

	Octree::Location loc ;
	primal.locate( Vec::Ones(), loc ) ;

	adapted = primal.split( loc.cell ) ;

	if( adapted ) {
		primal.rebuild() ;
	}

	return adapted ;
}

template <>
bool adaptDual( const DynParticles &particles, const Phase &, UnstructuredDOFs& dual )
{
	dual.resize( particles.count() ) ;

	return true ;
}

template <typename PMeshT, typename DMeshT>
void AbstractPhaseMeshes<PMeshT, DMeshT>::adapt( const DynParticles& particles, std::unique_ptr< Phase >& grains )
{
	std::unique_ptr< PrimalMesh > newPrimal( new PrimalMesh( primal() ) ) ;

	if( adaptPrimal( particles, *grains, *newPrimal ) ) {
		m_primal.swap( newPrimal );
		grains.reset( new Phase( *this, *grains ) );
	}


	if( adaptDual( particles, *grains, *m_dual ) ) {

		particles.events().replay( grains->stresses ) ;

		grains->stresses.fit_shape() ;
		grains->sym_grad.fit_shape() ;
		grains->spi_grad.fit_shape() ;
	}

}

template <typename PMeshT>
void AbstractPhaseMeshes<PMeshT, PMeshT>::adapt( const DynParticles& particles, std::unique_ptr< Phase >& grains )
{
	std::unique_ptr< PrimalMesh > newPrimal( new PrimalMesh( primal() ) ) ;

	if( adaptPrimal( particles, *grains, *newPrimal ) ) {
		m_primal.swap( newPrimal );
		grains.reset( new Phase( *this, *grains ) );
	}

}


template struct AbstractPhaseMeshes<PrimalMesh, DualMesh> ;

}
