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

#ifndef D6_ROTATIONS_HH
#define D6_ROTATIONS_HH

#include "utils/alg.hh"
#include <Eigen/Geometry>

namespace d6 {

template< Index Dim >
struct RotationTraits
{
    typedef Scalar Type ;
    static Type rotate( const Type& base, const Type& other ) {
        return base + other ;
    }
    static Vec rotate( const Type& rot, const Vec& vec ) {
        const Scalar c = std::cos( rot ) ;
        const Scalar s = std::sin( rot ) ;
        return Vec( c*vec[0] - s*vec[1], s*vec[0] + c*vec[1] )  ;
    }
    static Type from_vel( const Scalar& angVel, Scalar dt ) {
        return angVel * dt ;
    }
};
template< >
struct RotationTraits<3>
{
    typedef Eigen::Quaternion< Scalar, Eigen::DontAlign > Type ;
    static Type rotate( const Type& base, const Type& other ) {
        return base * other ;
    }
    static Vec3 rotate( const Type& rot, const Vec3& vec ) {
        return rot * vec ;
    }
    static Type from_vel( const Vec3& angVel, Scalar dt ) {
        const Scalar avn = angVel.norm() ;
        Vec3 axis = avn > 1.e-12
                ? Vec3(angVel/avn)
                : Vec3(1,0,0) ;
        return Type( Eigen::AngleAxis< Scalar >( dt * avn, axis ) );
    }
};

typedef typename RotationTraits<WD>::Type Rotation ;
inline Rotation rotate( const Rotation& base, const Rotation& other )
{ return RotationTraits<WD>::rotate(base,other) ; }
inline Vec rotate( const Rotation& rot, const Vec& vec )
{ return RotationTraits<WD>::rotate(rot,vec) ; }
inline Rotation vel_rotation( const VecR& vel, Scalar dt )
{ return RotationTraits<WD>::from_vel(vel, dt) ; }

} //d6

#endif
