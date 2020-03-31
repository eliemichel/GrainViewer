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

#include "VTKWriter.hh"

#include "utils/File.hh"
#include "utils/Log.hh"
#include "utils/endian.hh"

#include "geo/Tensor.hh"

namespace d6 {

VTKWriter::VTKWriter(const char *base_dir)
	: m_base_dir( base_dir ), m_mode( Binary )
{
}

bool VTKWriter::startFile( const char* name, unsigned frame )
{
	assert( !m_file.is_open() ) ;

	if( !open( frame, name ) )
		return false ;

	writeHeader( m_file, name ) ;
	writeMesh( m_file );

	m_file << "POINT_DATA " << nDataPoints() << "\n" ;

	return true ;
}

std::string VTKWriter::fileName(const unsigned frame, const char *dataName) const
{
	return FileInfo( FileInfo( m_base_dir ).filePath("vtk") ).filePath( arg("%1-%2.vtk", dataName, frame) ) ;
}

bool VTKWriter::open(const unsigned frame, const char *dataName )
{
	const std::string fn = fileName(frame, dataName) ;
	FileInfo(fn).makePath() ;
	if( !m_file.open(fn , std::ios_base::out ) ) {
		Log::Error() << "Could not write into " << m_file.name() <<std::endl ;
		return false ;
	}

	Log::Info() << "Writing " << fn << std::endl ;

	return true ;
}

void VTKWriter::writeHeader( File& file, const char *title ) const
{
	file << "# vtk DataFile Version 2.0\n" ;
	file << title << "\n" ;
	if( m_mode == Ascii )
		file << "ASCII\n" ;
	else
		file << "BINARY\n" ;
}

void VTKWriter::writeAttributeHeader( File& file, const int Dim, const char* name) const
{
	switch( Dim ) {
		case 1:
			file << "SCALARS " << name << " float\n" ;
			file << "LOOKUP_TABLE default\n" ;
			break ;
		case WD:
			file << "VECTORS " << name << " float\n" ;
			break ;
		case SD:
			file << "TENSORS " << name << " float\n" ;
			break ;
	}

}

template< typename T >
struct binary_type
{
	typedef T type ;
} ;
template< >
struct binary_type< double >
{
	typedef float type ;
} ;
template< >
struct binary_type< Index >
{
	typedef int type ;
} ;


template< typename Scalar >
static void write_scalar_ascii( File& file, const Scalar* data, const size_t size )
{
	for( size_t i = 0 ; i < size ; ++ i ) {
		file << data[i] << " " ;
	}
}

template< typename Scalar >
static void write_scalar_binary( File& file, const Scalar* data, const size_t size )
{
	typedef typename binary_type< Scalar >::type BinScalar ;
	unsigned char bin[ sizeof(BinScalar) ] ;

	for( size_t i = 0 ; i < size ; ++ i ) {
		to_big_endian( BinScalar(data[i]), bin ) ;
		for( unsigned j=0 ; j < sizeof(BinScalar) ; ++j )
			file << bin[j] ;
	}
}

#if D6_DIM == 3
template< typename Scalar >
static void write_vector_ascii( File& file, const Scalar* data, const size_t size )
{
	write_scalar_ascii( file, data, size*WD ) ;
}
template< typename Scalar >
static void write_vector_binary( File& file, const Scalar* data, const size_t size )
{
	write_scalar_binary( file, data, size*WD ) ;
}

template< typename Scalar >
static void write_tensor_ascii( File& file, const Scalar* data, const size_t size )
{
	Mat mat ;
	for( size_t i = 0 ; i < size ; ++ i ) {
		tensor_view( Eigen::Matrix<Scalar,SD,1>::Map( data + SD*i ) ).get( mat ) ;
		write_scalar_ascii( file, mat.data(), mat.size() ) ;
	}
}

template< typename Scalar >
static void write_tensor_binary( File& file, const Scalar* data, const size_t size )
{
	Mat mat ;
	for( size_t i = 0 ; i < size ; ++ i ) {
		tensor_view( Eigen::Matrix<Scalar,SD,1>::Map( data + SD*i ) ).get( mat ) ;
		write_scalar_binary( file, mat.data(), mat.size() ) ;
	}
}
#else
template< typename Scalar >
static void write_vector_ascii( File& file, const Scalar* data, const size_t size )
{
	Eigen::Matrix<Scalar,3,1> vec ; vec.setZero() ;
	for( size_t i = 0 ; i < size ; ++ i ) {
		vec.template head<WD>() = Eigen::Matrix<Scalar,WD,1>::Map( data + WD*i ) ;
		write_scalar_ascii( file, vec.data(), vec.size() ) ;
	}
}
template< typename Scalar >
static void write_vector_binary( File& file, const Scalar* data, const size_t size )
{
	Eigen::Matrix<Scalar,3,1> vec ; vec.setZero() ;
	for( size_t i = 0 ; i < size ; ++ i ) {
		vec.template head<WD>() = Eigen::Matrix<Scalar,WD,1>::Map( data + WD*i ) ;
		write_scalar_binary( file, vec.data(), vec.size() ) ;
	}
}

template< typename Scalar >
static void write_tensor_ascii( File& file, const Scalar* data, const size_t size )
{
	Mat33 mat ; mat.setZero() ;
	Mat mat2d ;
	for( size_t i = 0 ; i < size ; ++ i ) {
		tensor_view( Eigen::Matrix<Scalar,SD,1>::Map( data + SD*i ) ).get( mat2d ) ;
		mat.template block<WD,WD>(0,0) = mat2d ;
		write_scalar_ascii( file, mat.data(), mat.size() ) ;
	}
}

template< typename Scalar >
static void write_tensor_binary( File& file, const Scalar* data, const size_t size )
{
	Mat33 mat ; mat.setZero() ;
	Mat mat2d ;
	for( size_t i = 0 ; i < size ; ++ i ) {
		tensor_view( Eigen::Matrix<Scalar,SD,1>::Map( data + SD*i ) ).get( mat2d ) ;
		mat.template block<WD,WD>(0,0) = mat2d ;
		write_scalar_binary( file, mat.data(), mat.size() ) ;
	}
}
#endif

template< typename Scalar >
void VTKWriter::write( File &file, const Scalar* data, int Dim, const size_t size ) const
{
	if( m_mode == Ascii ) {
		if( Dim == SD ) {
			write_tensor_ascii( file, data, size ) ;
		} else if( Dim == WD ) {
			write_vector_ascii( file, data, size ) ;
		} else {
			write_scalar_ascii( file, data, size*Dim ) ;
		}
	} else {
		if( Dim == SD ) {
			write_tensor_binary( file, data, size ) ;
		} else if( Dim == WD ) {
			write_vector_binary( file, data, size ) ;
		} else {
			write_scalar_binary( file, data, size*Dim ) ;
		}
	}

	file << "\n" ;
}

template< typename Scalar >
void VTKWriter::writeAttribute( const char *name, const Scalar* data, int Dim )
{
	writeAttributeHeader( m_file, Dim, name ) ;
	write( m_file, data, Dim, nDataPoints() ) ;
}

template void VTKWriter::write( File &file, const double*, int, size_t) const ;
template void VTKWriter::write( File &file, const  float*, int, size_t) const ;
template void VTKWriter::write( File &file, const    int*, int, size_t) const ;
template void VTKWriter::writeAttribute( const char* name, const double*, int ) ;

}
