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

#include "Stats.hh"

#include <iomanip>

namespace d6 {

template < typename T >
struct FieldWidth {
	static constexpr int width = 7 ;
};
template < >
struct FieldWidth< Scalar > {
	static constexpr int width = 11 ;
};

Stats::Stats(const char *base_dir)
	: m_out( FileInfo(base_dir).filePath("stats.txt"), std::ios_base::out )
#define STAT_FIELD( name, type, abv ) \
	, name(0)
	EXPAND_STAT
#undef STAT_FIELD
{

#define STAT_FIELD( name, type, abv ) \
	m_out << std::setw( FieldWidth<type>::width ) << abv << " " ;
	EXPAND_STAT
#undef STAT_FIELD

	m_out << std::endl ;

}

void Stats::dump() {

#define STAT_FIELD( name, type, abv ) \
	m_out << std::setw( FieldWidth<type>::width ) << name << " " ;
	EXPAND_STAT
#undef STAT_FIELD

   m_out << std::endl ;

	++ stepId ;
}

} //d6
