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

#include "Config.hh"

#include "utils/string.hh"
#include "utils/Log.hh"

#include <cmath>

#include <fstream>
#include <regex>

namespace d6 {

template< typename dest_t >
void rescale( dest_t &, Scalar ) { }

template< >
void rescale( Scalar & src, Scalar s ) { src *= s ; }
template< int D >
void rescale( Eigen::Matrix<Scalar, D, 1> &src, Scalar s ) { src *= s ; }


Config::Config() :
	fps(240), substeps(1), nFrames( 1 ),
	box( Vec::Ones() ), res( VecWi::Constant(10) ),
	nSamples(2), randomize( 0 ),
	volMass( 1.5e3 ),
	viscosity( 1.e-3 ),
	gravity( Vec::Zero() ),
	phiMax(1), mu(0),
	delta_mu( 0 ), I0( 0.4 ), grainDiameter( 1.e-3 ),
	muRigid( 0.5 ),
	cohesion(0), cohesion_decay(0),
	anisotropy( 0 ), elongation( 1 ), brownian( 0 ),
	initialOri( Vec::Constant(1./3) ),
	enforceMaxFrac( false ), weakStressBC( false ),
	usePG( false ), useInfNorm( D6_DIM == 2 ),
	boundary("cuve"),
	output( true ), exportAllFields( bool(3-D6_DIM) ), dumpPrimalData( 0 )
{
	gravity[WD-1] = -9.81 ;
}

void Config::internalize()
{
	m_units.setTypical( (box.array()/res.array().cast<Scalar>()).minCoeff(), gravity.norm(), volMass );

	#define CONFIG_FIELD( name, type, u ) rescale( name, m_units.fromSI( u ) ) ;
	EXPAND_CONFIG
	#undef CONFIG_FIELD
}

bool Config::from_string(const std::string& key, const std::string &val)
{
	std::istringstream iss ( val ) ;
	return from_string( key, iss ) ;
}

bool Config::from_string(const std::string& key, std::istringstream &val )
{
	bool f = false ;
	#define CONFIG_FIELD( name, type, s ) \
		if(key == D6_stringify(name)) { cast(val, name) ; f = true ; }
	EXPAND_CONFIG
	 #undef CONFIG_FIELD

	if( !f ) Log::Warning() << "Warning: '" << key << "' is not a valid config field" << std::endl;
	return f ;
}

bool Config::from_file(const std::string &file_name)
{
	std::ifstream in( file_name ) ;
	if(!in)
		return false ;

	std::string line, key ;

	while( std::getline( in, line ))
	{
		std::istringstream iss ( line ) ;
		if ( (iss >> key) && key[0] != '#' )
		{
			from_string( key, iss ) ;
		}
	}

	return true ;
}

bool Config::dump(const std::string &file_name, const char *comment ) const
{
	std::ofstream out( file_name ) ;
	if(!out) {
		Log::Error() << "Could not wrtie config file " << file_name << std::endl ;
		return false ;
	}

	if(comment) {
		out << "# " << comment << std::endl ;
	}

	#define CONFIG_FIELD( name, type, s ) \
		out << D6_stringify(name) << "\t" ; d6::dump( out, name ) ; out << "\n" ;
	EXPAND_CONFIG
	 #undef CONFIG_FIELD

	return true ;
}

} //ns hyb2d

