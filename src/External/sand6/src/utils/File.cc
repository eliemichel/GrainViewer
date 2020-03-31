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

#include "File.hh"

#include "string.hh"
#include "Log.hh"

#include <fstream>

#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>
#endif // WIN32

using namespace std ;


namespace d6 {

File::File( const std::string& name )
	: fstream(), _name( name ), _mode( ios_base::in | ios_base::out  )
{
}

File::File( const std::string& name, const std::ios_base::openmode mode )
	: fstream( name.c_str(), mode ), _name( name ), _mode( mode )
{
}

bool File::exists( ) const
{
	return exists( _name ) ;
}

bool File::exists( const std::string& path )
{
	return FileInfo( path ).exists() ;
}

bool File::open( std::ios_base::openmode mode )
{
	if( is_open() ) return false ;

	_mode = mode ;
	fstream::open( _name.c_str(), _mode ) ;

	return is_open() ;
}

bool File::open( const std::string& name, const std::ios_base::openmode mode )
{
	setName( name ) ;
	return open( mode ) ;
}

bool File::get_contents( string &contents)
{
	if( !*this )
		return false ;

	std::stringstream sstr ;
	std::string line ;
	while( std::getline( *this, line ))
	{
		sstr << line << '\n' ;
	}

	contents = sstr.str() ;

	return true ;
}


#ifdef WIN32
const char* FileInfo::DIR_SEP = "\\" ;
#else
const char* FileInfo::DIR_SEP = "/" ;
#endif
const char* FileInfo::DIR_CUR = ".";
const char* FileInfo::DIR_ROOT = "";

FileInfo::FileInfo( File& file )
{
	setPath( file.name() ) ;
}

FileInfo::FileInfo( const std::string &path )
{
	setPath( path ) ;
}

void FileInfo::setPath( const std::string& path )
{
	_path = path ;
}

bool FileInfo::exists() const
{
	struct stat info ;
	return ! stat( _path.c_str(), &info ) ;
}

bool FileInfo::isDir() const
{
	struct stat info ;
	if( -1 == stat( _path.c_str(), &info ) )
	{
		print_error( "stat", errno );
		return false ;
	}

	return 0 != ( info.st_mode & S_IFDIR );

}

void FileInfo::getAPath( std::vector< std::string >& path_elements ) const
{
   if( starts_with( _path, DIR_SEP ) ) path_elements.push_back( "" ); //Root
   split( _path, DIR_SEP, path_elements ) ;
}


bool FileInfo::makePath( ) const
{
	const FileInfo& parentDir = parentDirectory() ;
	if( parentDir.exists() ) return true ;

	std::vector< std::string > directories ;
	parentDir.getAPath( directories );

	std::ostringstream curPath ;

	bool ok = true ;

	for ( unsigned dirIndex = 0 ; ok && dirIndex < directories.size() ; ++dirIndex )
	{
		if( dirIndex ) curPath << DIR_SEP ;
		curPath << directories[ dirIndex ] ;

		FileInfo fi( curPath.str() ) ;
		if( !fi.exists() ) {
			ok = fi.makeDir() ;
		}

	}

	return ok ;

}

bool FileInfo::makeDir( ) const
{
	if( _path.empty() ) return true ;

#ifdef WIN32
	if( 0 == CreateDirectory(  _path.c_str(), NULL ) )
	{
	  print_error( "CreateDirectory", GetLastError(), true ) ;
	  return false ;
	}
#else
	if( -1 == mkdir( _path.c_str(), 0755 ) )
	{
		print_error( "mkdir", errno );
		return false ;
	}
#endif
	return true ;
}

FileInfo FileInfo::parentDirectory() const
{
   size_t pos = _path.rfind( DIR_SEP ) ;

   if( pos == string::npos )
   {
	   return FileInfo( DIR_CUR );
   }
   return FileInfo( _path.substr( 0, pos ) );
}

std::string FileInfo::name() const
{
   size_t pos = _path.rfind( DIR_SEP ) ;

   if( pos == string::npos )
   {
	   return _path ;
   }
   return _path.substr( pos+1 ) ;
}

std::string FileInfo::absolutePath() const
{
	std::string res ;
#ifdef WIN32
	static const DWORD BUF_SIZE = 4096 ;
	TCHAR resolved[ BUF_SIZE ] ;
	DWORD retval = GetFullPathName( _path.c_str(),
					 BUF_SIZE,
					 resolved,
					 NULL);
	if( 0 == retval )
	{
	  print_error( "GetFullPathName", GetLastError(), true ) ;
	} else {
	  res = resolved ;
	}
#else
	char * resolved = realpath( _path.c_str(), NULL ) ;

	if( ! resolved )
	{
		print_error( "realpath", errno ) ;
	} else {
		res = resolved ;
		free( resolved ) ;
	}
#endif
	return res ;
}

std::string FileInfo::virtualAbsolutePath() const
{
	if( exists() ) return absolutePath() ;

	size_t pos = string::npos ;

	while ( pos && string::npos != ( pos = _path.rfind( DIR_SEP, pos ) ) )
	{
		FileInfo dir( _path.substr( 0, pos ) ) ;
		if( dir.exists() )
		{
		   return FileInfo( dir.absolutePath() ).filePath( _path.substr( pos + 1 ) ) ;
		}

		if( pos ) --pos ;
	}
	return _path ;
}

std::string FileInfo::filePath( const std::string& fileName ) const
{
	return _path + DIR_SEP + fileName ;
}

void FileInfo::print_error( const char *func, int errnum, bool win32_error ) const
{
	Log::Error() << "FileInfo::"<< func << " on " << _path << ": "  ;
	if( win32_error )
	{
	  Log::Error() << errnum  ;
	} else {
	  Log::Error() << strerror( errnum )  ;
	}
	Log::Error() << std::endl ;
}


} //namespace d6


