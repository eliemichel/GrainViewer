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

#include "PrimalData.hh"

#include "utils/Log.hh"

#include <bogus/Core/Block.io.hpp>
#include <bogus/Core/Block.impl.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>

#include <fstream>

namespace d6 {


template <class Archive>
void PrimalData::serialize(Archive &ar, const unsigned int version )
{
	ar & H ;
	ar & w ;

	ar & mu ;

	ar & jacobians ;
	ar & inv_inertia_matrices ;

	if( version > 0 ) {
		ar & mass_matrix_mode ;
		ar & M ;
		ar & f ;
	}
}

bool PrimalData::load(const char *file)
{
	std::ifstream ifs( file );

	if(!ifs) {
		Log::Error() << "Cannot read " << file << std::endl ;
		return false ;
	}

	boost::archive::binary_iarchive ia(ifs);

	mass_matrix_mode = Lumped ;
	ia >> (*this) ;

	return true ;
}

bool PrimalData::dump(const char *file) const
{
	std::ofstream ofs( file );

	if(!ofs) {
		Log::Error() << "Cannot write " << file << std::endl ;
		return false ;
	}

	boost::archive::binary_oarchive oa(ofs);
	oa << (*this) ;
	return true ;
}


} //d6

BOOST_CLASS_VERSION(d6::PrimalData, 1)
