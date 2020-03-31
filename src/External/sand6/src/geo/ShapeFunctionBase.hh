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

#ifndef D6_SHAPE_FUNCTION_BASE_HH
#define D6_SHAPE_FUNCTION_BASE_HH

#include "utils/alg.hh"
#include "geo.fwd.hh"

namespace d6 {

template <typename ShapeFunc>
struct ShapeFuncTraits
{
	static bool constexpr is_mesh_based = false ;
} ;


template< typename Derived >
struct ShapeFuncBase
{
	typedef ShapeFuncTraits< Derived > Traits ;

	typedef typename Traits::Location Location ;
	enum {
		NI = Traits::NI
	} ;
	static bool constexpr is_mesh_based = Traits::is_mesh_based ;
	typedef typename Traits::DOFDefinition DOFDefinition ;

	typedef Eigen::Matrix<  Index, Traits::NI, 1  > NodeList ;
	typedef Eigen::Matrix< Scalar, Traits::NI, 1  > CoefList ;
	typedef Eigen::Matrix< Scalar, Traits::NI, WD > Derivatives ;

	struct Interpolation {
		NodeList nodes  ; // Adjacent node indices
		CoefList coeffs ; // Interpolation coeff for nodes
	};


	Index nDOF() const { return derived().nDOF() ; }

	const Derived& derived() const
	{ return static_cast< const Derived& >( *this ) ; }

	void locate_by_pos_or_id( const Vec &pos, const Index id, Location &loc ) const
	{ derived().locate_by_pos_or_id( pos, id, loc ) ; }

	void interpolate( const Location& loc, Interpolation& itp ) const
	{ derived().interpolate( loc, itp ) ; }
	void interpolate_tpz( const Location& loc, Interpolation& itp ) const
	{ derived().interpolate_tpz( loc, itp ) ; }

	void get_derivatives( const Location& loc, Derivatives& dc_dx ) const
	{ derived().get_derivatives( loc, dc_dx ) ; }
	void list_nodes( const Location& loc, NodeList& list ) const
	{ derived().list_nodes( loc, list ) ; }
	void locate_dof( Location& loc, Index dofIndex ) const
	{ derived().locate_dof( loc, dofIndex ) ; }

	void compute_lumped_mass( DynVec& volumes ) const
	{ derived().compute_lumped_mass( volumes ) ; }
	void compute_tpz_mass( DynVec& volumes ) const
	{ derived().compute_tpz_mass( volumes ) ; }

	typename Traits::template QPIterator<>::Type qpBegin() const
	{ return derived().qpBegin() ; }
	typename Traits::template QPIterator<>::Type qpEnd() const
	{ return derived().qpEnd() ; }
	template <typename CellIterator>
	typename Traits::template QPIterator<CellIterator>::Type qpIterator( const CellIterator &it ) const
	{ return derived().template qpIterator<CellIterator>( it ) ; }

	const DOFDefinition& dofDefinition() const
	{ return derived().dofDefinition() ; }

	void build_visu_mesh( DynMatW& vertices, DynMati& indices ) const
	{ derived().build_visu_mesh( vertices, indices ) ; }

};


} // d6

#endif
