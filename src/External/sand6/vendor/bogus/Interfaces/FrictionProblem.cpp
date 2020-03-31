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

#include "FrictionProblem.impl.hpp"

#include "../Core/BlockSolvers/GaussSeidel.impl.hpp"
#include "../Core/BlockSolvers/ProjectedGradient.impl.hpp"


namespace bogus {

// DualFrictionProblem

template< unsigned Dimension >
void DualFrictionProblem< Dimension >::computeFrom(const PrimalFrictionProblem<Dimension> &primal )
{

	//W
	W = primal.H * ( primal.MInv * primal.H.transpose() ) ;

	// M^-1 f, b
	b = primal.E.transpose() * Eigen::VectorXd::Map( primal.w, primal.H.rows())
	        - primal.H * ( primal.MInv * Eigen::VectorXd::Map( primal.f, primal.H.cols() ) );

	mu = Eigen::VectorXd::Map( primal.mu, W.rowsOfBlocks() ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::solveWith( GaussSeidelType &gs, double *r,
                                         const bool staticProblem ) const
{
	gs.setMatrix( W );

	return friction_problem::solve( *this, gs, r, staticProblem ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::solveWith( ProjectedGradientType &pg,
                                                    double *r, const bool staticProblem ) const
{
	pg.setMatrix( W );

	return friction_problem::solve( *this, pg, r, staticProblem ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::evalWith( const GaussSeidelType &gs,
                                                     const double *r,
                                                     const bool staticProblem ) const
{
	return friction_problem::eval( *this, gs, r, staticProblem ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::evalWith( const ProjectedGradientType &gs,
                                                   const double *r, const bool staticProblem ) const
{
	return friction_problem::eval( *this, gs, r, staticProblem) ;
}


template< unsigned Dimension >
double DualFrictionProblem< Dimension >::solveCadoux(GaussSeidelType &gs, double *r, const unsigned cadouxIterations,
        const Signal<unsigned, double> *callback ) const
{
	gs.setMatrix( W );

	return friction_problem::solveCadoux( *this, gs, r, cadouxIterations, callback ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::solveCadoux(ProjectedGradientType &pg, double *r, const unsigned cadouxIterations,
        const Signal<unsigned, double> *callback ) const
{
	pg.setMatrix( W );

	return friction_problem::solveCadoux( *this, pg, r, cadouxIterations, callback ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::solveCadouxVel(GaussSeidelType &gs, double *u, const unsigned cadouxIterations,
        const Signal<unsigned, double> *callback ) const
{
	gs.setMatrix( W );

	return friction_problem::solveCadouxVel( *this, gs, u, cadouxIterations, callback ) ;
}

template< unsigned Dimension >
double DualFrictionProblem< Dimension >::solveCadouxVel(ProjectedGradientType &pg, double *u, const unsigned cadouxIterations,
        const Signal<unsigned, double> *callback ) const
{
	pg.setMatrix( W );

	return friction_problem::solveCadouxVel( *this, pg, u, cadouxIterations, callback ) ;
}

template< unsigned Dimension >
void DualFrictionProblem< Dimension >::applyPermutation(
        const std::vector<std::size_t> &permutation)
{
	assert( !permuted() ) ;

	m_permutation = permutation ;

	m_invPermutation.resize( m_permutation.size() );
	for( std::size_t i = 0 ; i < m_permutation.size() ; ++i )
		m_invPermutation[ m_permutation[i] ] = i ;

	W.applyPermutation( data_pointer(m_permutation) ) ;
	friction_problem::applyPermutation< Dimension >( m_permutation, b, W.colOffsets() ) ;
	bogus::applyPermutation( m_permutation.size(), data_pointer(m_permutation), mu ) ;
}

template< unsigned Dimension >
void DualFrictionProblem< Dimension >::undoPermutation()
{
	if( !permuted() )
		return ;

	W.applyPermutation( data_pointer(m_invPermutation) ) ;
	friction_problem::applyPermutation< Dimension >( m_invPermutation, b, W.colOffsets() ) ;
	bogus::applyPermutation( m_invPermutation.size(), data_pointer(m_invPermutation), mu ) ;

	m_permutation.clear() ;
}

#ifdef BOGUS_INSTANTIATE_2D_SOC
template struct DualFrictionProblem< 2u > ;
#endif

#ifdef BOGUS_INSTANTIATE_3D_SOC
template struct DualFrictionProblem< 3u > ;
#endif

#ifdef BOGUS_INSTANTIATE_DYNAMIC_SOC
template struct DualFrictionProblem< Eigen::Dynamic > ;
#endif

} //namespace bogus
