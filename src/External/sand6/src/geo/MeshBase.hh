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

#ifndef D6_MESH_BASE_HH
#define D6_MESH_BASE_HH

#include "geo.fwd.hh"
#include "MeshTraits.hh"

#include "utils/alg.hh"

namespace d6 {

struct BoundaryInfo ;
struct BoundaryMapper ;
typedef std::vector< BoundaryInfo > BoundaryConditions ;


template <typename Derived>
class MeshBase {

public:

	typedef MeshTraits< Derived > Traits ;
	typedef typename Traits::CellIterator CellIterator ;
	typedef typename Traits::Cell    Cell  ;
	typedef typename Traits::Cells   Cells ;
	typedef typename Traits::CellGeo CellGeo ;

	enum {
		NV = Traits::NV ,
		NC = Traits::NC
	} ;

	typedef Eigen::Matrix< Scalar, NC, 1  > Coords ;


	struct Location {
		Cell      cell  ; // Cell
		Coords   coords ; // Coordinates in cell
	};

	Derived& derived()
	{ return static_cast< Derived& >( *this ) ; }
	const Derived& derived() const
	{ return static_cast< const Derived& >( *this ) ; }

	Index nNodes() const { return derived().nNodes() ; }
	Index nCells() const { return derived().nCells() ; }

	Index cellIndex( const Cell& cell ) const
	{ return derived().cellIndex( cell ) ; }

	Vec box() const { return derived().box() ; }
	Vec clamp_point( const Vec& p ) const {
		return Vec::Zero().cwiseMax(p).cwiseMin(box()) ;
	}

	void locate( const Vec &x, Location& loc ) const {
		derived().locate( x, loc ) ;
	}
	Location locate( const Vec &x ) const {
		Location loc ;
		derived().locate( x, loc ) ;
		return loc ;
	}

	CellIterator cellBegin() const { return derived().cellBegin() ; }
	CellIterator cellEnd()  const { return derived().cellEnd() ; }

	void get_geo( const Cell &cell, CellGeo& geo ) const {
		derived().get_geo( cell, geo ) ;
	}

	bool onBoundary( const Cell &cell ) const {
		return derived().onBoundary( cell ) ;
	}

	void boundaryInfo( const Location &loc, const BoundaryMapper& mapper, BoundaryInfo& bc ) const {
		derived().boundaryInfo( loc, mapper, bc ) ;
	}

	Vec pos( const Location& loc ) const ;

	template< template <typename> class ShapeFunc > ShapeFunc<Derived> shaped() const {
		return ShapeFunc<Derived>( derived() ) ;
	}
} ;

} //d6

#endif
