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

#ifndef D6_BOUNDARY_INFO_HH
#define D6_BOUNDARY_INFO_HH

#include "utils/alg.hh"

#include <string>
#include <vector>

#include <unordered_map>

namespace d6 {

struct BoundaryInfo {

	enum Bc {
		Interior,
		Stick,   // u = 0
		Slip,    // u.n = 0, d( (I - nn') u ).n = 0, (I - nn')sn = 0
		Normal,  // (I -nn') u = 0, d ( u.n ).n = 0, (nn')sn = 0
		Free,    // d( u ).n = 0, sn = 0
		Corner
	};

	Bc bc ;
	Vec normal ;

	BoundaryInfo() :
		bc( Interior )
	{}

	BoundaryInfo( const Bc bc_, const Vec n )
		: bc(bc_), normal(n)
	{}

	void set( const Bc bc_, const Vec n )
	{
		bc = bc_ ;
		normal = n ;
	}

	void combine( const Bc bc_, const Vec n ) ;
	static BoundaryInfo combine(const BoundaryInfo &b1, const BoundaryInfo &b2 ) ;

	void    velProj( Mat  &proj ) const ;
	void   spinProj( MatR &proj ) const ;
	void stressProj( MatS &proj ) const ;

};

typedef std::vector< BoundaryInfo > BoundaryConditions ;

struct BoundaryMapper {
	virtual BoundaryInfo::Bc operator() ( const std::string &/*domain*/ ) const
	{ return BoundaryInfo::Stick ; }
};

class StrBoundaryMapper : public BoundaryMapper
{
public:

	explicit StrBoundaryMapper( const std::string & str ) ;

	virtual BoundaryInfo::Bc operator() ( const std::string &domain ) const ;

private:

	BoundaryInfo::Bc from_string( const std::string &bc ) ;

	typedef std::unordered_map< std::string, BoundaryInfo::Bc > Map ;
	Map m_bc ;
};

} //ns d6

#endif
