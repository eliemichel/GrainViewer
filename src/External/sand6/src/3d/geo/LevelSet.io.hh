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

#ifndef D6_LEVEL_SET_IO_HH
#define D6_LEVEL_SET_IO_HH

#include "LevelSet.impl.hh"

#include <boost/serialization/base_object.hpp>

namespace d6 {

// save/load base class information
template<class Archive>
void PlaneLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
}
template<class Archive>
void SphereLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
}
template<class Archive>
void TorusLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
	ar & m_radius ;
}
template<class Archive>
void HourglassLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
	ar & m_height ;
	ar & m_radius ;
}
template<class Archive>
void HoleLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
	ar & m_radius ;
}
template<class Archive>
void CylinderLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
	ar & m_height ;
}
template<class Archive>
void MeshLevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & boost::serialization::base_object<LevelSet>(*this);
	ar & m_objFile ;
}


// register derived class ptrs
template<class Archive>
void LevelSet::register_derived(Archive &ar )
{
	ar.register_type(static_cast<   SphereLevelSet *>(NULL));
	ar.register_type(static_cast<    PlaneLevelSet *>(NULL));
	ar.register_type(static_cast<    TorusLevelSet *>(NULL));
	ar.register_type(static_cast< CylinderLevelSet *>(NULL));
	ar.register_type(static_cast<     MeshLevelSet *>(NULL));
	ar.register_type(static_cast<     HoleLevelSet *>(NULL));
	ar.register_type(static_cast<HourglassLevelSet *>(NULL));
}

//base class serilization
template<class Archive>
void LevelSet::serialize(Archive &ar, const unsigned int )
{
	ar & m_origin ;
	ar & m_scale ;
	ar & m_frame.w() & m_frame.x() & m_frame.y() & m_frame.z() ;
}


} //d6

#endif
