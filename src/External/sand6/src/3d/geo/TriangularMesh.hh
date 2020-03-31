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

#ifndef D6_TRIANGULAR_MESH_HH
#define D6_TRIANGULAR_MESH_HH

#include "utils/alg.hh"
#include <string>

namespace d6 {

class File ;

class TriangularMesh
{

public:
	typedef unsigned Index ;

	TriangularMesh() {}

	bool loadObj ( const char* fileName ) ;
	void computeFaceNormals() ;



	Index nFaces() const { return m_vertexIndices.cols() ; }
	Index nVertices() const { return m_vertices.cols() ; }

	bool hasVertexNormals() const
	{ return m_normalIndices.cols() > 0 ; }
	bool hasVertexUVs() const
	{ return m_uvIndices.cols() > 0 ; }
	bool hasFaceNormals() const
	{ return m_faceNormals.cols() > 0 ; }

	DynMat3::ConstColXpr vertex( Index face, Index num ) const
	{
		return m_vertices.col( m_vertexIndices( num, face ) ) ;
	}
	DynMat3::ConstColXpr normal( Index face, Index num ) const
	{
		assert( hasVertexNormals() ) ;
		return m_vertexNormals.col( m_normalIndices( num, face ) ) ;
	}
	DynMat3::ConstColXpr uv( Index face, Index num ) const
	{
		assert( hasVertexUVs() ) ;
		return m_vertexUVs.col( m_uvIndices( num, face ) ) ;
	}

	DynMat3::ConstColXpr faceNormal( Index face ) const
	{
		assert( hasFaceNormals() ) ;
		return m_faceNormals.col( face ) ;
	}
	Vec interpolatedNormal( Index face, const Vec& baryCoords ) const  ;

	const std::string& name() const
	{ return m_name ; }

	const DynMat3& vertices() const { return m_vertices ; }

private:

	bool firstObjPass( File& file ) ;
	bool secondObjPass( File& file ) ;

	// Vertex data
	DynMat3 m_vertices ;
	DynMat3 m_vertexNormals ;
	DynMat3 m_vertexUVs ;

	// Face data
	DynMat3 m_faceNormals ;
	Eigen::Matrix< Index, 3, Eigen::Dynamic > m_vertexIndices ;
	Eigen::Matrix< Index, 3, Eigen::Dynamic > m_normalIndices ;
	Eigen::Matrix< Index, 3, Eigen::Dynamic > m_uvIndices ;

	std::string m_name ;

} ;


} //d6

#endif
