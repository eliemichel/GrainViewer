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

#include "geo/LevelSet.hh"

#include "LevelSet.impl.hh"

#include <iostream>

namespace d6 {

LevelSet::LevelSet()
	: m_origin(Vec::Zero()), m_frame( Rotation::Identity() )
{
	scale(1.) ;
}

void LevelSet::move(const Vec &depl, const Rotation &rot)
{
	m_origin += depl ;
	m_frame   = rot * m_frame ;
}

Scalar LevelSet::eval_at(const Vec &x) const
{
	Vec loc;
	to_local(x, loc);
	return m_scale * eval_local( loc ) ;
}

void LevelSet::grad_at(const Vec &x, Vec &grad) const
{
	Vec loc;
	to_local(x, loc);
	grad = m_frame * ( grad_local( loc ).array() ) ;
}

void LevelSet::to_local(const Vec &world, Vec &local) const
{
//	Quaternion fi = m_frame ; fi.w() = -fi.w() ;
	local = ( m_frame.inverse() * ( world - m_origin ) ) / m_scale  ;
}
void LevelSet::to_local_mat( Mat &mat) const
{
	mat = m_frame.inverse().matrix() / m_scale ;
}

void LevelSet::inv_inertia(Mat66 &Mi) const
{
	Mi.setIdentity() ;
	Mi.diagonal().head<WD>() /= local_volume() ;

	Mat I ;
	local_inv_inertia( I );
	Mat w2l ;
	to_local_mat( w2l );

	Mi.block<WD,WD>(SD-WD,SD-WD) = w2l.transpose() * I * w2l ;


	Mi *= std::pow( m_scale, -WD ) ;
}

LevelSet::Ptr LevelSet::make_sphere() { return Ptr( new SphereLevelSet() ) ; }
LevelSet::Ptr LevelSet::make_cylinder(Scalar height) { return Ptr( new CylinderLevelSet( height ) ) ; }
LevelSet::Ptr LevelSet::make_plane() { return Ptr( new PlaneLevelSet() ) ; }
LevelSet::Ptr LevelSet::make_torus(Scalar radius) { return Ptr( new TorusLevelSet( radius ) ) ; }
LevelSet::Ptr LevelSet::make_hole(Scalar radius) { return Ptr( new HoleLevelSet( radius ) ) ; }
LevelSet::Ptr LevelSet::make_hourglass(Scalar height, Scalar radius) { return Ptr( new HourglassLevelSet( height, radius ) ) ; }
LevelSet::Ptr LevelSet::from_mesh(const char* objFile) { return Ptr( new MeshLevelSet( objFile ) ) ; }

} //d6
