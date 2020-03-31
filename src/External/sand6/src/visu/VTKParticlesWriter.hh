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

#ifndef D6_VTK_PARTICLES_WRITER_HH
#define D6_VTK_PARTICLES_WRITER_HH

#include "VTKWriter.hh"
#include "utils/alg.hh"
#include "geo/geo.fwd.hh"

namespace d6 {

class Particles ;

class VTKParticlesWriter : public VTKWriter
{

public:
	enum Quantity {
		Volumes,
		Velocities,
		Frames,
		Orientations
	} ;

	VTKParticlesWriter( const char* base_dir, const Particles& particles ) ;

	template< typename Derived >
	bool dump( const char* name, const Eigen::MatrixBase< Derived > &data,
			   Index dim = Derived::RowsAtCompileTime ) ;

	template< typename Derived >
	bool dump( const char* name, const FieldBase< Derived >& field ) ;

	bool dump( Quantity quantity ) ;
	bool dump_all( ) ;

protected:
	void writeMesh( File& file ) const ;
	size_t nDataPoints( ) const ;

private:

	const Particles& m_particles ;

};

} //d6

#endif
