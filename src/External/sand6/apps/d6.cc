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

#include "utils/Config.hh"
#include "mono/MonoSimu.hh"

#include "utils/Log.hh"
#include "utils/File.hh"

#include <cstring>

namespace d6 {
	extern const char* g_git_branch ;
	extern const char* g_git_commit ;
	extern const char* g_timestamp  ;
}

static void usage( const char *name )
{
	std::cout << "Usage: " << name
			  << " [sim_dir=out] [options] "
			  << "\nGranular material simulator. "
			  << "\n Output files are created inside the directory specified by `sim_dir`, which defaults to 'out'. "
			  << "\n\n" ;

	std::cout << "Options:\n"
			  << "-? \t Display this help message and exit\n"
			  << "-i file \t Load a configuration file  \n"
			  << "-v level \t Specify the verbosity level \n"
			  << "-key value \t Set the configuration parameter 'key' to 'value' ( see README.md )  \n"
			  << std::endl ;
}

int main( int argc, const char* argv[] )
{
	d6::Config config ;

	const char * base_dir = "out" ;

	// Read coonfiguration from input files and CLI arguments
	for( int i = 1 ; i < argc ; ++i )
	{
		if( argv[i][0] == '-' ){
			if( std::strlen(argv[i]) > 2 ) {
				if( ++i == argc ) break ;
				config.from_string( argv[i-1]+1, argv[i] ) ;
			} else {
				switch(argv[i][1]) {
				case '?':
					usage( argv[0]) ;
					return 0;
				case 'i':
					if( ++i == argc ) break ;
					if( !config.from_file(argv[i]) ) {
						d6::Log::Error() << "Error reading file " << argv[i] << std::endl ;
					}
					break ;
				case 'v':
					if( ++i == argc ) break ;
					d6::Log::Config::get().setLevel( argv[i] ) ;
				}
			}
		} else {
			base_dir = argv[i] ;
		}
	}

	std::string info = d6::arg( d6::arg3("%1 %3 on %2 [%4]", argv[0], d6::g_git_branch, d6::g_git_commit ), d6::g_timestamp ) ;
	d6::Log::Info() << "This is " << info << std::endl ;

	// Save copy of final configuration and convert to interal units
	d6::FileInfo outDir ( base_dir ) ;
	if( !outDir.exists() ) outDir.makeDir() ;
	config.dump( outDir.filePath("config"), info.c_str() );
	config.internalize();

	d6::Log::Debug() << "1/Re = " << config.viscosity << std::endl ;

	// Run simulation
	d6::MonoSimu( config, base_dir ).run() ;

	return 0 ;
}
