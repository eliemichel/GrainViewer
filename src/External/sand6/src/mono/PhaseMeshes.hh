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

#ifndef D6_PHASE_MESHES_HH
#define D6_PHASE_MESHES_HH

#include "PhaseFields.hh"

namespace d6 {

class  DynParticles ;
struct Phase ;
struct Config ;

template <typename PMeshT, typename DMeshT>
struct AbstractPhaseMeshes
{
    std::unique_ptr<PrimalMesh> m_primal ;
    std::unique_ptr<  DualMesh> m_dual  ;

    const PrimalMesh& primal() const { return *m_primal ; }
    const   DualMesh&   dual() const { return   *m_dual ; }

    AbstractPhaseMeshes( std::unique_ptr<PrimalMesh> pMesh, std::unique_ptr<  DualMesh> dMesh )
        : m_primal( std::move(pMesh)),
          m_dual  ( std::move(dMesh))
    {}

    void adapt( const DynParticles& particles, std::unique_ptr< Phase >& phase ) ;

    template <typename Ar>
    void serialize( Ar &ar, unsigned int ) {
        ar & (*m_primal) ;
        ar & (*m_dual) ;
    }
};

template <typename PMeshT>
struct AbstractPhaseMeshes<PMeshT, PMeshT> {
    std::unique_ptr<PrimalMesh> m_primal ;

    AbstractPhaseMeshes( std::unique_ptr<PrimalMesh> pMesh, std::unique_ptr<  DualMesh>  )
        : m_primal( std::move( pMesh ) )
    {}

    const PrimalMesh& primal() const { return *m_primal ; }
    const   DualMesh&   dual() const { return *m_primal ; }

    void adapt( const DynParticles& particles, std::unique_ptr< Phase >& phase ) ;

    template <typename Ar>
    void serialize( Ar &ar, unsigned int ) {
        ar & (*m_primal) ;
    }
};

typedef AbstractPhaseMeshes< PrimalMesh, DualMesh > PhaseMeshes ;


} //d6

#endif
