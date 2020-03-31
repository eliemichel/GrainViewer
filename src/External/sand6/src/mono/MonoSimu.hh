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

#ifndef D6_MONO_SIMU_HH
#define D6_MONO_SIMU_HH

#include "simu/Simu.hh"

#include "PhaseMeshes.hh"
#include "PhaseSolver.hh"

namespace d6 {

class MonoSimu : public Simu
{

public:

	MonoSimu( const Config& config, const char* base_dir ) ;
	virtual ~MonoSimu() ;

	const PhaseMeshes& meshes() const { return m_meshes ;  }

protected:

	virtual void update_fields ( const Scalar dt ) override ;
	virtual void move_particles( const Scalar dt ) override ;

	virtual void adapt_meshes() override ;

	// Output
	virtual void dump_fields(unsigned frame) const override ;

private:

	PhaseMeshes  m_meshes ;
	std::unique_ptr<Phase>     m_grains ;
	// Useful for warm-starting stresses at frictional boundary conditions
	std::vector< RBStresses > m_rbStresses  ;

	PhaseSolver m_solver ;

};


} //d6


#endif
