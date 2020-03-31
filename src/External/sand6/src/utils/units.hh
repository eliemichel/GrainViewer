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

#ifndef D6_UNITS_HH
#define D6_UNITS_HH

#include "scalar.hh"

#include <cmath>

namespace d6 {

struct Units {

	enum Unit {
		None,
		Length,
		Volume,
		VolumicMass,
		Acceleration,
		Velocity,
		Time,
		Frequency,
		Viscosity,
		Stress,
		Torque
	} ;

	Scalar L ;  //!< Typical length (m)
	Scalar G ;  //!< Typical acceleration (m.s^{-2})
	Scalar R ;  //!< Typical density (kg.m^{-3})

	Scalar U ;  //!< Typical velocity (m.s^{-1})
	Scalar T ;  //!< Typical time (s)
	Scalar P ;  //!< Typical stress (Pa)
	Scalar M ;  //!< Typical viscosity (Pa.s)

	Units() ;

	void setTypical( Scalar length, Scalar acc, Scalar volMass ) ;

	Scalar fromSI( Unit u ) const ;
	Scalar   toSI( Unit u ) const ;

};


}

#endif
