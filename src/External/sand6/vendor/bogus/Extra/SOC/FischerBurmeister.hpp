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


#ifndef BOGUS_FISCHER_BURMEISTER_HPP
#define BOGUS_FISCHER_BURMEISTER_HPP

#include "../SecondOrder.fwd.hpp"

namespace bogus {

//! Binary Fischer-Burmeister function and jacobian computation
template< DenseIndexType Dimension, typename Scalar >
struct FBBaseFunction
{
	typedef LocalProblemTraits< Dimension, Scalar > Traits ;
	typedef typename Traits::Vector Vector ;
	typedef typename Traits::Matrix Matrix ;

	//! Computation of the FB function on the cone of aperture \p mu
	static void compute( const Scalar mu, const Vector& x, const Vector& y, Vector& fb ) ;

	//! Computation of the FB function with its jacobian on the cone of aperture \p mu
	static void computeJacobian(
				const Scalar mu, const Vector& x, const Vector& y,
			Vector& fb, Matrix& dFb_dx, Matrix& dFb_dy ) ;

	//! Computation of the FB function and (optionally) its jacobian on the canonical second order cone
	/*!
	  Computes \f$ fb := x + y - \left( x \circ x + y \circ y \right)^{ \frac 1 2 } \f$
	  according to \cite Fukushima02
	  \tparam JacobianAsWell If true, computes the jacobian matrices \p dFb_dx and \p dFb_dy
	  */
	template <bool JacobianAsWell >
	static void compute(
			const Vector& x, const Vector& y, Vector& fb,
			Matrix& dFb_dx, Matrix& dFb_dy ) ;

} ;

//! Fischer-Burmeister function and jacobian computation, with optional change of variable
/*! \tparam DeSaxceCOV whther to perform the DeSaxce change of variable. \sa SOCLaw */
template< DenseIndexType Dimension, typename Scalar, bool DeSaxceCOV >
class FischerBurmeister
{

public:
  typedef LocalProblemTraits< Dimension, Scalar > Traits ;
  typedef FBBaseFunction< Dimension, Scalar > BaseFunction ;
  typedef typename Traits::Vector Vector ;
  typedef typename Traits::Matrix Matrix ;

  //! Constructs an object modeling the function \f$ f : x \mapsto FB \left( mu, scaling \times x, A x + b \right) \f$
  FischerBurmeister(
	const Scalar mu,
	const Matrix& A,
	const Vector& b,
	const Scalar scaling )
	  : m_mu( mu ), m_scaling( scaling ), m_A( A ), m_b( b )
  {}

  //! Sets \f$ fb := f(x) \f$
  void compute( const Vector& x, Vector& fb ) const ;
  //! Sets \f$ fb := f(x) \f$ and \f$ dFb\_dx := \frac {dF} {dx} \f$
  void computeJacobian( const Vector& x, Vector& fb, Matrix& dFb_dx ) const ;

  //! Sets \f$ fb :=  FB \left( mu, x, y \right) \f$
  static void compute( const Scalar mu, const Vector& x, const Vector& y, Vector& fb ) ;

private:
  Scalar m_mu ;
  Scalar m_scaling ;
  const Matrix& m_A ;
  const Vector& m_b ;

} ;



}

#endif
