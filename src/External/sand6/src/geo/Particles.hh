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

#ifndef D6_PARTICLES_HH
#define D6_PARTICLES_HH

#include "Expr.hh"

#include <vector>
#include <mutex>

namespace d6 {

class Particles {

public:

	template< int D >
	struct Data {
		typedef Eigen::Matrix< Scalar, D, Eigen::Dynamic > Type ;
	};

	struct Event {
		enum Type { Split, Merge, Remove } ;
		Type   type   ;
		size_t first  ;
		size_t second ;

		Vec    dx ;

		template < typename Archive >
		void serialize( Archive &ar, unsigned int ) ;

		static Event split( size_t first, size_t second, Vec dx ) {
			return Event{ Split, first, second, dx } ;
		}
		static Event merge( size_t first, size_t second, Vec dx ) {
			return Event{ Merge, first, second, dx } ;
		}
		static Event remove( size_t removed, size_t reloc  ) {
			return Event{ Remove, removed, reloc, Vec::Zero() } ;
		}
	};
	struct EventLog {
		void log( const Event& event )	;

		void start() {
			m_log.emplace_back() ;
		}
		void clear() {
			m_log.clear() ;
		}
		const std::vector< std::vector< Event > >& events() const 	{
			return m_log ;
		}

		template < typename Archive >
		void serialize( Archive &ar, unsigned int ) {
			ar & m_log ;
		}

		const std::vector< std::vector< Event > >& log() const {
			return m_log ;
		}

		template < typename Derived >
		void replay( FieldBase<Derived>& field ) const ;

	private:
		std::vector< std::vector< Event > > m_log ;
		std::mutex m_log_mutex ;

	};

	static const size_t s_MAX ;

	Particles() ;

	void generate(const ScalarExpr &expr, const unsigned nSamples,
				  const MeshType& mesh, const bool alignOnCells , const Vec &initialOri) ;

	size_t count() const { return m_count ; }

	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) ;

	const typename Data< 1  >::Type&    volumes() const { return    m_volumes ; }
	const typename Data< WD >::Type&    centers() const { return    m_centers ; }
	const typename Data< WD >::Type& velocities() const { return m_velocities ; }
	const typename Data< SD >::Type&     frames() const { return     m_frames ; }
	const typename Data< SD >::Type&     orient() const { return     m_orient ; }


private:

	std::size_t m_count ;

	typename Data< 1  >::Type m_volumes ;

	typename Data< WD >::Type m_centers ;
	typename Data< WD >::Type m_velocities ;

	typename Data< SD >::Type m_frames ;
	typename Data< SD >::Type m_orient ; // Aniso

	void resize( size_t n ) ;

	friend class DynParticles ;

};

} //d6

#endif

