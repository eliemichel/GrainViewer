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

#include "Particles.hh"

#include "MeshImpl.hh"

#include "geo/Tensor.hh"
#include "geo/Voxel.hh"

#include "UnstructuredShapeFunction.hh"
#include "TensorField.hh"

#include "utils/Log.hh"

#include <bogus/Core/Utils/Timer.hpp>

namespace d6 {

const size_t Particles::s_MAX = 1.e7 ;

Particles::Particles()
    : m_count(0)
{
	resize(s_MAX) ;
}

void Particles::generate(const ScalarExpr &expr, const unsigned nSamples,
                         const MeshType &mesh, const bool alignOnCells,
                         const Vec& initialOri )
{
	bogus::Timer timer ;

	m_count = 0 ;

	// Uniform gen

	if( nSamples > 10 )
	{
		// nSamples is to be understood globally (w.r.t dim domain)

		Voxel cellGeo ;
		cellGeo.origin.setZero() ;
		cellGeo.box = mesh.box() ;

		Index nPerSide = nSamples / ( std::pow( cellGeo.box.prod(), 1./3 ) /  cellGeo.box.minCoeff() )  ;
		Index n = cellGeo.sample_uniform( nPerSide, m_count, m_centers, m_frames ) ;
		const Scalar volume = cellGeo.volume() / n ;

		for( size_t i = m_count ; i < m_count+n ; ) {
			const Scalar phi = expr( m_centers.col(i) ) ;
			if( phi == 0. ) {
				-- n ;
				m_centers.col(i) = m_centers.col(m_count+n) ;
				m_frames .col(i) = m_frames .col(m_count+n) ;
			} else {
				m_volumes[i] = volume * phi ;
				++i ;
			}
		}


		m_count += n ;

	} else {
		//sNamples to be understood w.r.t mesh cell

		typename MeshType::CellGeo cellGeo ;

		for( typename MeshType::CellIterator it = mesh.cellBegin() ; it != mesh.cellEnd() ; ++it ) {
			mesh.get_geo( *it, cellGeo ) ;

			Scalar phi = expr( cellGeo.center() ) ;
			if( alignOnCells && phi < 1.e-8 )
				continue ;

			Index n = cellGeo.sample_uniform( nSamples, m_count, m_centers, m_frames ) ;
			const Scalar volume = cellGeo.volume() / n ;

			for( size_t i = m_count ; i < m_count+n ; ) {
				if(!alignOnCells)
					phi = expr( m_centers.col(i) ) ;
				if( phi < 1.e-8 ) {
					-- n ;
					m_centers.col(i) = m_centers.col(m_count+n) ;
					m_frames .col(i) = m_frames .col(m_count+n) ;
				} else {
					m_volumes[i] = volume * phi ;
					++i ;
				}
			}

			m_volumes.segment( m_count, n ).setConstant( volume ) ;

			m_count += n ;
		}

	}

	m_velocities.leftCols( count() ).setZero() ;

	VecS oriCoeffs ;
	{
		Mat oriTensor = Mat::Zero() ;
		oriTensor.diagonal() = initialOri ;
		tensor_view( oriCoeffs ).set( oriTensor) ;
	}

	m_orient.leftCols(m_count).colwise() = oriCoeffs ; // Isotropic ori

	Log::Verbose() << arg( "Generated %1 particles in %2 s ", m_count, timer.elapsed() ) << std::endl ;
}

void Particles::resize(size_t n)
{
	m_volumes.resize( n );

	m_centers.resize( WD, n);
	m_velocities.resize( WD, n);

	m_frames.resize( SD, n);
	m_orient.resize( SD, n);
}

void Particles::EventLog::log(const Event& event)
{
	std::lock_guard<std::mutex> lock( m_log_mutex ) ;
	m_log.back().push_back( event );
}

template <typename Derived >
void Particles::EventLog::replay( FieldBase<Derived> &field ) const
{
	if( m_log.empty() ) return ;

	const std::vector< Event >& events = m_log.back() ;

	Index splits = 0, merges = 0, removes = 0 ;

	const Index n = field.size() ;

	for( const Event& e: events ) {

		if( e.type == Event::Split ) {
			++splits ;
		} else if( e.type == Event::Remove ) {
			++removes ;
		} else if( e.type == Event::Merge ) {
			++merges;
		}
	}

	size_t nmax = n + splits ;
	field.fit_conservative( nmax ) ;

	if( splits > 0 ) {
		for( const Event &e : events ) {
			if( e.type == Event::Split ) {
				field[e.second] = field[e.first] ;
			}
		}
	}

	if (merges > 0 ){

		std::vector< bool > merged( nmax, false ) ;

		for( const Event &e : events ) {
			if( e.type == Event::Merge ) {
				field[e.first] = .5*( field[e.first] + field[e.second] ) ;
				merged[e.second] = true ;
			}
		}

		size_t reloc_src = nmax ;
		for( const Event &e : events ) {

			if( e.type == Event::Merge && e.second + merges < nmax ) {
				// Find last non-emptym pos
				do {
					--reloc_src ;
				} while( reloc_src > e.second && merged[reloc_src] ) ;

				field[ e.second ] = field[ reloc_src ] ;
			}
		}
	}

	if( removes > 0 ){
		for( const Event &e : events ) {
			if( e.type == Event::Remove ) {
				field[e.second] = field[e.first] ;
			}
		}
	}

	// TODO avoid so many copies
//	field.fit_shape() ;
	field.fit_conservative( field.shape().nDOF() ) ;

	if( field.size() > (Index) nmax ) {
		const Index nExtra = (field.size() - nmax) ;
		Log::Debug() << "Zeroing extra " << nExtra  << " comps " << std::endl ;
		field.flatten().segment( nmax*Derived::D, nExtra*Derived::D ).setZero() ;
	}

}

template void Particles::EventLog::replay( FieldBase<AbstractTensorField<UnstructuredShapeFunc>> &values ) const ;

} //d6
