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

#ifndef D6_CONFIG_HH
#define D6_CONFIG_HH

#include "utils/units.hh"
#include "utils/alg.hh"

#include <string>

namespace d6 {


#define EXPAND_CONFIG \
	CONFIG_FIELD( fps				, Scalar		,	Units::Frequency	) \
	CONFIG_FIELD( substeps			, unsigned		,	Units::None			) \
	CONFIG_FIELD( nFrames			, unsigned		,	Units::None			) \
	\
	CONFIG_FIELD( box				, Vec			,	Units::Length		) \
	CONFIG_FIELD( res				, VecWi			,	Units::None			) \
	CONFIG_FIELD( nSamples			, unsigned		,	Units::None			) \
	CONFIG_FIELD( randomize			, Scalar		,	Units::None			) \
	\
	CONFIG_FIELD( volMass			, Scalar		,	Units::VolumicMass	) \
	CONFIG_FIELD( viscosity			, Scalar		,	Units::Viscosity	) \
	CONFIG_FIELD( gravity			, Vec			,	Units::Acceleration	) \
	CONFIG_FIELD( phiMax			, Scalar		,	Units::None			) \
	\
	CONFIG_FIELD( mu				, Scalar		,	Units::None			) \
	\
	CONFIG_FIELD( delta_mu			, Scalar		,	Units::None			) \
	CONFIG_FIELD( I0				, Scalar		,	Units::None			) \
	CONFIG_FIELD( grainDiameter		, Scalar		,	Units::Length		) \
	CONFIG_FIELD( muRigid			, Scalar		,	Units::None			) \
	\
	CONFIG_FIELD( cohesion			, Scalar		,	Units::Stress		) \
	CONFIG_FIELD( cohesion_decay	, Scalar		,	Units::None			) \
	\
	CONFIG_FIELD( anisotropy		, Scalar		,	Units::None			) \
	CONFIG_FIELD( elongation		, Scalar		,	Units::None			) \
	CONFIG_FIELD( brownian			, Scalar		,	Units::None			) \
	CONFIG_FIELD( initialOri		, Vec			,	Units::None			) \
	\
	CONFIG_FIELD( enforceMaxFrac	, bool			,	Units::None			) \
	CONFIG_FIELD( weakStressBC		, bool			,	Units::None			) \
	CONFIG_FIELD( usePG				, bool			,	Units::None			) \
	CONFIG_FIELD( useInfNorm		, bool			,	Units::None			) \
	\
	CONFIG_FIELD( scenario			, std::string	,	Units::None			) \
	CONFIG_FIELD( boundary			, std::string	,	Units::None			) \
	CONFIG_FIELD( output			, bool			,	Units::None			) \
	CONFIG_FIELD( exportAllFields	, bool			,	Units::None			) \
	CONFIG_FIELD( dumpPrimalData	, unsigned		,	Units::None			) \


struct Config
{
	Config( ) ;

	bool from_string( const std::string &key, const std::string &value ) ;
	bool from_file(const std::string& file_name) ;

	bool dump( const std::string& file_name, const char* comment = nullptr ) const ;

	//! Transform all the parameters from SI to internal units
	void internalize() ;

	const Units& units() const {
		return m_units ;
	}

	Scalar time( unsigned frame_nb ) const {
		return (frame_nb / fps ) ;
	}

#define CONFIG_FIELD( name, type, u ) \
	type name ;
	EXPAND_CONFIG
#undef CONFIG_FIELD

private:
	bool from_string( const std::string &key, std::istringstream &value ) ;

	Units m_units ;

} ;

} //ns hyb2d

#endif

