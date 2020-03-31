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

#include "UnstructuredShapeFunction.hh"

#include "Grid.hh"
#include "Particles.hh"

#include "geo/Tensor.hh"

#include <Eigen/Eigenvalues>

namespace d6 {

UnstructuredDOFs::UnstructuredDOFs(const Vec &box, const VecWi &res, const Particles *particles)
	: vertices( particles->centers() ), m_count( particles->count() ),
	  m_box(box), m_res(res), m_particles( particles )
{
	rebuild();
}

void UnstructuredDOFs::rebuild()
{
	if( m_particles )
		compute_weights_from_particles();
	else
		compute_weights_from_vertices();
}

void UnstructuredDOFs::compute_weights_from_vertices()
{
	static constexpr Index K = WD+1 ;

	const Index n = count() ;
	weights.setConstant( vertices.cols(), 0 ) ;

	Grid g( m_box, m_res ) ;
	std::vector< std::vector< Index > > ids( g.nCells() ) ;

	for( Index i = 0 ; i < n ; ++i ) {
		Grid::Location loc ;
		g.locate( vertices.col(i), loc) ;
		ids[ g.cellIndex(loc.cell) ].push_back( i ) ;
	}

#pragma omp parallel for
	for( Index i = 0 ; i < n ; ++i ) {
		Grid::Location loc ;
		g.locate( vertices.col(i), loc) ;

		Eigen::Matrix< Scalar, K, 1 > dist ;
		dist.setOnes() ;

		g.each_neighbour( loc.cell, [&]( const Grid::Cell& nb ) {
			for ( Index pid : ids[g.cellIndex(nb)] ) {
				if( pid == i ) continue ;
				Scalar d2 = ( vertices.col(i) - vertices.col(pid) ).squaredNorm() ;

				for( Index n = K-1 ; n >= 0 && d2 < dist(n) ; --n ) {
					if( n + 1 < K ) dist(n+1) = dist(n) ;
					dist(n) = d2 ;
				}
			}
		} ) ;

		weights(i) = std::pow( dist.prod(), (.5/K)*WD ) ;
//		weights(i) = std::pow( dist.array().sqrt().sum()/K, WD ) ;

	}


}

void UnstructuredDOFs::compute_weights_from_particles()
{
	assert( m_particles ) ;

	const Index n = count() ;
	weights.resize( n ) ;

#pragma omp parallel for
	for( Index i = 0 ; i < n ; ++i ) {

		auto  frame_view( tensor_view( m_particles->frames().col(i) ) ) ;
		Mat frame ;
		frame_view.get( frame ) ;

		weights[i] = std::sqrt( std::max(1.e-8, frame.determinant() ) ) * (1<<WD) ;

	}

}

} //d6
