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

#include "Voxel.hh"

#include "Tensor.hh"

namespace d6 {

Index Voxel::sample_uniform(const unsigned N, const Index start, Points &points, Frames &frames) const
{
	Scalar min = box.minCoeff() ;

	Vec3i Nsub ;
	for( int k = 0 ; k < 3 ; ++ k)
		Nsub[k] = N * std::round( box[k] / min ) ;

	const Vec subBox = box.array() / Nsub.array().cast< Scalar >() ;

	Vec6 frame ;
	tensor_view( frame ).set_diag( Vec( .25 * subBox.array() * subBox.array() ) ) ;

	Index p = start ;
	for( int i = 0 ; i < Nsub[0] ; ++i )
		for( int j = 0 ; j < Nsub[1] ; ++j )
			for( int k = 0 ; k < Nsub[2] ; ++k ) {
				points.col(p) = origin + (Vec(i+.5,j+.5,k+.5).array() * subBox.array()).matrix() ;
				frames.col(p) = frame ;
				++p ;
			}

	return p - start ;
}

typename QuadraturePoints<Voxel, 2>::QuadPoints QuadraturePoints<Voxel, 2>::Qps()
{
	// .5 * ( 1 +- 1./sqrt(3) )
	const Vec dqp = Vec::Constant( 1./sqrt(3.) );
	//		const Vec dqp = Vec::Constant( 1. );
	const Vec qp0 = .5 * ( Vec::Ones() - dqp );

	QuadPoints qps ;
	for( int i = 0 ; i < 2 ; ++i ) {
		for( int j = 0 ; j < 2 ; ++j ) {
			for( int k = 0 ; k < 2 ; ++k ) {
				Vec3i corner ( i, j, k) ;
				qps.col( Voxel::cornerIndex(corner) ) = qp0.array() + corner.cast< Scalar >().array()*dqp.array() ;
			}
		}
	}
	return qps ;
}

} //d6
