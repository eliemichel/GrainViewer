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

#ifndef D6_ALG_HH
#define D6_ALG_HH

#include "Segmenter.hh"

namespace d6 {

#if D6_DIM==2
static constexpr Index WD = 2 ;
static constexpr Index SD = 3 ;
static constexpr Index RD = 1 ;
#else
static constexpr Index WD = 3 ;
static constexpr Index SD = 6 ;
static constexpr Index RD = 3 ;
#endif

typedef Eigen::Matrix< Scalar, WD, WD > Mat ;
typedef Eigen::Matrix< Scalar, WD,  1 > Vec ;
typedef Eigen::Array < Scalar, WD,  1 > Arr ;

typedef Eigen::Matrix< Scalar, SD, SD > MatS ;
typedef Eigen::Matrix< Scalar, SD,  1 > VecS ;
typedef Eigen::Array < Scalar, SD,  1 > ArrS ;

typedef Eigen::Matrix< Scalar, RD, RD > MatR ;
typedef typename Segmenter<RD>::ValueType VecR ;

typedef Eigen::Matrix< Index, WD, 1 > VecWi ;
typedef Eigen::Array < Index, WD, 1 > ArrWi ;

typedef Eigen::Matrix< Scalar, 3, 3 > Mat33 ;
typedef Eigen::Matrix< Scalar, 3, 1 > Vec3 ;
typedef Eigen::Array < Scalar, 3, 1 > Arr3 ;

typedef Eigen::Matrix< Index, 3, 1 > Vec3i ;
typedef Eigen::Array < Index, 3, 1 > Arr3i ;

typedef Eigen::Matrix< Scalar, 6, 6 > Mat66 ;
typedef Eigen::Matrix< Scalar, 6, 1 > Vec6 ;

typedef Eigen::Matrix< Scalar, WD, Eigen::Dynamic > DynMatW ;
typedef Eigen::Matrix< Scalar, SD, Eigen::Dynamic > DynMatS ;
typedef Eigen::Matrix< Scalar, 3, Eigen::Dynamic > DynMat3 ;
typedef Eigen::Matrix< Scalar, 6, Eigen::Dynamic > DynMat6 ;
typedef Eigen::Matrix<  Index, Eigen::Dynamic, Eigen::Dynamic > DynMati;


} //d6

#endif
