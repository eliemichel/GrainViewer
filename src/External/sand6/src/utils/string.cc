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

#include "string.hh"

#include <algorithm>
#include <iostream>

#define WHITE_SPACE " \t\r\v\n\f"

using namespace std ;

namespace d6 {

string trim ( const string& str )
{
	return trim( str, WHITE_SPACE ) ;
}

string trim ( const string& str, const string& chars )
{
	size_t first = str.find_first_not_of( chars ) ;
	size_t last = str.find_last_not_of( chars ) ;

	if( first == string::npos || first > last ) return string() ;
	return str.substr( first, 1 + last - first ) ;
}

string to_lower ( const string& str )
{
	string res = str ;
	transform( str.begin(), str.end(), res.begin(), ::tolower );
	return res ;
}

string to_upper ( const string& str )
{
	string res = str ;
	transform( str.begin(), str.end(), res.begin(), ::toupper );
	return res ;
}

float to_float ( const std::string& str )
{
	return to_num< float > ( str ) ;
}

double to_double ( const std::string& str )
{
	return to_num< double > ( str ) ;
}

int to_int ( const std::string& str )
{
	return to_num< int > ( str ) ;
}

unsigned to_uint ( const std::string& str )
{
	return to_num< unsigned > ( str ) ;
}

size_t to_size_t ( const std::string& str )
{
	return to_num< size_t > ( str ) ;
}


  //! Converts a String to a boolean value
  /*!
   * False values are "false", "f", "no" and "0", case insensitive
   * Every other strings convert to true
   */
bool to_bool ( const std::string &str )
{
	const std::string& s = to_lower( trim ( str ) ) ;
	return !( s.empty() || s == "false" || s == "f" || s == "0" || s == "no" );
}

std::string canonicalize( const std::string& str )
{
	std::vector< std::string > res ;
	split( str, res ) ;

	return to_lower( join( res ) ) ;
}

void split ( const std::string& str, std::vector< std::string > &res )
{
	split( str, WHITE_SPACE, res ) ;
}

void split ( const std::string& str, const std::string &separators, std::vector< std::string > &res )
{
	size_t pos = 0 ;
	while( string::npos != ( pos = str.find_first_not_of( separators, pos ) ) )
	{
	   const size_t next_sep = str.find_first_of( separators, pos ) ;

	   if( string::npos == next_sep )
	   {
		   res.push_back( str.substr( pos ) );
		   break ;
	   }

	   res.push_back( str.substr( pos, next_sep - pos ) );
	   pos = next_sep ;
	}
}

std::string join ( const std::vector< std::string > &src )
{
	return join( src, " " ) ;
}

std::string join ( const std::vector< std::string > &src, const std::string &separator )
{
	if( src.empty() ) return "" ;

	std::stringstream stream( std::stringstream::out ) ;
	for( unsigned k = 0 ; ; ++ k )
	{
		stream << src[k] ;

		if( k+1 == src.size() ) break ;
		stream << separator ;
	}
	return stream.str() ;
}

bool starts_with( const std::string &src, const std::string &start )
{
	if( start.size() > src.size() ) return false ;
	return 0 == src.compare( 0, start.size(), start ) ;
}

bool ends_with( const std::string &src, const std::string &end )
{
	if( end.size() > src.size() ) return false ;
	return 0 == src.compare( src.size() - end.size(), string::npos, end ) ;
}

bool split_on_next_marker ( const std::string &src, std::string &first_part, std::string &second_part )
{
	size_t lm_start = 0, lm_end = 0, lm_idx = string::npos ;

	size_t pos = 0 ;
	while( string::npos != ( pos = src.find( '%', pos ) ) && ( pos+1 ) < src.size() )
	{
		if( src[pos+1] == '%' )
		{
		  pos += 2 ;
		  if( pos == src.size() ) break ;
		  else                    continue ;
		}

		second_part = src.substr( pos+1, 64 ) ;
		istringstream is ( second_part ) ;
		size_t marker_idx ;
		is >> marker_idx ;

		if( marker_idx < lm_idx )
		{
		  lm_start = pos ;
		  lm_end = pos + 1 + second_part.size() - is.rdbuf()->in_avail() ;
		  lm_idx = marker_idx ;
		}

		++pos ;
	}

	if( lm_idx == string::npos )
	{
		return false ;
	}

	first_part = src.substr( 0, lm_start ) ;
	second_part = src.substr( lm_end ) ;

	return true ;
}

} //namespace d6

