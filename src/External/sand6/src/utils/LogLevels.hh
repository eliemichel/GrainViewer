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

#ifndef D6_LOGLEVELS_HH
#define D6_LOGLEVELS_HH
/*!
 * \file Loglevels.hpp
 * \brief Definition of the output verbosity levels
 *
 * \sa Log.hh
 *
 */


//! Logs levels, ordered from more verbose to less verbose
/*! For each of those levels, one typedef will be defined in the namespace Log.
  It will be then possible to use it as an out stream. \sa Log */
#define D6_LOG_LEVELS \
  D6_LOG_LEVEL( All     ) \
  D6_LOG_LEVEL( Debug   ) \
  D6_LOG_LEVEL( Verbose ) \
  D6_LOG_LEVEL( Info    ) \
  D6_LOG_LEVEL( Warning ) \
  D6_LOG_LEVEL( Error   )

namespace d6 {
namespace Log {

//! Enum containing all log levels, prefixed by 'L_' ( e.g L_Debug )
enum Level {
#define D6_LOG_LEVEL( level ) L_##level,
  D6_LOG_LEVELS
#undef D6_LOG_LEVEL
  L_Nothing
} ;

} // Log

} // namespace d6

#endif // LOGLEVELS_DEF_D6_H

