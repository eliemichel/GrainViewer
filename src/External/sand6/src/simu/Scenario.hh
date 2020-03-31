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

#ifndef D6_SCENARIO_HH
#define D6_SCENARIO_HH

#include "utils/scalar.hh"
#include "utils/units.hh"
#include "geo/Expr.hh"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace d6 {

struct Config ;
struct ScenarioFactory ;
class RigidBody ;
class Simu ;

class Scenario {

public:

	static std::unique_ptr< Scenario > parse( const Config& config ) ;
	static void register_factory( const ScenarioFactory& factory ) ;

	virtual ~Scenario() {}

	struct ParticleGenerator : public Expr<Scalar> {
		const Scenario& scenario ;
		ParticleGenerator( const Scenario& s ) : scenario(s) {}
		Scalar operator() ( const Vec&  x ) const { return scenario.particle_density(x) ; }
	};
	ParticleGenerator generator() const { return ParticleGenerator(*this) ; }

	virtual Scalar particle_density( const Vec &x ) const = 0 ;

	virtual void add_rigid_bodies( std::vector< RigidBody >& /*rbs*/ ) const {}
	virtual void update( Simu& /*simu*/, Scalar /*time*/, Scalar /*dt*/ ) const {}

protected:
	typedef std::unordered_map< std::string, std::string > Params ;

	Scenario() : m_config(0) {}

	virtual void init( const Params& /*params*/ ) {}

	Scalar      scalar_param( const Params& params, const std::string& key,
						 Units::Unit unit = Units::None,
						 Scalar def = 0. ) const ;
	std::string string_param( const Params& params, const std::string& key,
						 const std::string& def = "" ) const ;

	const Vec& box() const ;
	Scalar box( Index i ) const { return box()[i] ; }

	const Config* m_config ;

};

struct ScenarioFactory {
	virtual std::unique_ptr< Scenario > make( const std::string& str ) const = 0 ;

	ScenarioFactory() {}
	ScenarioFactory( const ScenarioFactory& ) = delete ;
	ScenarioFactory& operator= ( const ScenarioFactory& ) = delete ;
};

struct DefaultScenarioFactory : public ScenarioFactory
{
	std::unique_ptr< Scenario > make( const std::string & str ) const override ;
};


} //d6

#endif
