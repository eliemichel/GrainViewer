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

#include "visu/Offline.hh"

#include "utils/Log.hh"

#include <iostream>

using namespace d6 ;

static void usage( const char *name )
{
	std::cout << "Usage: " << name
			  << " [sim_dir=out] [options] "
			  << "\nGenerates discharge curves."
			  << "\n\n" ;

	std::cout << "Options:\n"
			  << "-? \t Display this help message and exit\n"
			  << "-n frame_id \t Stop at frame frame_id\n"
			  << std::endl ;
}

static Scalar remaining_volume( const Particles& particles, const Vec& box )
{
	const Scalar zmin = box[2] / 2 ;

	Scalar volume = 0 ;
#pragma omp parallel for reduction(+:volume)
	for( unsigned i = 0 ; i < particles.count() ; ++i ) {
		if( particles.centers()(2,i) > zmin ) {
			volume += particles.volumes()[i] ;
		}
	}

	return volume ;
}

int main( int argc, const char* argv[] ) {


	const char * base_dir = "out" ;
	unsigned last_frame = -1 ;

	for( int i = 1 ; i < argc ; ++i )
	{
		if( argv[i][0] == '-' ){
			switch(argv[i][1]) {
			case '?':
				usage(argv[0]) ;
				return 0 ;
			case 'n':
				if( ++i == argc ) break ;
				last_frame = d6::to_uint( argv[i] ) ;
				break ;
			}
		} else {
			base_dir = argv[i] ;
		}
	}

	Log::Config::get().level = Log::L_Error ; ;

	Offline offline( base_dir ) ;
	const Config &config = offline.config() ;

	for( unsigned cur_frame = 0 ;
		 ( last_frame > cur_frame ) && offline.load_frame( cur_frame ) ;
		 ++cur_frame )
	{
		const Scalar t =config.time( cur_frame ) ;
		const Scalar vol = remaining_volume( offline.particles(), config.box )  ;
		std::cout << cur_frame << "\t" << t << "\t" << (t * config.units().toSI( Units::Time ))
				  << "\t" << vol << "\t" << (vol * config.units().toSI( Units::Volume ) )
				  << "\n" ;
	}

	return 1 ;

}
