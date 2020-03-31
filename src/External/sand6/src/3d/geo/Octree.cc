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

#include "geo/Octree.hh"

#include "geo/BoundaryInfo.hh"

namespace d6 {

OctreeIterator& OctreeIterator::operator ++()
{
    if( ++cell[3] == grid.nSubCells( cell ) ) {
        cell[3] = 0 ;
        ++cell[2] ;
        if(cell[2] == grid.dim()[2]) {
            cell[2] = 0 ;
            ++cell[1] ;
            if(cell[1] == grid.dim()[1]) {
                cell[1] = 0 ;
                ++cell[0] ;
            }
        }
    }

    return *this ;
}


void Octree::boundaryInfo( const Location &loc, const BoundaryMapper& mapper, BoundaryInfo &info ) const
{
    constexpr Scalar eps = 1.e-6 ;

    const Vec &p = pos( loc ) ;
    const Vec &b = box() ;

    info.bc = BoundaryInfo::Interior ;

    if( p[0] < eps )
        info.combine( mapper( "left"), Vec(-1,0,0) ) ;
    if( p[0] > b[0] - eps )
        info.combine( mapper("right"), Vec( 1,0,0) ) ;

    if( p[1] < eps )
        info.combine( mapper("front"), Vec(0,-1,0) ) ;
    if( p[1] > b[1] - eps )
        info.combine( mapper( "back"), Vec(0, 1,0) ) ;

    if( p[2] < eps )
        info.combine( mapper("bottom"), Vec(0,0,-1) ) ;
    if( p[2] > b[2] - eps )
        info.combine( mapper(   "top"), Vec(0,0, 1) ) ;
}



}
