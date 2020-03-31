/*
 * This file is part of So-bogus, a C++ sparse block matrix library and
 * Second Order Cone solver.
 *
 * Copyright 2013 Gilles Daviet <gdaviet@gmail.com>
 *
 * So-bogus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * So-bogus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with So-bogus.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef BOGUS_FISCHER_BURMEISTER_IMPL_HPP
#define BOGUS_FISCHER_BURMEISTER_IMPL_HPP

#include "FischerBurmeister.hpp"

#include <iostream>
namespace bogus {

template< DenseIndexType Dimension, typename Scalar >
void FBBaseFunction< Dimension, Scalar >::compute(
	 const Scalar mu, const Vector& x, const Vector& y, Vector& fb )
{
	static Matrix a,b ; //unused

	if( NumTraits< Scalar >::isZero( mu ) )
	{
		Traits::np( fb ) = x[0] + y[0] - std::sqrt( x[0]*x[0] + y[0]*y[0] ) ;
		Traits::tp( fb ) = Traits::tp( x ) ;
	} else {
		Vector xh = x, yh = y ;
		Traits::np( xh ) *= mu ;
		Traits::tp( yh ) *= mu ;
		compute< false >( xh, yh, fb, a, b ) ;
	}
}

template< DenseIndexType Dimension, typename Scalar >
void FBBaseFunction< Dimension, Scalar >::computeJacobian(
		const Scalar mu, const Vector& x, const Vector& y,
		Vector& fb, Matrix& dFb_dx, Matrix& dFb_dy )
{
	if( NumTraits< Scalar >::isZero( mu ) )
	{
		const Scalar z = std::sqrt( x[0]*x[0] + y[0]*y[0] ) ;
		Traits::np( fb ) = x[0] + y[0] - z ;
		Traits::tp( fb ) = Traits::tp( x ) ;

		dFb_dx.setIdentity() ;
		dFb_dy.setZero() ;

		if( !NumTraits< Scalar >::isZero( z ) )
		{
			dFb_dx(0,0) = 1. - x[0] / z ;
			dFb_dy(0,0) = 1. - y[0] / z ;
		}
	} else {
		Vector xh = x, yh = y ;
		Traits::np( xh ) *= mu ;
		Traits::tp( yh ) *= mu ;
		compute< true >( xh, yh, fb, dFb_dx, dFb_dy ) ;
		Traits::nc( dFb_dx ) *= mu ;
		Traits::tc( dFb_dy ) *= mu ;
	}

}

template< DenseIndexType Dimension, typename Scalar >
template< bool JacobianAsWell >
void FBBaseFunction< Dimension, Scalar >::compute(
			const Vector& x, const Vector& y, Vector& fb,
			Matrix& dFb_dx, Matrix& dFb_dy )
{
	const unsigned d = x.rows() ;

	// see [Daviet et al 2011], Appendix A.1

	Vector z2(d) ;

	z2[0] = x.squaredNorm() + y.squaredNorm() ;
	Traits::tp( z2 ) = x[0] * Traits::tp( x ) + y[0] * Traits::tp( y ) ;
	const Scalar nz2t = Traits::tp( z2 ).norm() ;

	Vector omega1(d), omega2(d) ;
	omega1[0] = omega2[0] = .5 ;

	if( NumTraits< Scalar >::isZero( nz2t ) )
	{
		Traits::tp( omega1 ).setZero() ;
		omega1[1] = -.5 ;
		Traits::tp( omega2 ).setZero() ;
		omega2[1] =  .5 ;
	} else {
		Traits::tp( omega1 ) = -.5* Traits::tp( z2 ) / nz2t ;
		Traits::tp( omega2 ) =  .5* Traits::tp( z2 ) / nz2t ;
	}

	const Scalar rlambda1 = std::sqrt( std::max( (Scalar) 0, z2[0] - 2*nz2t ) ) ;
	const Scalar rlambda2 = std::sqrt( z2[0] + 2*nz2t ) ;

	const Vector z = rlambda1 * omega1 + rlambda2 * omega2 ;
	fb = x + y - z ;

	if( JacobianAsWell )
	{
		const Matrix Id( Matrix::Identity( d, d ) ) ;

		if( NumTraits< Scalar >::isZero( rlambda2 ) )
		{
			// x = y = 0
			dFb_dx.setZero( d,d ) ;
			dFb_dy.setZero( d,d ) ;
		} else {
			if( NumTraits< Scalar >::isZero( rlambda1 ) )
			{
				const Scalar izn = 1. / ( x[0]*x[0] + y[0]*y[0] ) ;
				dFb_dx = ( 1. - x[0]*izn ) * Id ;
				dFb_dy = ( 1. - y[0]*izn ) * Id ;
			} else {
				const Scalar det = rlambda1 * rlambda2 ;

				Matrix L, invLz(d,d) ;

				invLz(0, 0) = z[0] ;
				Traits::ntb( invLz ) = -Traits::tp( z ).transpose() ;
				Traits::tnb( invLz ) = -Traits::tp( z ) ;
				Traits::ttb( invLz ) =
					( det * Traits::TgMatrix::Identity( d-1, d-1 ) +
					  Traits::tp( z ) * Traits::tp( z ).transpose() ) / z[0] ;
				invLz /= det ;

				L = x[0] * Id ;
				Traits::ntb( L ) = Traits::tp( x ).transpose() ;
				Traits::tnb( L ) = Traits::tp( x ) ;
				dFb_dx.setIdentity( d,d ) ;
				dFb_dx.noalias() -= invLz * L ;

				L = y[0] * Id ;
				Traits::ntb( L ) = Traits::tp( y ).transpose() ;
				Traits::tnb( L ) = Traits::tp( y ) ;
				dFb_dy.setIdentity( d,d ) ;
				dFb_dy.noalias() -= invLz * L ;

			}
		}

	}

}

template< DenseIndexType Dimension, typename Scalar, bool DeSaxceCOV >
void FischerBurmeister< Dimension, Scalar, DeSaxceCOV >::
compute( const Scalar mu, const Vector& x, const Vector& y, Vector& fb )
{
  if ( DeSaxceCOV )
  {
	Vector yt ( y );
	Traits::np( yt ) += mu * Traits::tp( y ).norm() ;
	BaseFunction::compute( mu, x, yt, fb ) ;
  } else {
	BaseFunction::compute( mu, x, y, fb ) ;
  }
}

template< DenseIndexType Dimension, typename Scalar, bool DeSaxceCOV >
void FischerBurmeister< Dimension, Scalar, DeSaxceCOV >::compute(
	const Vector& x, Vector& fb ) const
{
  const Vector y  = m_A * x + m_b ;
  const Vector xs = m_scaling * x ;
  compute( m_mu, xs, y, fb ) ;
}

template< DenseIndexType Dimension, typename Scalar, bool DeSaxceCOV >
void FischerBurmeister< Dimension, Scalar, DeSaxceCOV >::computeJacobian(
	  const Vector& x, Vector& fb, Matrix& dFb_dx ) const
{
  const Vector xs = m_scaling * x ;
  Vector y ( m_A * x + m_b ) ;
  Scalar s = 0. ;
//  std::cout << m_A << std::endl ;
//  std::cout << m_b.transpose() << std::endl ;

  if ( DeSaxceCOV )
  {
	s = Traits::tp( y ).norm() ;
	Traits::np( y ) += m_mu * s ;
  }
//  std::cout << y.transpose() << std::endl ;

  Matrix dFb_dy ;
  BaseFunction::computeJacobian( m_mu, xs, y, fb, dFb_dx, dFb_dy ) ;
//  std::cout << dFb_dx.transpose() << std::endl ;
//  std::cout << dFb_dy.transpose() << std::endl ;

  if ( DeSaxceCOV && !NumTraits< Scalar >::isZero( s ) )
  {
	Traits::tc( dFb_dy ).noalias() +=
	  Traits::nc( dFb_dy ) *  ( m_mu / s ) * Traits::tp( y ).transpose() ;

  }

  dFb_dx *= m_scaling ;
  dFb_dx.noalias() += dFb_dy * m_A ;

}


}

#endif
