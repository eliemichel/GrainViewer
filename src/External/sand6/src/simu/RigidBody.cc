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

#include "RigidBody.hh"

#include "geo/LevelSet.hh"
#include "geo/Tensor.hh"

#include <iostream>

namespace d6 {

RigidBody::RigidBody(std::unique_ptr<LevelSet> &ls, Scalar volMass )
		: m_levelSet ( std::move(ls) ), m_volumicMass( volMass )
{
	m_velocity.setZero() ;
}

Vec RigidBody::velocity_at(const Vec &x) const
{
	Eigen::Matrix< Scalar, WD, RD > cross_mat ;
	make_cross_mat( x, cross_mat ) ;

	return velocity() + cross_mat * angularVelocity() ;
}

void RigidBody::integrate_forces(const Scalar dt, const Dofs &forces)
{
	MatS Mi ;
	inv_inertia( Mi );

	m_velocity += dt * Mi * forces ;
}

void RigidBody::integrate_gravity(const Scalar dt, const Vec &gravity)
{
	Dofs forces ;
	forces.head<WD>() = m_volumicMass * m_levelSet->volume() * gravity ;
	forces.tail<RD>().setZero() ;

	integrate_forces( dt, forces );
}

void RigidBody::move(const Scalar dt) const
{
	m_levelSet->move( dt * velocity(), vel_rotation( angularVelocity(), dt ) );
}

void RigidBody::move_to(const Vec &pos) const
{
	m_levelSet->set_origin( pos ) ;
}

void RigidBody::inv_inertia( MatS& Mi ) const
{
	m_levelSet->inv_inertia( Mi ) ;
	Mi /= m_volumicMass ;
}

} //d6
