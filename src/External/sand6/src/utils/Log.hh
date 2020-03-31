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

#ifndef LOG_D6_HPP
#define LOG_D6_HPP

/*!
 * \file Log.hpp
 * \brief Filtered, thread-safe, textual output mechanism
 * The stream will be exclusively owned by one thread until
 * it is flushed, for example by appending std::endl.
 *
 * /!\ Not flushing the streams will cause deadlocks !
 *
 */

#include "LogLevels.hh"
#include "string.hh"

#include <mutex>
#include <iostream>
#include <sstream>

namespace d6 {

//! Log-related definitions
/*! Usage
  \code
  Log::Info() << " This is message is relatively important" << std::endl ;
  Log::Debug()::as_std_ostream() << " This is much less so " << std::endl ;
  \endcode
  */
namespace Log
{

  //! Config : Defines verbosity level and output streams
  /*! Should be accessed only through Config::get() */
  struct Config
  {
	//! Normal output stream ( defaults to std::cout )
	std::ostream* out ;
	//! Error output stream ( defaults to std::cerr )
	std::ostream* err ;
	//! Output level ( defaults to L_All )
	Level level ;
	//! If true, do not prefix output with '[StreamLetter]'
	bool no_prefix ;

	//! Returns the static global instance
	static Config& get()
	{
	  static Config s_config ;
	  return s_config ;
	}

	void setLevel( const std::string& levelStr ) {
		#define D6_LOG_LEVEL( l ) \
			if(levelStr == D6_stringify( l ) ) { level = L_##l ; }
		D6_LOG_LEVELS
		#undef D6_LOG_LEVEL
	}

  private:
	//! Forbid construction outside of class
	Config() : out( NULL ), err(NULL), level( L_All ), no_prefix( false )
	{}

  } ;

  namespace log_impl {
  template < Level L > struct Log ;
  //! Should accessed directly with the typedefs below
  template < Level L > struct Stream {

	//! Filtered, thread safe operator<<
	template< typename T >
	Log< L >& operator<< (  const T& o ) const ;

	//! Filtered, thread safe operator<< for stream manipulators
	Log< L >& operator<< ( std::ostream& modifier( std::ostream& )  ) const ;

	//! Returns an almost thread safe std::ostream
	/*! Actually, the rarely used sputc() method of the streambuf won't be thread safe */
	static std::ostream& as_std_ostream( ) ;

	//! Thread-unsafe stream, with should not perform any blocking call to malloc
	static std::ostream& unsafe( ) ;

  private:
	static Log< L >& impl() ;
  } ;
  }

  //! Convenience typedef for each log level
#define D6_LOG_LEVEL( level ) typedef log_impl::Stream< L_##level > level ;
	D6_LOG_LEVELS
	typedef log_impl::Stream< L_Nothing > Nothing ;
#undef D6_LOG_LEVEL

  //! Full and abbreviated names
  template< Level L > struct  LevelNames {
	static const char* full() { return "Nothing" ; }
	static char abbrev() { return  'N' ; }
  } ;

  // Specializations for each Level
#define D6_LOG_LEVEL( level ) \
template< > struct  LevelNames< L_##level > { \
  static const char* full() { return #level ; } \
  static char abbrev() { return  #level[0] ; } \
} ;
  D6_LOG_LEVELS
#undef D6_LOG_LEVEL

  namespace log_impl
  {

   struct LogBase
   {

	 explicit LogBase( std::stringbuf *buf )
	   : stream( buf )
	 {
	   Config &c = Config::get() ;
	   if( !c.out )
	   {
		 c.out = &std::cout ;
	   }
	   if( !c.err )
	   {
		 c.err = &std::cerr ;
	   }
	 }

	template< Level L >
	 static const char * prefix( )
	 {
	  static const char s_prefix[5] = { '[', LevelNames<L>::abbrev(), ']', ' ', '\0' };

	  return ( Config::get().no_prefix ) ? "" : s_prefix ;
	 }

	 static std::recursive_mutex& global_mutex()
	 {
	   static std::recursive_mutex s_global ;
	   return s_global ;
	 }

	std::ostream stream ;

  } ;

  template < Level L   >
   struct Buf : public std::stringbuf {
	 std::ostream* &stream ;
	
	 std::recursive_mutex mutex ;
	 volatile bool locked ;

	 Buf( std::ostream* &stream_ptr )
	   : stream( stream_ptr )
	 {}

	 //! Called each time the stream is flushed ( e.g. for each std::endl )
	 virtual int sync ( )
	 {
	   {
		 //Output prefix + buffered string
		 std::lock_guard<std::recursive_mutex> lock( LogBase::global_mutex() ) ;
		 
		 *stream << LogBase::prefix<L>( ) << str();
		 stream->flush();
	   }

	   str("");

		// Release mutex
	   if( locked )
	   {
		 locked = false ;
		 mutex.unlock() ;
	   }

	   return 0;
	 }

	 virtual std::streamsize xsputn (const char* s, std::streamsize n)
	 {
	   acquire_mutex();
	   return std::stringbuf::xsputn( s, n ) ;
	 }

	 void acquire_mutex()
	 {
	   mutex.lock() ;
	   if( locked ) mutex.unlock() ; //This thread was already owning the recursive mutex, release it once

	   locked = true ;
	 }

   } ;

  template < Level L   >
  struct Log : public LogBase {
	Buf< L > buffer ;
	Log() : LogBase( &buffer ), buffer ( Config::get().out ) {}
	Log& thread_safe() { buffer.acquire_mutex() ; return *this ; }
	std::ostream& unsafe() { return  *buffer.stream << LogBase::prefix< L >() ; }
  } ;

  template < >
  struct Log< L_Error > : public LogBase {
	Buf< L_Error > buffer ;
	Log() : LogBase( &buffer ), buffer ( Config::get().err ) {}
	Log& thread_safe() { buffer.acquire_mutex() ; return *this ; }
	std::ostream& unsafe() { return *buffer.stream << LogBase::prefix< L_Error >() ; }
  } ;

  template < Level L, typename T >
  Log< L > & operator << ( Log< L > & s, const T& o )
  {
	if( L < Config::get().level ) return s ;
	s.thread_safe().stream << o ;
	return s ;
  }

  template < Level L >
  Log< L > & operator << ( Log< L > & s, std::ostream& modifier( std::ostream& )  )
  {
	if( L < Config::get().level ) return s ;
	s.thread_safe().stream << modifier ;
	return s ;
  }


  template < > struct Stream< L_Nothing >  {

	static std::ostream& as_std_ostream( )
	{
	  return impl() ;
	}

	static std::ostream& unsafe( )
	{
	  return as_std_ostream() ;
	}

private:

	struct black_hole_buf : public std::streambuf {
		std::streamsize xsputn (const char *, std::streamsize n) override {
			return n;
		}
		int overflow (int) override {
			return 1;
		}
	};
	
	static std::ostream& impl() 
	{
		static black_hole_buf s_buf ;
		static std::ostream s_stream(&s_buf) ;
		return s_stream ;
	}
  };


  //! Filtered, thread safe operator<<
  template< Level L >
  template< typename T >
  Log< L >& Stream< L >::operator<< (  const T& o ) const
  {
	return ( impl() << o ) ;
  }

  //! Filtered, thread safe operator<< for stream manipulators
  template< Level L >
  Log< L >& Stream< L >::operator<< ( std::ostream& modifier( std::ostream& )  ) const
  {
	return ( impl() << modifier ) ;
  }

  //! Returns an almost thread safe std::ostream
  /*! Actually, the rarely used sputc() method of the streambuf won't be thread safe */
  template< Level L >
  std::ostream& Stream< L >::as_std_ostream( )
  {
	if ( L < Config::get().level )
	  return Nothing::as_std_ostream() ;

	return impl().thread_safe().stream ;
  }

  //! Thread-unsafe stream, with should not perform any blocking call to malloc
  template< Level L >
  std::ostream& Stream< L >::unsafe( )
  {
	if ( L < Config::get().level )
	  return Nothing::as_std_ostream() ;

	return impl().unsafe() ;
  }

  template< Level L >
  Log< L >& Stream< L >::impl()
  {
	static Log< L > s_impl ;
	return s_impl ;
  }


  } // log_impl

} // Log

} // namespace d6



#endif // LOG_D6_HPP
