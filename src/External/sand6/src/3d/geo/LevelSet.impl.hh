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

#ifndef D6_LEVEL_SET_IMPL_HH
#define D6_LEVEL_SET_IMPL_HH

#include "geo/LevelSet.hh"

#include "geo/Grid.hh"
#include "geo/MeshShapeFunction.hh"
#include "geo/ScalarField.hh"

#include <limits>

namespace d6 {

struct SphereLevelSet : public LevelSet
{
	Scalar eval_local(const Vec &x) const override {
		return 1. - x.norm() ;
	}

	Vec grad_local(const Vec &x) const override {
		return -x / ( 1.e-12 + x.norm() ) ;
	}

	void local_inv_inertia( Mat& I ) const override {
		I = Mat::Identity() / ( local_volume() * 2./5. ) ;
	}

	Scalar local_volume() const override {
		return 4./3 * M_PI ;
	}

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;
};
struct PlaneLevelSet : public LevelSet
{
	Scalar eval_local(const Vec &x) const override {
		return - x[2] ;
	}

	Vec grad_local(const Vec & ) const override {
		return Vec(0, 0, -1) ;
	}

	void local_inv_inertia( Mat& I ) const override {
		I.setZero() ;
	}

	Scalar local_volume() const override {
		return std::numeric_limits<Scalar>::infinity() ;
	}

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;
};

struct TorusLevelSet : public LevelSet
{

	explicit TorusLevelSet( Scalar rad = 1. )
		: m_radius( rad )
	{}

	Scalar radius() const { return m_radius ; }

	Scalar eval_local(const Vec &x) const override
	{
		Vec proj ;
		proj_on_circle( x, proj );

		return m_radius - (x - proj).norm() ;
	}

	Vec grad_local(const Vec &x) const override {
		Vec proj ;
		proj_on_circle( x, proj );

		const Scalar n = (x - proj).norm() ;
		return (proj - x) / (1.e-12 + n) ;
	}

	void local_inv_inertia( Mat& I ) const override {
		//From http://mathworld.wolfram.com/Torus.html
		I.setZero() ;
		I(0,0) = I(1,1) = 1./( 5./8 * m_radius * m_radius + .5 ) ;
		I(2,2)          = 1./( 3./4 * m_radius * m_radius + 1. ) ;
		I.diagonal() /= local_volume() ;
	}

	Scalar local_volume() const override {
		return 2 * M_PI * M_PI * m_radius * m_radius ;
	}

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;

private:

	static void proj_on_circle( const Vec&x, Vec& proj )
	{
		const Scalar n = x.head<2>().norm() ;

		// Projection on unit circle on XY axis
		if( n < 1.e-12 ) proj = Vec(0,1,0) ;
		else {
			proj.head<2>() = x.head<2>() / n ;
			proj[2] = 0 ;
		}
	}

	Scalar m_radius ;
};

struct HoleLevelSet : public LevelSet
{
	explicit HoleLevelSet( Scalar rad = 1. )
		: m_radius( rad )
	{}

	Scalar radius() const { return m_radius ; }

	Scalar eval_local(const Vec &x) const override {
		Vec proj ;
		proj_on_plane( x, proj );

		return 1. - (x - proj).norm() ;
	}

	Vec grad_local(const Vec &x ) const override {
		Vec proj ;
		proj_on_plane( x, proj );

		const Scalar n = (x - proj).norm() ;
		return (proj - x) / (1.e-12 + n) ;
	}

	void local_inv_inertia( Mat& I ) const override {
		I.setZero() ;
	}

	Scalar local_volume() const override {
		return std::numeric_limits<Scalar>::infinity() ;
	}

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;

private:

	void proj_on_plane( const Vec&x, Vec& proj ) const
	{

		proj.head<2>() = x.head<2>() ;
		proj[2] = 0 ;

		const Scalar n = proj.norm() ;

		if( n < m_radius ) {
			if( n < 1.e-12 ) {
				proj = Vec(0,m_radius,0) ;
			} else {
				proj.head<2>() *= m_radius/n ;
			}
		}
	}

	Scalar m_radius ;
};

struct CylinderLevelSet : public LevelSet
{

	explicit CylinderLevelSet( Scalar h = 1. )
		: m_height( h )
	{}

	Scalar height() const { return m_height ; }

	Scalar eval_local(const Vec &x) const override
	{
		Vec proj ;
		proj_on_axis( x, proj );

		return 1. - (x - proj).norm() ;
	}

	Vec grad_local(const Vec &x) const override {
		Vec proj ;
		proj_on_axis( x, proj );

		const Scalar n = (x - proj).norm() ;
		return (proj - x) / (1.e-12 + n) ;
	}

	void local_inv_inertia( Mat& I ) const override {
		//From http://mathworld.wolfram.com/Torus.html
		I.setZero() ;
		I(0,0) = I(1,1) = 6./( 3. + m_height * m_height ) ;
		I(2,2)          = 1. ;
		I.diagonal() *= 2./local_volume() ;
	}

	Scalar local_volume() const override {
		return m_height * M_PI ;
	}

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;

private:

	void proj_on_axis( const Vec&x, Vec& proj ) const
	{
		proj[0] = 0. ;
		proj[1] = 0. ;
		proj[2] = std::max(-.5*m_height, std::min(.5*m_height, x[2])) ;
	}

	Scalar m_height ;
};

struct HourglassLevelSet : public LevelSet
{

	explicit HourglassLevelSet( Scalar h = 2., Scalar r = .1 )
		: m_height( h ), m_radius(r)
	{}

	Scalar height() const { return m_height ; }
	Scalar outletRadius() const { return m_radius ; }

	Scalar eval_local(const Vec &x) const override
	{
		Vec proj_e, proj_a ;
		proj_on_axis( x, proj_a );
		proj_on_ends( x, proj_e );

		Scalar phi_e = 1.       - (x-proj_e).norm() ;
		Scalar phi_a = m_radius - (x-proj_a).norm() ;

		return - std::max(phi_e, phi_a) ;
	}

	Vec grad_local(const Vec &x) const override
	{
		Vec proj_e, proj_a ;
		proj_on_axis( x, proj_a );
		proj_on_ends( x, proj_e );

		Scalar phi_e = 1.       - (x-proj_e).norm() ;
		Scalar phi_a = m_radius - (x-proj_a).norm() ;

		if( phi_e > phi_a ) {
			const Scalar n = (x - proj_e).norm() ;
			return -(proj_e - x) / (1.e-12 + n) ;
		} else {
			const Scalar n = (x - proj_a).norm() ;
			return -(proj_a - x) / (1.e-12 + n) ;
		}
	}

	void local_inv_inertia( Mat& I ) const override {
		I.setZero() ;
	}

	Scalar local_volume() const override {
		return 1 ;
	}

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;

private:

	void proj_on_ends( const Vec&x, Vec& proj ) const
	{
		proj[0] = 0. ;
		proj[1] = 0. ;
		if(x[2]>0)
			proj[2] =  .5*m_height ;
		else
			proj[2] = -.5*m_height ;
	}

	void proj_on_axis( const Vec&x, Vec& proj ) const
	{
		proj[0] = 0. ;
		proj[1] = 0. ;
		proj[2] = std::max(-.5*m_height, std::min(.5*m_height, x[2])) ;
	}

	Scalar m_height ;
	Scalar m_radius ;
};

struct MeshLevelSet : public LevelSet
{
	explicit MeshLevelSet( const char* objFile = "" ) ;

	Scalar eval_local(const Vec &x) const override ;
	Vec grad_local(const Vec &x) const override ;

	// TODO -- can only be used as static obstacle for now
	void local_inv_inertia( Mat& I ) const override {
		I.setZero() ;
	}
	// TODO -- can only be used as static obstacle for now
	Scalar local_volume() const override {
		return 1 ;
	}

	bool compute() override ;

	template<class Archive>
	void serialize(Archive &ar, const unsigned int version ) ;

	const std::string& objFile() const
	{ return m_objFile ; }

private:
	std::string m_objFile ;
	Scalar m_radius ;

	Grid m_grid ;
	Vec  m_offset ;
	Scalar m_emptyVal ;
	AbstractScalarField< Linear<Grid> > m_values ;
};

} //ns d6

#endif
