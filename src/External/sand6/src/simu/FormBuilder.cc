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

#include "FormBuilder.impl.hh"

#include "geo/MeshImpl.hh"
#include "geo/FormBuildingBlocks.hh"

#include <bogus/Core/Block.impl.hpp>
#include <algorithm>

namespace d6 {


template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addToIndex(
		const std::vector< Index > &rowIndices,
		const std::vector< Index > &colIndices
		) {
	addToIndexIf( rowIndices, colIndices, [](const Vec&){return true ;} ) ;
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::makeCompressed()
{

	const size_t m = m_data.size() ;

#pragma omp parallel for
	for( int i = 0 ; i < m ; ++i ) {
		std::sort( m_data[i].begin(),  m_data[i].end() ) ;
		auto last = std::unique( m_data[i].begin(),  m_data[i].end() )  ;
		m_data[i].erase( last,  m_data[i].end() ) ;
	}

	m_compressed.clear() ;
	m_compressed.outer.resize( m + 1 ) ;

	m_compressed.outer[0] = 0 ;
	for( size_t i = 0 ; i < m ; ++i ) {
		m_compressed.outer[i+1] = m_compressed.outer[i] + m_data[i].size() ;
	}

	m_compressed.inner.resize( m_compressed.outer.back() ) ;

#pragma omp parallel for
	for( int i = 0 ; i < m ; ++i ) {
		memcpy( m_compressed.inner.data() + m_compressed.outer[i], m_data[i].data(),
				m_compressed.size( i ) * sizeof( BgIndex ) ) ;
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::reset(Index rows)
{
	m_data.clear();
	m_data.resize( rows );
	m_compressed.outer.clear() ;
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addRows( Index rows )
{
	m_data.resize( m_data.size() + rows );

	const Index old = m_compressed.outer.size() ;
	if( old > 0  )
	{
		m_compressed.outer.resize( old + rows ) ;
		for( Index i = 0 ; i < rows ; ++i  ) {
			m_compressed.outer[ old+i ] = m_compressed.outer[ old-1 ] ;
		}
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addDuDv( FormMat<WD,WD>::Type& A, Scalar w,
											   LhsItp lhs_itp, LhsDcdx lhs_dc_dx,
											   RhsItp rhs_itp, RhsDcdx rhs_dc_dx,
											   Indices rowIndices, Indices colIndices )
{
	for( int k = 0 ; k < lhs_itp.nodes.rows() ; ++k ) {
		addDuDv( A, w, rowIndices[lhs_itp.nodes[k]], lhs_dc_dx.row(k), rhs_itp, rhs_dc_dx, colIndices );
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addTauDu( FormMat<SD,WD>::Type& A, Scalar w,
												LhsItp lhs_itp, RhsItp rhs_itp, RhsDcdx dc_dx,
												Indices rowIndices, Indices colIndices )
{
	for( int k = 0 ; k < lhs_itp.nodes.rows() ; ++k ) {
		addTauDu( A, w * lhs_itp.coeffs[k], rowIndices[lhs_itp.nodes[k]], rhs_itp, dc_dx, colIndices );
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addDpV( FormMat<1,WD>::Type& A, Scalar w,
											  LhsItp lhs_itp, LhsDcdx dc_dx, RhsItp rhs_itp,
											  Indices rowIndices, Indices colIndices )
{
	for( int k = 0 ; k < lhs_itp.nodes.rows() ; ++k ) {
		addDpV( A, w, rowIndices[lhs_itp.nodes[k]], dc_dx.row(k), rhs_itp, colIndices );
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addTauWu( FormMat<RD,WD>::Type& A, Scalar w,
												LhsItp lhs_itp, RhsItp rhs_itp, RhsDcdx dc_dx,
												Indices rowIndices, Indices colIndices )
{
	for( int k = 0 ; k < lhs_itp.nodes.rows() ; ++k ) {
		addTauWu( A, w * lhs_itp.coeffs[k], rowIndices[lhs_itp.nodes[k]], rhs_itp, dc_dx, colIndices );
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addDuDv( FormMat<WD,WD>::Type& A, Scalar w, Index rowIndex,
											   LhsDcdxRow row_dx, RhsItp itp, RhsDcdx dc_dx, Indices colIndices )
{
	typedef FormMat<WD,WD>::Type::BlockType Block ;

	for( int j = 0 ; j < itp.nodes.rows() ; ++j ) {
		assert( A.blockPtr( rowIndex, colIndices[itp.nodes[j]] ) != A.InvalidBlockPtr ) ;
		Block &b = A.block( rowIndex, colIndices[itp.nodes[j]] ) ;
		FormBlocks::addDuDv( b, w, row_dx, dc_dx.row(j) ) ;
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addTauDu( FormMat<SD,WD>::Type& A, Scalar m, Index rowIndex,
												RhsItp itp, RhsDcdx dc_dx, Indices colIndices )
{
	typedef FormMat<SD,WD>::Type::BlockType Block ;

	for( int j = 0 ; j < itp.nodes.rows() ; ++j ) {
		assert( A.blockPtr( rowIndex, colIndices[itp.nodes[j]] ) != A.InvalidBlockPtr ) ;
		Block &b = A.block( rowIndex, colIndices[itp.nodes[j]] ) ;
		FormBlocks::addTauDu( b, m, dc_dx.row(j) ) ;
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>:: addDpV  ( FormMat< 1,WD>::Type& A, Scalar w, Index rowIndex, LhsDcdxRow row_dx,
												 RhsItp itp, Indices colIndices )
{
	typedef FormMat<1,WD>::Type::BlockType Block ;

	for( int j = 0 ; j < itp.nodes.rows() ; ++j ) {
		assert( A.blockPtr( rowIndex, colIndices[itp.nodes[j]] ) != A.InvalidBlockPtr ) ;
		Block &b = A.block( rowIndex, colIndices[itp.nodes[j]] ) ;
		FormBlocks::addDpV( b, w*itp.coeffs[j], row_dx ) ;
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addTauWu( FormMat<RD,WD>::Type& A, Scalar m, Index rowIndex,
												RhsItp itp, RhsDcdx dc_dx, Indices colIndices )
{
	typedef FormMat<RD,WD>::Type::BlockType Block ;

	for( int j = 0 ; j < itp.nodes.rows() ; ++j ) {
		assert( A.blockPtr( rowIndex, colIndices[itp.nodes[j]] ) != A.InvalidBlockPtr ) ;
		Block &b = A.block( rowIndex, colIndices[itp.nodes[j]] ) ;
		FormBlocks::addTauWu( b, m, dc_dx.row(j) ) ;
	}
}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addUTauGphi( FormMat<SD,WD>::Type& A, Scalar w,
												   LhsItp lhs_itp, RhsItp rhs_itp,
												   const Vec& dphi_dx, Indices rowIndices, Indices colIndices )
{
	typedef FormMat<SD,WD>::Type::BlockType Block ;

	for( int k = 0 ; k < lhs_itp.nodes.rows() ; ++k ) {
		for( int j = 0 ; j < rhs_itp.nodes.rows() ; ++j ) {
			assert( A.blockPtr( rowIndices[lhs_itp.nodes[k]], colIndices[rhs_itp.nodes[j]] ) != A.InvalidBlockPtr ) ;
			Block &b = A.block( rowIndices[lhs_itp.nodes[k]], colIndices[rhs_itp.nodes[j]] ) ;
			const Scalar m = w * lhs_itp.coeffs[k] * rhs_itp.coeffs[j] ;
			FormBlocks::addUTauGphi( b, m, dphi_dx );
		}
	}

}

template < typename LhsShape, typename RhsShape >
void FormBuilder<LhsShape, RhsShape>::addUTaunGphi( FormMat<SD,WD>::Type& A, Scalar w,
													LhsItp lhs_itp, RhsItp rhs_itp,
													const Vec& dphi_dx, Indices rowIndices, Indices colIndices )
{
	typedef FormMat<SD,WD>::Type::BlockType Block ;

	for( int k = 0 ; k < lhs_itp.nodes.rows() ; ++k ) {
		for( int j = 0 ; j < rhs_itp.nodes.rows() ; ++j ) {
			assert( A.blockPtr( rowIndices[lhs_itp.nodes[k]], colIndices[rhs_itp.nodes[j]] ) != A.InvalidBlockPtr ) ;

			Block &b = A.block( rowIndices[lhs_itp.nodes[k]], colIndices[rhs_itp.nodes[j]] ) ;
			const Scalar m = w * lhs_itp.coeffs[k] * rhs_itp.coeffs[j] ;
			FormBlocks::addUTaunGphi( b, m, dphi_dx );
		}
	}
}

template class FormBuilder<     Linear< MeshImpl >, Linear< MeshImpl > > ;
template class FormBuilder<   DGLinear< MeshImpl >, Linear< MeshImpl > > ;
template class FormBuilder< DGConstant< MeshImpl >, Linear< MeshImpl > > ;
template class FormBuilder<  UnstructuredShapeFunc, Linear< MeshImpl > > ;

#if (D6_MESH_IMPL == D6_MESH_TET_GRID)
template class FormBuilder<   Linear< MeshImpl >, P2< MeshImpl > > ;
template class FormBuilder< DGLinear< MeshImpl >, P2< MeshImpl > > ;
template class FormBuilder<       P2< MeshImpl >, P2< MeshImpl > > ;
template class FormBuilder<  UnstructuredShapeFunc, P2< MeshImpl > > ;
#endif

} //d6
