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

#ifndef D6_STRING_HH
#define D6_STRING_HH

#include <string>
#include <vector>
#include <sstream>

#include "alg.hh"

namespace d6 {

//! Left- and Right-trim \p res, removing all the characters in \p chars
std::string trim ( const std::string& str, const std::string& chars ) ;
//! Same as trim( str, all_white_space_characters )
std::string trim ( const std::string& str ) ;

std::string to_lower ( const std::string& str ) ;
std::string to_upper ( const std::string& str ) ;

float       to_float  ( const std::string& str ) ;
double      to_double ( const std::string& str ) ;
int         to_int    ( const std::string& str ) ;
unsigned    to_uint   ( const std::string& str ) ;
std::size_t to_size_t ( const std::string& str ) ;
bool        to_bool   ( const std::string& str ) ;


//! Trim a string, replace all whitespace sequences with a single space, and puts the result in lowercase
std::string canonify( const std::string& str ) ;
//! Same thing, with less frenglish
std::string canonicalize( const std::string& str ) ;

//! Explodes \p src into an array \p res, splitting on any character that is present in \p separators
void split ( const std::string& str, const std::string &separators, std::vector< std::string > &res ) ;
//! Same as split( src, all_white_space_characters )
void split ( const std::string& str, std::vector< std::string > &res ) ;

//! \return a string which is the concatenation of all elements of \p src separated by \p separator
std::string join ( const std::vector< std::string > &src, const std::string &separator ) ;
//! Same as join( src, ' ' )
std::string join ( const std::vector< std::string > &src ) ;

//! \return whether src starts with \p start
bool starts_with( const std::string &src, const std::string &start ) ;
//! \return whether src ends with \p end
bool ends_with( const std::string &src, const std::string &end ) ;

// Poor-man version of QString::arg
bool split_on_next_marker ( const std::string &src, std::string &first_part, std::string &second_part ) ;

//!
template< typename T >
std::ostream& farg( std::ostream &os, const std::string &src, const T &replacement )
{
	std::string p1, p2 ;
	if( split_on_next_marker( src, p1, p2 ) )
	{
		os << p1 << replacement << p2 ;
	}
	return os ;
}

template< typename T >
std::stringstream& farg( std::stringstream &os, const T &replacement )
{
	std::string src = os.str() ;
	os.str("") ;
	farg< T >( os, src, replacement ) ;
	return os ;
}

template< typename T >
std::string arg( const std::string &src, const T &replacement )
{
	std::stringstream os( std::stringstream::out ) ;
	os.str( src );
	return farg<T>( os, replacement).str() ;
}
//! Same with two arguments
template< typename T1, typename T2 >
std::string arg( const std::string &src, const T1 &r1, const T2& r2 )
{
	std::stringstream os( std::stringstream::out ) ;
	os.str( src );
	return farg< T2 >( farg< T1 >( os, r1 ), r2 ).str() ;
}

//! Same with three arguments
template< typename T1, typename T2, typename T3 >
std::string arg3( const std::string &src, const T1 &r1, const T2& r2, const T3& r3 )
{
	std::stringstream os( std::stringstream::out ) ;
	os.str( src );
	return farg< T3 >( farg< T2 >( farg< T1 >( os, r1 ), r2 ), r3 ).str() ;
}

// Templated comversion functions using operator>>

template< typename T >
bool cast ( std::istringstream& stream, T& res )
{
	return static_cast<bool>( stream >> res ) ;
}

template< >
inline bool cast ( std::istringstream& stream, std::string& res )
{
	std::string remaining ;

	if( ! std::getline( stream, remaining ) )
		return false ;
	res = trim( remaining );
	return true ;
}

template< typename Scalar, int Rows, int Cols >
bool cast ( std::istringstream& stream, Eigen::Matrix<Scalar, Rows, Cols>& res )
{
	for( int k = 0 ; stream && ( k < res.size() ) ; ++ k) {
		stream >> res.data()[k] ;
	}
	return static_cast<bool>( stream ) ;
}

template< typename T >
bool cast ( const std::string& str, T& res )
{
	std::istringstream stream( str ) ;
	return cast( stream, res ) ;
}

template< typename T >
std::ostream& dump ( std::ostream& stream, const T& val )
{
	return ( stream << val ) ;
}

template< typename Scalar, int Rows, int Cols >
std::ostream& dump ( std::ostream& stream, const Eigen::Matrix<Scalar, Rows, Cols>& val )
{
	for( int k = 0 ; stream && ( k < val.size() ) ; ++ k) {
		stream << val.data()[k] << "\t" ;
	}
	return stream ;
}

template< typename NumType >
NumType to_num ( const std::string& str )
{
	NumType n ;
	return cast( str, n ) ? n : 0 ;
}

} //namespace d6


#define D6_stringify(s) D6_preproc_str(s)
#define D6_preproc_str(s) #s

#endif
