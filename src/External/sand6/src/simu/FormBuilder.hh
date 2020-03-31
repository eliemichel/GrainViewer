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

#ifndef D6_FORM_BUILDER_HH
#define D6_FORM_BUILDER_HH

#include "geo/geo.fwd.hh"
#include "geo/MeshBase.hh"

#include "utils/block_mat.hh"

namespace d6 {

class Particles ;

namespace form {
	enum Side {
		Left,
		Right
	};

	template< typename LhsShape, typename RhsShape, Side side >
	struct GetSide ;
}

//! Utility class for building matrices of bilinear forms
//! a(u,v) = u' A v, i.e. row == left, col == right
template < typename LhsShape, typename RhsShape >
class FormBuilder {

	typedef typename FormMat< WD,WD >::Type::MajorIndexType          CompressedIndexType ;
	typedef typename CompressedIndexType::Index BgIndex ;

	typedef const std::vector< Index > &Indices ;

	typedef const typename LhsShape::Interpolation& LhsItp ;
	typedef const typename LhsShape::Derivatives& LhsDcdx ;
	typedef const typename LhsShape::Derivatives::ConstRowXpr LhsDcdxRow ;

	typedef const typename RhsShape::Interpolation& RhsItp ;
	typedef const typename RhsShape::Derivatives& RhsDcdx ;

public:

	FormBuilder( const LhsShape& lhsShape, const RhsShape& rhsShape )
		: m_lhsShape( lhsShape ), m_rhsShape( rhsShape )
	{}

	void reset( Index rows ) ;
	Index rows() const { return m_data.size() ; }

	//! Computes matrices non-zero blocks (nodes sharing a cell) from list of active cells
	template < form::Side side, typename QPIterator, typename Func >
	void addToIndexIf( const QPIterator& qpBegin, const QPIterator& qpEnd,
					   Indices rowIndices, Indices colIndices, const Func& f ) ;

	template < typename Func >
	void addToIndexIf( Indices rowIndices, Indices colIndices, const Func& f ) ;
	void addToIndex( Indices rowIndices, Indices colIndices	 ) ;

	// only for LhsShape==RhsShape )
	template < typename CellIterator >
	void addToIndex( const CellIterator& cellBegin, const CellIterator& cellEnd,
					 Indices rowIndices, Indices colIndices	 ) ;

	void addRows( Index rows ) ;

	//! Compute compressed sparse block matrix index
	void makeCompressed() ;


	//! Integrate over quadrature points
	template < form::Side side, typename QPIterator, typename Func >
	void integrate_qp( const QPIterator& qpBegin, const QPIterator& qpEnd, Func func ) const ;
	template < typename Func >
	void integrate_qp( Func func ) const ;

	template < form::Side side, typename CellIterator, typename Func >
	void integrate_cell( const CellIterator& cellBegin, const CellIterator& cellEnd, Func func ) const ;

	//! Integrate over nodes ( trapezoidal approx -- only for LHSShape==RhsShape )
	template < typename CellIterator, typename Func >
	void integrate_node( const CellIterator& cellBegin, const CellIterator& cellEnd, Func func ) const ;

	//! Integrate over particles (MPM)
	template < typename Func >
	void integrate_particle( const Particles& particles, Func func ) const  ;

	// Building blocks

	static void addDuDv     ( FormMat<WD,WD>::Type& A, Scalar w, Index rowIndex, LhsDcdxRow row_dx,
							  RhsItp itp, RhsDcdx dc_dx, Indices colIndices ) ;
	static void addDpV      ( FormMat< 1,WD>::Type& A, Scalar w, Index rowIndex, LhsDcdxRow row_dx, RhsItp itp, Indices colIndices ) ;
	static void addTauDu    ( FormMat<SD,WD>::Type& A, Scalar w, Index rowIndex, RhsItp itp, RhsDcdx dc_dx, Indices colIndices ) ;
	static void addTauWu    ( FormMat<RD,WD>::Type& A, Scalar w, Index rowIndex, RhsItp itp, RhsDcdx dc_dx, Indices colIndices ) ;

	static void addDuDv     ( FormMat<WD,WD>::Type& A, Scalar w, LhsItp lhs_itp, LhsDcdx lhs_dc_dx, RhsItp rhs_itp, RhsDcdx rhs_dc_dx, Indices rowIndices, Indices colIndices ) ;
	static void addDpV      ( FormMat<1, WD>::Type& A, Scalar w, LhsItp lhs_itp, LhsDcdx dc_dx, RhsItp rhs_itp, Indices rowIndices, Indices colIndices ) ;
	static void addTauDu    ( FormMat<SD,WD>::Type& A, Scalar w, LhsItp lhs_itp, RhsItp rhs_itp, RhsDcdx dc_dx, Indices rowIndices, Indices colIndices ) ;
	static void addTauWu    ( FormMat<RD,WD>::Type& A, Scalar w, LhsItp lhs_itp, RhsItp rhs_itp, RhsDcdx dc_dx, Indices rowIndices, Indices colIndices ) ;

	static void addUTaunGphi( FormMat<SD,WD>::Type& A, Scalar w, LhsItp lhs_itp, RhsItp rhs_itp, const Vec& dphi_dx, Indices rowIndices, Indices colIndices ) ;
	static void addUTauGphi ( FormMat<SD,WD>::Type& A, Scalar w, LhsItp lhs_itp, RhsItp rhs_itp, const Vec& dphi_dx, Indices rowIndices, Indices colIndices ) ;

	const CompressedIndexType& index() { return m_compressed ; }

private:

	template< typename LhsS, typename RhsS, form::Side side >
	friend struct form::GetSide ;

	LhsShape m_lhsShape ;
	RhsShape m_rhsShape ;

	CompressedIndexType m_compressed ;
	std::vector< std::vector< BgIndex > > m_data ;

};


} //d6

#endif
