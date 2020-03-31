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

#ifndef D6_FILE_HH
#define D6_FILE_HH

/*!
 * \file File.hh
 * \brief Utilities for manipulating files
 */

#include <string>
#include <vector>
#include <list>
#include <fstream>

namespace d6 {


class File : public std::fstream
{
public :
	//! Constructor defining the file name, without opening it
	explicit File( const std::string& name = "" ) ;
	//! Constructor defining the file name and opening it
	File( const std::string& name, const std::ios_base::openmode mode ) ;

	virtual ~File() {}

	bool exists() const ;

	bool open( )  { return open( _mode ) ; }
	bool open( const std::string& name ) { return open( name, _mode ) ; }
	bool open( std::ios_base::openmode mode ) ;
	bool open( const std::string& name, const std::ios_base::openmode mode ) ;

	void setName( const std::string& name )
	{
		if( !is_open() ) _name = name ;
	}
	const std::string &name( ) const
	{
		return _name ;
	}

	void setMode( std::ios_base::openmode mode )
	{
		if( !is_open() ) _mode = mode ;
	}

	static bool exists( const std::string& name ) ;

	bool get_contents( std::string& contents ) ;

private:
	std::string _name ;
	std::ios_base::openmode _mode ;
} ;


class FileInfo
{
public:
	const static char* DIR_SEP ;
	const static char* DIR_ROOT ;
	const static char* DIR_CUR ;

	explicit FileInfo( File& file ) ;
	explicit FileInfo( const std::string &path = "" ) ;

	void setPath( const std::string& path ) ;
	const std::string& path() const { return _path ; }

	bool exists() const ;
	bool isDir() const ;

	void getAPath( std::vector< std::string >& path_elements ) const ;
	bool makePath( ) const ;
	bool makeDir( ) const ;

	FileInfo parentDirectory() const;
	std::string name() const;

	//! Translates relative location and links in path, but /!\ works only for exising files
	std::string absolutePath() const;

	//! Same as absolutePath, but the file needs not exist
	std::string virtualAbsolutePath() const;

	std::string filePath( const std::string& fileName ) const ;

private:
	void print_error( const char* func, int errnum, bool win32_error = false ) const ;

	std::string _path ;
	std::string _absolutePath ;
} ;

} //namespace d6


#endif // FILE_MECHE_HPP
