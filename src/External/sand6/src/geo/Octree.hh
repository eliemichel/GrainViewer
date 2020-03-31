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

#ifndef D6_OCTREE_HH
#define D6_OCTREE_HH

#include "MeshBase.hh"
#include "geo/Voxel.hh"

#include <unordered_map>

namespace d6 {

class Octree ;
class Particles ;

struct OctreeIterator
{
	typedef MeshTraits< Octree > Traits ;
	typedef typename Traits::Cell    Cell ;
	typedef typename Traits::CellGeo CellGeo ;

	const Octree& grid ;
	Cell cell ;

	OctreeIterator( const Octree& g, const Cell& c )
		: grid(g), cell(c)
	{}

	OctreeIterator& operator++() ;

	bool operator==( const OctreeIterator& o ) const
	{
		return (cell == o.cell).all() ;
	}
	bool operator!=( const OctreeIterator& o ) const
	{
		return (cell != o.cell).any() ;
	}

	Cell operator*() const {
		return cell ;
	}

	Index index() const ;

};


class Octree : public MeshBase< Octree >
{
public:

	typedef MeshBase< Octree > Base ;

	typedef typename Base::Cell Cell ;
	typedef typename Base::CellGeo CellGeo ;
	typedef VecWi 				Vertex ;

	Octree( const Vec& box, const VecWi &res, const Particles* = nullptr ) ;

	void set_box( const Vec& box ) ;

	Index nNodes() const {
		return m_nodeIds.size() ;
	}

	Index nCells() const {
		return m_trees.back().offset() + m_trees.back().nLeafs() ;
	}

	Index cellIndex( const Cell& cell ) const {
		return subtree(cell).offset() + cell[WD] ;
	}

	Vec box() const
	{ return firstCorner( m_dim ) ; }

	using Base::locate ;
	void locate( const Vec &x, Location& loc ) const ;

	CellIterator cellBegin() const {
		return OctreeIterator( *this, Cell::Zero() ) ;
	}
	CellIterator cellEnd() const {
		Cell cell = Cell::Zero() ;
		cell[0] = m_dim[0] ;
		return OctreeIterator( *this, cell ) ;
	}

	void get_geo( const Cell &cell, CellGeo& geo ) const ;


	template < typename Archive >
	void save( Archive &ar, unsigned int ) const ;
	template < typename Archive >
	void load( Archive &ar, unsigned int ) ;
	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) ;


	bool onBoundary( const Cell &cell ) const {
		//TODO remove false-positives (use high-res)
		return cell.head<WD>().minCoeff() == 0 || (m_dim - cell.head<WD>()).minCoeff() == 1 ;
	}

	void boundaryInfo( const Location &loc, const BoundaryMapper& mapper, BoundaryInfo &info ) const ;

	void clamp_cell( Cell& cell ) const {
		cell.head<WD>() = ArrWi::Zero().max(cell.head<WD>()).min(m_dim.array()-ArrWi::Ones()) ;
	}

	const ArrWi& dim() const { return m_dim ; }
	const Arr&    dx() const { return  m_dx ; }

	// Octree specific public if

	typedef Eigen::Matrix< Index, Traits::NV, 1 > NodeList ;

	void list_nodes( const Cell& cell, NodeList& nodes ) const ;

	bool split( const Cell& cell ) ;

	void rebuild() ;

	Index cellRes( const Cell& cell ) const {
		return subtree(cell).res( cell[WD] ) ;
	}

	// Public for testing purposes

	struct SubTree {

		SubTree() ;

		void find( const Vec& pos, Index &leafIndex, Coords& coords ) const ;

		void compute_geo( Index leafIndex, Voxel &geo ) const ;
		void upres_cell(Index leafIndex, Index target_res, ArrWi& hires_cell, Index &size) const ;

		Index res( Index leafIndex ) const ;

		Index nLeafs() const ;

		Index offset() const { return m_offset ; }
		void offset( Index off ) { m_offset = off ; }

		bool split( Index leafIndex, Index maxDepth ) ;

		template < typename Archive >
		void serialize( Archive &ar, unsigned int ) {
			ar & m_nodes ;
		}

	private:
		struct Node {
			Index subSize ;
			Index nLeafs  ;

			template < typename Archive >
			void serialize( Archive &ar, unsigned int ) {
				ar & subSize ;
				ar & nLeafs ;
			}

		} ;

		Index m_offset ;
		std::vector< Node > m_nodes ;

		struct SubCell ;

		void find( Index offset, Arr& rel, Index &leafIndex, Coords& coords ) const ;

		void subcell ( Index leafIndex, SubCell& cell,
					   Index offset, Index leafOffset ) const  ;

		bool split( Index leafIndex,  Index &offset, Index leafOffset, Index maxDepth ) ;

	} ;

private:

	void get_corner( const ArrWi &cell, Vec& corner ) const {
		corner = (cell.head<WD>().array().cast< Scalar >() * m_dx.array()).matrix() ;
	}

	Vec firstCorner( const ArrWi &cell ) const
	{ Vec corner ; get_corner( cell, corner ) ; return corner ; }


	Index subtreeIndex( const Cell& cell ) const
	{
		Index idx =  (m_dim[1]) * cell[0]	+ cell[1] ;
		if( WD == 2 )
			return idx ;
		return idx*m_dim[2] + cell[2] ;
	}
	SubTree& subtree( const Cell& cell )
	{ return m_trees[ subtreeIndex(cell)] ; }
	const SubTree& subtree( const Cell& cell ) const
	{ return m_trees[ subtreeIndex(cell)] ; }

	Index nSubCells( const Cell& cell ) const {
		return subtree(cell).nLeafs() ;
	}

	Index hrNodeIndex( const ArrWi& node, Index res ) const
	{
		Index idx = (res*m_dim[1]+1) * node[0] + node[1] ;
		if( WD == 2 )
			return idx ;
		return idx*(res*m_dim[2]+1) + node[2] ;
	}

	ArrWi m_dim ;
	Arr   m_dx  ;
	Index m_maxDepth ;

	std::vector< SubTree > m_trees ;

	std::unordered_map< Index, Index > m_nodeIds ;

	friend struct OctreeIterator ;
} ;

} //d6
#endif
