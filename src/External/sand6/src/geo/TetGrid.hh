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

#ifndef D6_TETGRID_HH
#define D6_TETGRID_HH

#include "MeshBase.hh"
#include "geo/Tet.hh"

namespace d6 {

class TetGrid ;
class Particles ;

struct TetGridIterator
{
	typedef MeshTraits< TetGrid > Traits ;
	typedef typename Traits::Cell    Cell ;
	typedef typename Traits::CellGeo CellGeo ;

	const TetGrid& grid ;
	Cell cell ;

	TetGridIterator( const TetGrid& g, const Cell& c )
		: grid(g), cell(c)
	{}

	TetGridIterator& operator++() ;

	bool operator==( const TetGridIterator& o ) const
	{
		return cell.isApprox( o.cell );
	}
	bool operator!=( const TetGridIterator& o ) const
	{
		return ! cell.isApprox( o.cell );
	}

	Cell operator*() const {
		return cell ;
	}

	Index index() const ;

};


class TetGrid : public MeshBase< TetGrid >
{
public:
	typedef MeshBase< TetGrid > Base ;

	typedef typename Base::Cell Cell ;
	typedef typename Base::CellGeo CellGeo ;
	typedef VecWi 				Vertex ;

	static constexpr Index Nsub = WD == 3 ? 6 : 2 ;

	TetGrid( const Vec& box, const VecWi &res, const Particles * = nullptr ) ;


	Index nNodes() const
	{ return (m_dim+1).prod() ; }

	Index nEdges() const ;

	Index nCells() const
	{ return m_dim.prod() * Nsub ; }

	Index cellIndex( const Cell& cell ) const
	{
		Index idx =  (m_dim[1]) * cell[0]	+ cell[1] ;
		if( WD == 3 )
			idx = idx*m_dim[2] + cell[2] ;

		return Nsub*idx + cell[WD] ;
	}

	Vec box() const
	{ return firstCorner( m_dim ) ; }

	using Base::locate ;
	void locate( const Vec &x, Location& loc ) const ;

	CellIterator cellBegin() const {
		return TetGridIterator( *this, Cell::Zero() ) ;
	}
	CellIterator cellEnd() const {
		Cell cell ;
		cell[0] = m_dim[0] ;
		cell.tail<WD>().setZero() ;
		return TetGridIterator( *this, cell ) ;
	}

	void get_geo( const Cell &cell, CellGeo& geo ) const  ;

	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) {
		ar & m_dim ;
		ar &  m_dx ;
	}

	Index nAdjacent( Index idx ) const ;

	Index nodeIndex( const Vertex& node ) const
	{
		Index idx = (m_dim[1]+1) * node[0] + node[1] ;
		if( WD == 2 )
			return idx ;
		return idx*(m_dim[2]+1) + node[2] ;
	}

	bool onBoundary( const Cell &cell ) const {
		return cell.head<WD>().minCoeff() == 0 || (m_dim - cell.head<WD>()).minCoeff() == 1 ;
	}

	void boundaryInfo( const Location &loc, const BoundaryMapper& mapper, BoundaryInfo &info ) const ;

	const ArrWi& dim() const { return m_dim ; }
	const Arr&    dx() const { return  m_dx ; }

private:

	void get_corner( const VecWi &cell, Vec& corner ) const {
		corner = ( cell.array().cast< Scalar >() * m_dx ).matrix() ;
	}

	Vec firstCorner( const VecWi &cell ) const
	{ Vec corner ; get_corner( cell, corner ) ; return corner ; }

	ArrWi m_dim ;
	Arr   m_dx  ;

	friend struct TetGridIterator ;
};


} //d6

#endif
