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

#ifndef D6_STATS_HH
#define D6_STATS_HH

#include "utils/alg.hh"
#include "utils/File.hh"

namespace d6 {

#define EXPAND_STAT \
	STAT_FIELD( stepId					, unsigned		, "step"	) \
	STAT_FIELD( frameId					, unsigned		, "frame"	) \
	STAT_FIELD( delta_t					, Scalar		, "dt"		) \
	STAT_FIELD( nParticles				, unsigned		, "nPart"	) \
	STAT_FIELD( nNodes					, unsigned		, "totNds"  ) \
	STAT_FIELD( nPrimalNodes			, unsigned		, "priNds"  ) \
	STAT_FIELD( nDualNodes				, unsigned		, "duaNds"  ) \
	STAT_FIELD( nCouplingNodes			, unsigned		, "cplNds" ) \
	STAT_FIELD( maxVelocity				, Scalar		, "maxVel"	) \
	STAT_FIELD( frictionError			, Scalar		, "slvErr"	) \
	STAT_FIELD( frictionIterations		, unsigned		, "slvIter"	) \
	STAT_FIELD( assemblyTime			, Scalar		, "asmTime"	) \
	STAT_FIELD( linSolveTime			, Scalar		, "linTime"	) \
	STAT_FIELD( lcpSolveTime			, Scalar		, "lcpTime"	) \
	STAT_FIELD( frictionTime			, Scalar		, "slvTime"	) \
	STAT_FIELD( advectionTime			, Scalar		, "advTime"	) \
	STAT_FIELD( totalTime				, Scalar		, "totTime"	) \

class Stats {


public:
	Stats( const char* base_dir )	 ;

	void dump() ;

private:
	File 	 m_out ;

public:

#define STAT_FIELD( name, type, abv ) \
	type name ;
	EXPAND_STAT
#undef STAT_FIELD

};


} // ns d6


#endif
