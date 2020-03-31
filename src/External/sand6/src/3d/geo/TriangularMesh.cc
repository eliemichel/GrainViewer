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

#include "TriangularMesh.hh"

#include "utils/File.hh"
#include "utils/Log.hh"

#include <Eigen/Geometry>

namespace d6 {

bool TriangularMesh::loadObj( const char* fileName )
{

	File meshFile( fileName, std::ios_base::in ) ;
	if( !meshFile.is_open() ) {
		Log::Error() << "Could not read " << fileName << std::endl ;
		return false ;
	}

	if( !firstObjPass( meshFile ) )
		return false ;

	meshFile.clear() ;
	meshFile.seekg(0, std::ios::beg) ;

	if( !secondObjPass( meshFile ) )
		return false ;

	m_name = fileName ;

	return true ;

} ;

bool TriangularMesh::firstObjPass( File& file )
{
	//First pass count vertices, etc

	Index vtxCount = 0 ;
	Index vtxNormalsCount = 0 ;
	Index vtxUVsCount = 0 ;

	Index facesCount = 0 ;

	std::string line ;
	std::string tok  ;
	while( std::getline( file, line ) ) {

		std::istringstream iss(line) ;
		iss >> tok ;

		if( tok.size() == 0 ) continue ;

		switch( tok[0] ) {
		case 'v':
			if( tok.length() == 1 ) {
				++vtxCount ;
			} else {
				switch(tok[1]) {
				case 'p': // parameter space
					break ;
				case 'n':
					++ vtxNormalsCount ;
					break ;
				case 't': // tex coordinates
					++ vtxUVsCount ;
					break ;
				default:
					goto error ;
				}
			}
			break ;
		case 'f':
			++facesCount ;
		}
	}

	m_vertices.resize( 3, vtxCount ) ;

	m_vertexIndices.resize( 3, facesCount ) ;
	m_faceNormals.resize( 3, 0) ;

	m_vertexNormals.resize( 3, vtxNormalsCount ) ;
	if( vtxNormalsCount > 0 )
		m_normalIndices.resize( 3, facesCount ) ;
	else
		m_normalIndices.resize( 3, 0 ) ;

	m_vertexUVs.resize( 3, vtxUVsCount ) ;
	if( vtxUVsCount > 0 )
		m_uvIndices.resize( 3, facesCount ) ;
	else
		m_uvIndices.resize( 3, 0 ) ;

	return true ;

error:
	Log::Error() << "Obj parse error; Token: " << tok << std::endl ;
	Log::Error() << "\n Line: " << line << std::endl ;

	return false ;
}

bool TriangularMesh::secondObjPass( File& file )
{
	bool parseNormals = hasVertexNormals() ;
	bool parseTexture = hasVertexUVs() ;

	Index vtxCount = 0 ;
	Index vtxNormalsCount = 0 ;
	Index vtxUVsCount = 0 ;
	Index facesCount = 0 ;

	std::string line ;
	std::string tok  ;
	while( std::getline( file, line ) ) {

		std::istringstream iss(line) ;
		iss >> tok ;

		if( tok.size() == 0 ) continue ;

		switch( tok[0] ) {
		case 'v':
			if( tok.length() == 1 ) {
				iss >> m_vertices( 0, vtxCount )
					>> m_vertices( 1, vtxCount )
					>> m_vertices( 2, vtxCount )  ;
				if(! iss ) goto error ;
				++vtxCount ;
			} else {
				switch(tok[1]) {
				case 'n':
					iss >> m_vertexNormals( 0, vtxNormalsCount )
						>> m_vertexNormals( 1, vtxNormalsCount )
						>> m_vertexNormals( 2, vtxNormalsCount )  ;
					if(! iss ) goto error ;
					m_vertexNormals.col( vtxNormalsCount ) = m_vertexNormals.col(vtxNormalsCount).normalized() ;
					++ vtxNormalsCount ;
					break ;
				case 't':
					iss >> m_vertexUVs( 0, vtxUVsCount )
						>> m_vertexUVs( 1, vtxUVsCount )  ;
					if(! iss ) goto error ;
					iss >> m_vertexUVs( 2, vtxUVsCount )  ;
					if(! iss ) m_vertexUVs( 2, vtxUVsCount ) = 0 ;
					++ vtxUVsCount ;
					break ;
				default:
					break ;
				}
			}
			break ;
		case 'f':

			iss >> tok ;

			for( int k = 0 ; k < 3 ; ++k )
			{
				if(! iss ) goto error ;
				std::istringstream tss(tok) ;

				tss >> m_vertexIndices( k, facesCount ) ;
				char c ;
				if ( parseNormals || parseTexture ) {
					tss >> c ;
					if( c != '/' ) goto error ;
					if( parseTexture )
						tss >> m_uvIndices( k, facesCount ) ;
					if( parseNormals ) {
						tss >> c ;
						if( c != '/' ) goto error ;
						tss >> m_normalIndices( k, facesCount ) ;
					}
					if( !tss ) goto error ;
				}
				iss >> tok ;

			}
			++facesCount ;
		}
	}

	m_vertexIndices.array() -= 1 ;
	m_normalIndices.array() -= 1 ;
	m_uvIndices.array() -= 1 ;

	return true ;

error:
	Log::Error() << "Obj value error;  Line: " << line << std::endl ;

	return false ;
}


void TriangularMesh::computeFaceNormals()
{
	if( hasFaceNormals() )
		return ;

	const Index n = nFaces() ;

	m_faceNormals.resize( 3, n ) ;

#pragma omp parallel for
	for( int ii = 0 ; ii < n ; ++ii ) {
		Index i = static_cast<Index>(ii);
		// counterclockwise ori -- assumes non-degenerated faces
		const Vec ccw = ( vertex(i,1)-vertex(i,0) ).cross( vertex(i,2)-vertex(i,0) ).normalized() ;
		int inv = 0 ;

		if( hasVertexNormals() ) {
			//Deduce sign from vertex normals
			for( Index k = 0 ; k < 3 ; ++k ) {
				if( ccw.dot( normal(i,k) ) < 0 )
					++inv ;
			}
		}

		m_faceNormals.col(i) = (inv>1 ? -1 : 1.) * ccw ;
	}

}

Vec TriangularMesh::interpolatedNormal( Index face, const Vec& baryCoords ) const
{

	if( hasVertexNormals() ) {
		return ( baryCoords[0] * normal( face, 0 ) +
				baryCoords[1] * normal( face, 1 ) +
				baryCoords[2] * normal( face, 2 ) ).normalized() ;

	} else {
		assert( hasFaceNormals() ) ;
		return faceNormal( face ) ;
	}
}

} //d6
