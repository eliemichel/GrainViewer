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

#ifndef D6_VTK_WRITER_HH
#define D6_VTK_WRITER_HH

#include <string>
#include "utils/File.hh"

namespace d6 {

class VTKWriter {

public:
	enum Mode { Ascii, Binary } ;

	void setMode( Mode mode ) { m_mode = mode ; }
	Mode mode() const { return m_mode ; }

	bool startFile( const char* name, unsigned frame ) ;

protected:

	explicit VTKWriter( const char* base_dir ) ;
	virtual ~VTKWriter() {}

	virtual void writeMesh( File &file ) const = 0 ;
	virtual size_t nDataPoints() const = 0 ;

	//! Write attribute header + attribute data
	template< typename Scalar >
	void writeAttribute( const char* name, const Scalar* data, const int Dim ) ;

	//! Writes raw data
	template< typename Scalar >
	void write( File &file, const Scalar* data, const int Dim, const size_t size ) const ;

	File m_file ;

private:

	std::string fileName(  const unsigned frame, const char* dataName ) const ;
	bool open( const unsigned frame, const char* dataName ) ;

	void writeHeader( File &file, const char *title ) const ;
	void writeAttributeHeader( File &file, const int Dim, const char* name ) const ;

	const char* m_base_dir ;
	Mode m_mode ;

};

} //d6

#endif
