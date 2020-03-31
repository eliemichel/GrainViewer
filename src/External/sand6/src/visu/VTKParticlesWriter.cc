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

#include "VTKParticlesWriter.hh"

#include "geo/Particles.hh"

#include "geo/ScalarField.hh"
#include "geo/VectorField.hh"
#include "geo/TensorField.hh"
#include "geo/UnstructuredShapeFunction.hh"

#include "utils/File.hh"
#include "utils/Log.hh"

namespace d6 {

VTKParticlesWriter::VTKParticlesWriter(const char *base_dir, const Particles &particles)
	: VTKWriter( base_dir ), m_particles(particles)
{

}

void VTKParticlesWriter::writeMesh( File &vtk ) const
{
	vtk << "DATASET POLYDATA\n" ;
	vtk << "POINTS " << m_particles.count() << " float\n" ;
	write( vtk, m_particles.centers().data(), WD, m_particles.count() ) ;
}

size_t VTKParticlesWriter::nDataPoints() const {
	return m_particles.count() ;
}

template< typename Derived >
bool VTKParticlesWriter::dump( const char *name, const Eigen::MatrixBase< Derived > &data, Index dim )
{
	if( !m_file.is_open() ) {
		Log::Error() << " VTKParticlesWriter: should call startFile() before dumping data " << std::endl ;
		return false ;
	}

	writeAttribute( name, data.derived().data(), dim ) ;

	return true ;
}

template< typename Derived >
bool VTKParticlesWriter::dump( const char* name, const FieldBase< Derived >& field )
{

	return dump( name, field.flatten(), FieldBase< Derived >::D ) ;
}

bool VTKParticlesWriter::dump_all( )
{
	return
			dump( Volumes ) &&
			dump( Velocities ) &&
			dump( Frames ) &&
			dump( Orientations ) ;

	return true ;
}

bool VTKParticlesWriter::dump( Quantity quantity) {
	switch( quantity ) {
	case Volumes:
		return dump( "volumes", m_particles.volumes() ) ;
	case Velocities:
		return dump( "velocities", m_particles.velocities() ) ;
	case Frames:
		return dump( "frames", m_particles.frames() ) ;
	case Orientations:
		return dump( "orient", m_particles.orient() ) ;
	}
	return false ;
}

template  bool VTKParticlesWriter::dump( const char*, const FieldBase< AbstractScalarField<UnstructuredShapeFunc> >& ) ;
template  bool VTKParticlesWriter::dump( const char*, const FieldBase< AbstractVectorField<UnstructuredShapeFunc> >& ) ;
template  bool VTKParticlesWriter::dump( const char*, const FieldBase< AbstractTensorField<UnstructuredShapeFunc> >& ) ;

}
