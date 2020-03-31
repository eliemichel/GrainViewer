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

#include "Octree.hh"

namespace d6 {


Index OctreeIterator::index() const
{
	return grid.cellIndex( cell ) ;
}

Octree::Octree(const Vec &box, const VecWi &res, const Particles *)
	: Base(),
	  m_maxDepth(1)
{
	m_dim = res ;
	set_box( box ) ;
	rebuild() ;
}

void Octree::set_box( const Vec& box )
{
	m_dx.array() = box.array()/m_dim.array().cast< Scalar >() ;
}

void Octree::rebuild()
{

	Index nTrees = m_dim.prod() ;
	m_trees.resize( nTrees );

	Index offset = 0 ;
	for( SubTree& tree : m_trees) {
		tree.offset( offset ) ;
		offset += tree.nLeafs() ;
	}

	Index maxRes = 1 << m_maxDepth ;

	m_nodeIds.clear();
	for( auto it = cellBegin() ; it != cellEnd() ; ++it ) {
		const Cell& cell = *it ;
		ArrWi hires ; Index size ;
		subtree( cell ).upres_cell( cell[WD], maxRes, hires, size );
		hires += maxRes * cell.head<WD>() ;

		for( Index k = 0 ; k < Voxel::NV ; ++k ) {
			const ArrWi corner = Voxel::corner( k ) ;
			const Index nodeIdx = hrNodeIndex( hires + size*corner, maxRes ) ;

			if( m_nodeIds.find(nodeIdx) == m_nodeIds.end() ) {
				const size_t idx = m_nodeIds.size() ;
				m_nodeIds[nodeIdx] = idx ;
			}
		}
	}
}

void Octree::list_nodes(const Cell &cell, NodeList &nodes) const
{
	Index maxRes = 1 << m_maxDepth ;

	ArrWi hires ; Index size ;
	subtree( cell ).upres_cell( cell[WD], maxRes, hires, size );
	hires += maxRes * cell.head<WD>() ;

	for( Index k = 0 ; k < Voxel::NV ; ++k ) {
		const ArrWi corner = Voxel::corner( k ) ;
		const Index nodeIdx = hrNodeIndex( hires + size*corner, maxRes ) ;
		const auto it = m_nodeIds.find(nodeIdx) ;
		assert( it != m_nodeIds.end() ) ;
		nodes[k] = it->second ;
	}
}

void Octree::locate(const Vec &x, Location &loc) const
{
	Vec rel = x.array()/m_dx.array() ;
	loc.cell.head<WD>() = rel.cast< Index >();
	clamp_cell( loc.cell) ;

	rel -= loc.cell.head< WD >().cast< Scalar >().matrix() ;

	subtree(loc.cell).find( rel, loc.cell[WD], loc.coords );
}

void Octree::get_geo( const Cell &cell, CellGeo& geo ) const
{
	subtree(cell).compute_geo(cell[WD], geo);
	geo.box.array()    *= m_dx ;
	geo.origin.array() *= m_dx ;
	geo.origin += firstCorner( cell.head<WD>() ) ;
}

bool Octree::split(const Cell &cell)
{
	return subtree(cell).split(cell[WD], m_maxDepth) ;
}

/////////////////////////
////////////////////////

struct Octree::SubTree::SubCell {
	Index res ;
	ArrWi coords ;
};

Octree::SubTree::SubTree()
	: m_offset(0), m_nodes{{1,1}}
{}

Index Octree::SubTree::nLeafs() const
{
	return m_nodes[0].nLeafs ;
}

void Octree::SubTree::find( const Vec& pos, Index &leafIndex, Coords& coords ) const  {
	Arr rel = pos ;
	leafIndex = 0 ;
	find( 0, rel, leafIndex, coords ) ;
}

void Octree::SubTree::compute_geo( Index leafIndex, Voxel &geo ) const  {
	SubCell cell ;
	subcell( leafIndex, cell, 0, 0 ) ;
	geo.origin = cell.coords.cast< Scalar >() / cell.res ;
	geo.box    = Vec::Constant(1./cell.res) ;
}

void Octree::SubTree::upres_cell( Index leafIndex, Index target_res, ArrWi& hires_cell, Index &size ) const
{
	SubCell cell ;
	subcell( leafIndex, cell, 0, 0 ) ;
	size = target_res / cell.res ;
	hires_cell = cell.coords * size ;
}

Index Octree::SubTree::res( Index leafIndex ) const
{
	SubCell cell ;
	subcell( leafIndex, cell, 0, 0 ) ;
	return cell.res ;
}

void Octree::SubTree::find( Index offset, Arr& rel, Index &leafIndex, Coords& coords ) const
{
//	std::cout << offset << " "
//			  << m_nodes[offset].subSize << " "
//			  << m_nodes[offset].nLeafs << " -> "
//			  << leafIndex << " "
//			  << std::endl ;

	if( m_nodes[ offset ].subSize == 1 ) {
		coords = rel ;
	} else {
		ArrWi sub = (2*rel).cast< Index >().min(1) ;
		Index subi = Voxel::cornerIndex( sub ) ;

		++offset ;
		for (Index i = 0 ; i < subi ; ++i) {
			leafIndex += m_nodes[ offset ].nLeafs ;
			offset    += m_nodes[ offset ].subSize ;
		}

		rel = 2*rel - sub.cast<Scalar>() ;

		find( offset, rel, leafIndex, coords ) ;
	}
}

void Octree::SubTree::subcell ( Index leafIndex, SubCell& cell,
			   Index offset = 0, Index leafOffset = 0 ) const
{
	const Node& cur = m_nodes[offset] ;
	assert( leafIndex < leafOffset + cur.nLeafs ) ;
	if( cur.subSize == 1 ) {
		cell.res = 1 ;
		cell.coords.setZero() ;
	} else {
		++offset ;

		Index subi = 0 ;
		while( leafOffset + m_nodes[ offset ].nLeafs <= leafIndex ) {
			++ subi ;
			leafOffset += m_nodes[ offset ].nLeafs ;
			offset     += m_nodes[ offset ].subSize ;
		}

		const ArrWi sub = Voxel::corner( subi ) ;
		subcell( leafIndex, cell, offset, leafOffset ) ;

		cell.coords += cell.res * sub ;
		cell.res *= 2 ;
	}
}

bool Octree::SubTree::split(Index leafIndex, Index maxDepth )
{
	Index offset = 0 ;
	bool ok = split( leafIndex, offset, 0, maxDepth ) ;

	if( !ok ) return false ;

	const std::size_t orig_size = m_nodes.size() ;
	m_nodes.resize( orig_size + Voxel::NV );
	std::copy_backward( m_nodes.begin()+offset+1, m_nodes.begin() + orig_size, m_nodes.end());

	for( Index k = 0 ; k < Voxel::NV ; ++k ) {
		m_nodes[ offset+1+k ].subSize = 1 ;
		m_nodes[ offset+1+k ].nLeafs  = 1 ;
	}

	return true ;
}

bool Octree::SubTree::split ( Index leafIndex,  Index &offset,
							  Index leafOffset, Index maxDepth )
{
	if( 0 == maxDepth ) return false ;

	Node& cur = m_nodes[offset] ;

//	std::cout << offset << " [spli] "
//			  << cur.subSize<< " "
//			  << cur.nLeafs<< " "
//			  << std::endl ;

	if( cur.subSize > 1 ) {
		++offset ;

		while( leafOffset + m_nodes[ offset ].nLeafs <= leafIndex ) {
			leafOffset += m_nodes[ offset ].nLeafs ;
			offset     += m_nodes[ offset ].subSize ;
		}

		if( ! split( leafIndex, offset, leafOffset, maxDepth-1 ) )
			return false ;

	}

	cur.subSize += Voxel::NV ;
	cur.nLeafs  += Voxel::NV - 1 ;

	return true ;

}


}
