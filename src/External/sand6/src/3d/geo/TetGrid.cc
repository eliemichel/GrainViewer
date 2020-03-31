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

#include "geo/TetGrid.hh"

#include "geo/BoundaryInfo.hh"

namespace d6 {

TetGridIterator& TetGridIterator::operator ++()
{
    ++cell[3] ;
    if(cell[3] == 6) {
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

Index TetGridIterator::index() const {
    return grid.cellIndex( cell ) ;
}

TetGrid::TetGrid(const Vec &box, const Vec3i &res, const Particles *)
    : Base()
{
    m_dim = res ;
    m_dx = box.array()/res.array().cast< Scalar >() ;
}

Index TetGrid::nEdges() const {
    assert(false && "not implemented") ;
    return -1 ; //TODO ( for P2-3D)
}

void TetGrid::locate(const Vec &x, Location &loc) const
{
    Vec pos = x.array()/m_dx ;

    loc.cell.segment<3>(0) =
            pos.cast< Index >().array().max( ArrWi::Zero() ).min(m_dim-ArrWi::Ones()) ;

//	pos -= loc.cell.segment<3>(0).cast< Scalar >() ;

    Tet geo ;
    for( loc.cell[3] = 0 ; loc.cell[3] < 6 ; ++loc.cell[3] )
    {
        get_geo(  loc.cell, geo ) ;
        geo.compute_coords( x, loc.coords );

        if( loc.coords.minCoeff() >= -1.e-12 )
            break ;
    }
}

Index TetGrid::nAdjacent( Index idx ) const
{
    Vec3i node ;
    node[0] = idx / ( (m_dim[2]+1) * (m_dim[1]+1) ) ;
    idx -= node[0] * (m_dim[2]+1) * (m_dim[1]+1) ;
    node[1] = idx / (m_dim[2]+1) ;
    node[2] = idx - node[1]*(m_dim[2]+1)  ;

    // (0,0,1) et (1m,0,0)
    return ( ( (node[0]%2) == (node[1]%2) ) && ( (node[0]%2) != (node[2]%2) ) )
            ? 48 : 16 ;
}

void TetGrid::get_geo( const Cell &cell, CellGeo& geo ) const {
    const int color = (cell[0]%2) + (cell[1]%2) * 2 + (cell[2]%2) * 4 ;


    get_corner( cell.segment<3>(0), geo.origin );
    geo.box = m_dx ;

    geo.update_geometry( color, cell[3] ) ;

}

void TetGrid::boundaryInfo( const Location &loc, const BoundaryMapper& mapper, BoundaryInfo &info ) const
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
