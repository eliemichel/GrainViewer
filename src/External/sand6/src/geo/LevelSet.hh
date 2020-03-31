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

#ifndef D6_LEVEL_SET_HH
#define D6_LEVEL_SET_HH

#include "geo/rotations.hh"

#include <memory>

namespace d6 {

class LevelSet {

public:

	typedef std::unique_ptr< LevelSet > Ptr ;

	// Accessors
	Scalar eval_at( const Vec& x ) const ;
	void   grad_at( const Vec& x, Vec& grad ) const ;

	const Vec& origin() const
	{
		return m_origin ;
	}
	const Rotation& rotation() const
	{
		return m_frame ;
	}
	Scalar scale() const {
		return m_scale ;
	}

	//Constructors
	static Ptr make_sphere( ) ;
	static Ptr make_plane( ) ;
	static Ptr make_torus( Scalar radius ) ;
	static Ptr make_cylinder( Scalar height ) ;
	static Ptr make_hole( Scalar radius ) ;
	static Ptr make_hourglass( Scalar height, Scalar radius ) ;
	static Ptr from_mesh( const char* objFile ) ;

	virtual bool compute() { return true ; }

	// Absolute positioning
	LevelSet& scale( const Scalar s )
	{ m_scale = s ; return *this ; }

	LevelSet& set_origin( const Vec& pos ) {
		m_origin = pos ;
		return *this ;
	}
	LevelSet& set_rotation( const Rotation& frame ) {
		m_frame = frame ;
		return *this ;
	}
	LevelSet& rotate( const Rotation& frame ) {
		m_frame = d6::rotate(frame, m_frame) ;
		return *this ;
	}

#if D6_DIM==3
	LevelSet& set_rotation( const Vec& axis, Scalar angle ) {
		m_frame = Eigen::AngleAxis< Scalar >( angle, axis )  ;
		return *this ;
	}
	LevelSet& rotate( const Vec& axis, Scalar angle ) {
		m_frame = Eigen::AngleAxis< Scalar >( angle, axis ) * m_frame  ;
		return *this ;
	}
#endif

	void inv_inertia( MatS & Mi ) const ;

	Scalar volume() const {
		return local_volume() * std::pow( m_scale, WD ) ;
	}

	//Movement
	void move( const Vec& depl, const Rotation& rot ) ;

	template<class Archive>
	static void register_derived(Archive &ar) ;
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version) ;

		virtual ~LevelSet() {}

protected:

	void to_local( const Vec &world, Vec &local) const ;
	void to_local_mat ( MatR &mat ) const ;

	LevelSet() ;

	virtual Scalar eval_local( const Vec&x ) const = 0 ;
	virtual Vec    grad_local( const Vec&x ) const = 0 ;
	virtual Scalar local_volume( ) const = 0 ;
	virtual void local_inv_inertia( MatR &I ) const = 0 ;

private:
	Vec 	   m_origin ;
	Rotation   m_frame ;

	Scalar 	   m_scale ;
};


} //d6


#endif
