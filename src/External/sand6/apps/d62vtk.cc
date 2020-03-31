
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

#include "utils/string.hh"

#include "visu/Offline.hh"
#include "visu/VTKParticlesWriter.hh"
#include "visu/VTKFieldWriter.hh"

#include "mono/Phase.hh"

#include "geo/FieldBase.impl.hh"
#include "geo/ScalarField.hh"
#include "geo/TensorField.hh"

#include <iostream>

static void usage( const char *name )
{
	std::cout << "Usage: " << name
			  << " [sim_dir=out] [options] "
			  << "\nTransform raw simulation output into standard VTK files,"
			  << "\n sim_dir/vtk/fields-[frame_id].vtk and "
			  << "\n sim_dir/vtk/particles-[frame_id].vtk if the '-p' flag is provided"
			  << "\n\n" ;

	std::cout << "Options:\n"
			  << "-? \t Display this help message and exit\n"
			  << "-n frame_id \t Jump to frame frame_id\n"
			  << "-a \t Process all subsequent frames\n"
			  << "-p \t Create VTK file for particles as well \n"
			  << std::endl ;
}

void dump_frame( const d6::Offline& offline, bool particles,
				 const char* base_dir, unsigned frame )
{

	if(particles) {
		d6::VTKParticlesWriter particlesWriter( base_dir, offline.particles() ) ;
		particlesWriter.startFile( "particles", frame ) ;
		particlesWriter.dump_all() ;
	}

	{
		d6::VTKFieldWriter<d6::PrimalShape> fieldWriter( base_dir, offline.meshes().primal() ) ;
	//	fieldWriter.setMode( d6::VTKWriter::Ascii );
		fieldWriter.startFile( "primal-fields", frame ) ;
		fieldWriter.dump(    "phi", offline.grains().fraction ) ;
		fieldWriter.dump(      "u", offline.grains().velocity ) ;
		fieldWriter.dump(  "d_phi", offline.grains().grad_phi ) ;
		fieldWriter.dump( "forces", offline.grains().fcontact ) ;

		d6::PrimalTensorField tau = offline.grains().stresses.interpolate<d6::PrimalShape>(
					offline.meshes().primal()) ;
		fieldWriter.dump("stresses", tau ) ;
	}

	{
#ifdef D6_UNSTRUCTURED_DUAL
		d6::VTKParticlesWriter fieldWriter( base_dir, offline.particles() ) ;
#else
		d6::VTKFieldWriter<d6::DualShape> fieldWriter( base_dir, offline.meshes().primal() ) ;
#endif
		fieldWriter.startFile( "dual-fields", frame ) ;

		d6::DualScalarField p   = offline.grains().stresses.trace() ;
		d6::DualScalarField dh  = offline.grains().sym_grad.trace() ;
		d6::DualScalarField taun= d6::DualTensorField( offline.grains().stresses.deviatoricPart() ).norm() ;

		fieldWriter.dump(    "p", p ) ;
		fieldWriter.dump(   "dh", dh ) ;
		fieldWriter.dump( "taun", taun ) ;
	}

	//	fieldWriter.dump( "lambda", offline.grains().stresses ) ;
}

int main( int argc, const char* argv[] ) {

	const char * base_dir = "out" ;
	unsigned frame = 0 ;

	bool all = false ;

	bool particles = false ;

	for( int i = 1 ; i < argc ; ++i )
	{
		if( argv[i][0] == '-' ){
			switch(argv[i][1]) {
			case '?':
				usage(argv[0]) ;
				return 0 ;
			case 'a':
				all = true ;
				break ;
			case 'p':
				particles = true ;
				break ;
			case 'n':
				if( ++i == argc ) break ;
				frame = d6::to_uint( argv[i] ) ;
				break ;
			}
		} else {
			base_dir = argv[i] ;
		}
	}

	d6::Offline offline( base_dir ) ;

	unsigned cur_frame = frame ;

	do {
		if(! offline.load_frame( cur_frame ) )
			return all?0:1 ;

		dump_frame( offline, particles, base_dir, cur_frame++ ) ;

	} while( all ) ;

	return (frame == cur_frame) ? 1 : 0 ;

}
