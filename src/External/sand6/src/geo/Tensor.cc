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

#include "geo/Tensor.hh"

#include <unsupported/Eigen/MatrixFunctions>

namespace d6
{

void compute_anisotropy_matrix( const Mat& A, MatS & Abar )
{

	Abar.setIdentity() ;

	VecS taubar ;
	Mat tau, ani_tau ;

	for( int k = 1 ; k < SD ; ++k ) {
		taubar.setZero() ;
		taubar[k] = 1. ;

		tensor_view( taubar ).get( tau ) ;
		ani_tau = A * tau * A ;

		tensor_view( Abar.col(k) ).set( ani_tau );
		Abar(0,k) = 0 ;
	}

}

void compute_convection_matrix( const Mat& A, const Scalar dt, MatS & Aexp )
{
	MatS Abar ;

	VecS taubar ;
	Mat tau, conv_tau ;

	for( int k = 0 ; k < SD ; ++k ) {
		taubar.setZero() ;
		taubar[k] = 1. ;

		tensor_view( taubar ).get( tau ) ;
		conv_tau = A * tau + tau * A.transpose() ;

		tensor_view( Abar.col(k) ).set( conv_tau );
	}

	Aexp = (dt * Abar).exp() ;
}

} //d6
