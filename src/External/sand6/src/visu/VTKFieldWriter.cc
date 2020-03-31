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

#include "VTKFieldWriter.hh"

#include "geo/ScalarField.hh"
#include "geo/VectorField.hh"
#include "geo/TensorField.hh"

#include "geo/instanciations.hh"

#include "utils/File.hh"
#include "utils/Log.hh"

namespace d6 {

template <typename ShapeFuncT>
VTKFieldWriter<ShapeFuncT>::VTKFieldWriter(const char* base_dir, const ShapeFuncT &shape )
	: VTKWriter( base_dir ), m_shape( shape )
{
}

template <typename ShapeFuncT>
void VTKFieldWriter<ShapeFuncT>::writeMesh( File &vtk ) const
{
	DynMatW vertices ;
	DynMati ids ;
	m_shape.derived().build_visu_mesh( vertices, ids ) ;

	const Index NV = ids.rows() ;

	Eigen::MatrixXi nodeIndices( NV+1, ids.cols() ) ;

	nodeIndices.row(0).setConstant( NV ) ;
	nodeIndices.bottomRows(NV) = ids.cast<int>() ;

	const Index elemType =
		#if D6_DIM == 3
		NV == 4 ? /*VTK_TETRA*/ 10 : /*VTK_VOXEL*/ 11
		#else
		NV == 3 ? /*VTK_TRIANGLE*/ 5 : /*VTK_PIXEL*/ 8
		#endif
			;

	const Eigen::VectorXi cellTypes =
		Eigen::VectorXi::Constant( ids.cols(), elemType ) ;


	vtk << "DATASET UNSTRUCTURED_GRID\n" ;
	vtk << "POINTS " << vertices.cols() << " float\n" ;
	write( vtk, vertices.data(), WD, vertices.cols() ) ;
	vtk << "CELLS " << nodeIndices.cols() << " " << nodeIndices.size() << "\n";
	write( vtk, nodeIndices.data(), 1, nodeIndices.size() ) ;
	vtk << "CELL_TYPES " << nodeIndices.cols() << "\n";
	write( vtk, cellTypes.data(), 1, nodeIndices.cols() ) ;
}

template <typename ShapeFuncT>
size_t VTKFieldWriter<ShapeFuncT>::nDataPoints() const
{
	return m_shape.nDOF() ;
}

template <typename ShapeFuncT>
template< typename Derived >
bool VTKFieldWriter<ShapeFuncT>::dump( const char* name, const FieldBase< Derived >& field )
{
	if( !m_file.is_open() ) {
		Log::Error() << " VTKParticlesWriter: should call startFile() before dumping data " << std::endl ;
		return false ;
	}

	writeAttribute( name, field.flatten().data(), FieldBase< Derived >::D ) ;

	return true ;
}

#define INSTANTIATE( Shape ) \
	template class VTKFieldWriter<Shape> ; \
	template  bool VTKFieldWriter<Shape>::dump( const char*, const FieldBase< AbstractScalarField<Shape> >& ) ; \
	template  bool VTKFieldWriter<Shape>::dump( const char*, const FieldBase< AbstractVectorField<Shape> >& ) ; \
	template  bool VTKFieldWriter<Shape>::dump( const char*, const FieldBase< AbstractTensorField<Shape> >& ) ; \

EXPAND_INSTANTIATIONS
#undef INSTANTIATE

} //d6
