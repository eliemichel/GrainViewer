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

#ifndef D6_RIGID_BODY_HH
#define D6_RIGID_BODY_HH

#include "utils/alg.hh"
#include <memory>

namespace d6 {

class LevelSet ;

class RigidBody
{
public:

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	typedef Eigen::Matrix< Scalar, WD+RD, 1, Eigen::DontAlign > Dofs ;

	typedef typename Segmenter<WD, Dofs>::Seg      VelComp ;
	typedef typename Segmenter<WD, Dofs>::ConstSeg ConstVelComp ;
	typedef typename Segmenter<RD, Dofs>::Seg      AngVelComp ;
	typedef typename Segmenter<RD, Dofs>::ConstSeg ConstAngVelComp ;

	RigidBody( std::unique_ptr< LevelSet > &ls, Scalar volMass ) ;

	// Accessors

	Vec velocity_at( const Vec& x ) const ;

	const Dofs &velocities() const {
		return m_velocity ;
	}

	VelComp velocity() { return Segmenter<WD, Dofs>::head( m_velocity ) ; }
	AngVelComp angularVelocity() { return Segmenter<RD, Dofs>::tail( m_velocity ) ; }

	ConstVelComp velocity() const { return Segmenter<WD, Dofs>::head( m_velocity ) ; }
	ConstAngVelComp angularVelocity() const { return Segmenter<RD, Dofs>::tail(m_velocity ) ; }

	const LevelSet& levelSet() const
	{ return *m_levelSet ; }
	const LevelSet* levelSetPtr() const
	{ return m_levelSet.get() ; }

	Scalar volumicMass() const
	{
		return m_volumicMass ;
	}

	void inv_inertia( MatS& Mi ) const ;

	// Modifiers

	void set_velocity( const Vec& vel, const VecR& angularVel )
	{
		velocity() = vel ;
		angularVelocity() = angularVel ;
	}

	void integrate_gravity( const Scalar dt, const Vec& gravity ) ;
	void integrate_forces( const Scalar dt, const Dofs& forces ) ;

	void move( const Scalar dt ) const ;

	void move_to( const Vec& pos ) const ;

private:
	std::unique_ptr< LevelSet > m_levelSet ;

	Scalar m_volumicMass ;

	Dofs m_velocity ;

} ;

} //d6


#endif
