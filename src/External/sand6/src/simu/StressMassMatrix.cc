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

#include "StressMassMatrix.hh"

#include "simu/ActiveIndices.hh"
#include "geo/MeshImpl.hh"

#include <Eigen/Eigenvalues>

#include <bogus/Core/Block.impl.hpp>


namespace d6 {

template < typename Shape>
void AbstractStressMassMatrix<Shape>::compute( const Shape& , const Active& , const Index totNodes )
{
	inv_sqrt.setRows( totNodes ) ;
	inv_sqrt.setIdentity() ;
}

template < typename MeshT>
void AbstractStressMassMatrix<DGLinear<MeshT>>::compute( const Shape& shape, const Active& nodes, const Index totNodes )
{

	assert( 0 == ( nodes.count() % Shape::NI ) ) ;
	const Index nCells = nodes.count() / Shape::NI ;

	typedef Eigen::Matrix < Scalar, Shape::NI, Eigen::Dynamic > BlockAgg ;
	BlockAgg diagBlocks ;

	diagBlocks.resize( Shape::NI, Shape::NI*nCells );
	diagBlocks.setZero() ;

	typename Shape::Location loc ;
	typename Shape::Interpolation itp ;

	for( auto qpIt = shape.qpBegin() ; qpIt != shape.qpEnd() ; ++qpIt ) {
		qpIt.locate( loc ) ;
		shape.interpolate( loc, itp ) ;
		for( Index k = 0 ; k < itp.nodes.rows() ; ++k ) {
			const Index rowIndex = nodes.indices[itp.nodes[k]] ;
			if( rowIndex == Active::s_Inactive ) continue ;

			const Index cellIndex = rowIndex / Shape::NI ;
			const Index rowInCell = rowIndex - cellIndex * Shape::NI ;

			for( Index j = 0 ; j < itp.nodes.rows() ; ++j ) {
				const Index colIndex = nodes.indices[itp.nodes[j]] ;
				assert( cellIndex == (colIndex / Shape::NI ) ) ;
				if( colIndex == Active::s_Inactive ) continue ;

				diagBlocks( rowInCell, colIndex ) += itp.coeffs[k]*itp.coeffs[j] ;
			}
		}
	}

	typedef Eigen::Matrix< Scalar, Shape::NI, Shape::NI > MatDG ;

#pragma omp parallel for
	for( Index i = 0 ; i < nCells ; ++i) {
		typedef Eigen::Block< BlockAgg, Shape::NI, Shape::NI > BlockDG ;
		BlockDG B = diagBlocks.template block< Shape::NI, Shape::NI >( 0, i * Shape::NI  ) ;
		MatDG   M = B ;

		Eigen::SelfAdjointEigenSolver< MatDG > es( M ) ;
		B = es.eigenvectors() * (1. / es.eigenvalues().array().sqrt() ).matrix().asDiagonal() * es.eigenvectors().transpose() ;
//        B.setIdentity() ;

//        std::cout << (B*B*M) << std::endl ;
	}


	inv_sqrt.setRows( totNodes ) ;

	for( Index i = 0 ; i < nCells ; ++i ) {

		for( Index k = 0 ; k < Shape::NI ; ++k ) {
			for( Index j = 0 ; j <= k ; ++j ) {
				inv_sqrt.insertBack( i*Shape::NI + k, i*Shape::NI + j )
						= diagBlocks( k, i*Shape::NI + j ) * MatS::Identity() ;
			}

		}
	}
	for( Index i = nodes.count() ; i < totNodes ; ++i ) {
		inv_sqrt.insertBack( i,i ).setIdentity() ;
	}

	inv_sqrt.finalize() ;

}


template struct AbstractStressMassMatrix<     Linear< MeshImpl > > ;
template struct AbstractStressMassMatrix<   DGLinear< MeshImpl > > ;
template struct AbstractStressMassMatrix< DGConstant< MeshImpl > > ;
template struct AbstractStressMassMatrix<  UnstructuredShapeFunc > ;

} //d6

