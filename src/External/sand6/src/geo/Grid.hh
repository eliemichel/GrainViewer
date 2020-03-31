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

#ifndef D6_GRID_HH
#define D6_GRID_HH

#include "MeshBase.hh"
#include "geo/Voxel.hh"

#include <functional>

namespace d6 {

class Grid ;
class Particles ;

struct GridIterator
{
	typedef MeshTraits< Grid > Traits ;
	typedef typename Traits::Cell    Cell ;
	typedef typename Traits::CellGeo CellGeo ;

	const Grid& grid ;
	Cell cell ;

	GridIterator( const Grid& g, const Cell& c )
		: grid(g), cell(c)
	{}

	GridIterator& operator++() ;

	bool operator==( const GridIterator& o ) const
	{
		return (cell == o.cell).all() ;
	}
	bool operator!=( const GridIterator& o ) const
	{
		return (cell != o.cell).any() ;
	}

	Cell operator*() const {
		return cell ;
	}

	Index index() const ;

};


class Grid : public MeshBase< Grid >
{
public:

	typedef MeshBase< Grid > Base ;

	typedef typename Base::Cell Cell ;
	typedef typename Base::CellGeo CellGeo ;
	typedef VecWi 				Vertex ;

	Grid( const Vec& box, const VecWi &res, const Particles * = nullptr ) ;

	void set_box( const Vec& box ) ;

	Index nNodes() const
	{ return (m_dim+1).prod() ; }

	Index nCells() const
	{ return m_dim.prod(); }

	Index cellIndex( const Cell& cell ) const
	{
		Index idx =  (m_dim[1]) * cell[0]	+ cell[1] ;
		if( Cell::RowsAtCompileTime == 2 )
			return idx ;
		return idx*m_dim[2] + cell[2] ;
	}
	Index nodeIndex( const Vertex& node ) const
	{
		Index idx = (m_dim[1]+1) * node[0] + node[1] ;
		if( Cell::RowsAtCompileTime == 2 )
			return idx ;
		return idx*(m_dim[2]+1) + node[2] ;
	}
	Vec nodePosition( const Vertex& node ) const
	{
		return firstCorner( node ) ;
	}

	Vec box() const
	{ return firstCorner( m_dim ) ; }

	using Base::locate ;
	void locate( const Vec &x, Location& loc ) const
	{
		loc.coords = x.array()/m_dx.array() ;
		loc.cell = loc.coords.cast< Index >();
		clamp_cell( loc.cell) ;

		loc.coords -= loc.cell.cast< Scalar >().matrix() ;
	}

	CellIterator cellBegin() const {
		return GridIterator( *this, VecWi::Zero() ) ;
	}
	CellIterator cellEnd() const {
		VecWi cell = m_dim ;
		cell.tail<WD-1>().setZero() ;
		return GridIterator( *this, cell ) ;
	}

	void get_geo( const Cell &cell, CellGeo& geo ) const {
		get_corner( cell, geo.origin );
		geo.box = m_dx ;
	}

	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) {
		ar & m_dim ;
		ar &  m_dx ;
	}

	bool onBoundary( const Cell &cell ) const {
		return cell.minCoeff() == 0 || (m_dim - cell).minCoeff() == 1 ;
	}

	void boundaryInfo( const Location &loc, const BoundaryMapper& mapper, BoundaryInfo &info ) const ;

	void clamp_cell( Cell& cell ) const {
		cell = Cell::Zero().max(cell).min(m_dim.array()-Cell::Ones()) ;
	}

	const ArrWi& dim() const { return m_dim ; }
	const Arr&    dx() const { return  m_dx ; }

	void each_neighbour( const Cell& cell, const std::function<void(const Cell&) >& func ) const ;

private:

	void get_corner( const Cell &cell, Vec& corner ) const {
		corner = (cell.array().cast< Scalar >() * m_dx.array()).matrix() ;
	}

	Vec firstCorner( const Cell &cell ) const
	{ Vec corner ; get_corner( cell, corner ) ; return corner ; }

	ArrWi m_dim ;
	Arr   m_dx  ;

	friend struct GridIterator ;
} ;

} //d6
#endif
