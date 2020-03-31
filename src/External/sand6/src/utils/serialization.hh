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

#ifndef D6_SERIALIZATION_HH
#define D6_SERIALIZATION_HH

#include <boost/serialization/array.hpp>
#include <bogus/Core/Eigen/EigenSerialization.hpp>

namespace boost
{
namespace serialization
{

template<typename Archive, typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
inline void load(
       Archive & ar,
       Eigen::Array<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> & matrix,
       const unsigned int file_version
   )
{
    static_assert( _Rows != Eigen::Dynamic, "Dynamic array serial not implemented" ) ;
    static_assert( _Cols != Eigen::Dynamic, "Dynamic array serial not implemented" ) ;
    (void) file_version ;
    ar & make_array( matrix.data(), matrix.size() ) ;
}

template<typename Archive, typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
inline void save(
       Archive & ar,
       const Eigen::Array<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> & matrix,
       const unsigned int file_version
   )
{
    static_assert( _Rows != Eigen::Dynamic, "Dynamic array serial not implemented" ) ;
    static_assert( _Cols != Eigen::Dynamic, "Dynamic array serial not implemented" ) ;
    (void) file_version ;
    ar & make_array( matrix.data(), matrix.size() ) ;
}

template<typename Archive, typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
inline void serialize(
       Archive & ar,
       Eigen::Array<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> & matrix,
       const unsigned int file_version
   )
{
    split_free( ar, matrix, file_version ) ;
}

} //serialization
} //boost

#endif
