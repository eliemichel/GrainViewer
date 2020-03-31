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

#ifndef D6_UNSTRUCTURED_SHAPE_FUNCTION
#define D6_UNSTRUCTURED_SHAPE_FUNCTION

#include "ShapeFunctionBase.hh"

namespace d6 {

struct UnstructuredShapeFunc ;
struct Config ;
class  Particles ;

struct UnstructuredDOFs {
	typedef Eigen::Matrix< Scalar, WD, Eigen::Dynamic > Vertices ;
	typedef Eigen::Matrix< Scalar,  1, Eigen::Dynamic > Weights ;

	const Vertices& vertices ;
	Weights weights ;

	explicit UnstructuredDOFs( const Vertices& v )
		: vertices(v), m_count(0), m_box( Vec::Ones() ), m_res( VecWi::Ones() ),
		  m_particles( nullptr )
	{}
	UnstructuredDOFs( const Vec& box, const VecWi &res, const Particles * particles ) ;

	Index count() const { return m_count ; }

	void resize( Index n ) {
		m_count = n ;
		rebuild();
	}

	void rebuild() ;

	template < typename Archive >
	void save( Archive &ar, unsigned int ) const ;
	template < typename Archive >
	void load( Archive &ar, unsigned int ) ;
	template < typename Archive >
	void serialize( Archive &ar, unsigned int ) ;

private:

	void compute_weights_from_particles() ;
	void compute_weights_from_vertices() ;

	Index m_count ;
	// Useful for computing weigths
	Vec   m_box ;
	VecWi m_res ;

	const Particles *m_particles ;
};

struct UnstructuredDOFIterator
{

	explicit UnstructuredDOFIterator( const UnstructuredDOFs& def, const Index i )
		: dofDef(def), index(i)
	{}

	bool operator==( const UnstructuredDOFIterator& o ) const
	{ return o.index == index ;}
	bool operator!=( const UnstructuredDOFIterator& o ) const
	{ return o.index != index ; }

	UnstructuredDOFIterator& operator++() {
		++index ;
		return *this ;
	}

	Vec pos() const {
		return dofDef.vertices.col( index ) ;
	}
	Scalar weight() const {
		return dofDef.weights[index] ;
	}
	void locate( Index &loc ) const {
		loc = index ;
	}

private:
	const UnstructuredDOFs& dofDef ;
	Index index ;
};

template<>
struct ShapeFuncTraits< UnstructuredShapeFunc >
{

	typedef Index Location ;
	typedef UnstructuredDOFs DOFDefinition ;

	enum{ NI = 1, NQ = 1 } ;

	static bool constexpr is_mesh_based = false ;

	template <typename CellIterator = Index >
	struct QPIterator {
		typedef UnstructuredDOFIterator Type ;
	};
} ;

struct UnstructuredShapeFunc : public ShapeFuncBase< UnstructuredShapeFunc >
{

	typedef ShapeFuncBase< UnstructuredShapeFunc > Base ;
	typedef ShapeFuncTraits< UnstructuredShapeFunc > Traits ;
	typedef typename Traits::DOFDefinition DOFDefinition ;

	UnstructuredShapeFunc( const DOFDefinition &v ) : m_dofDef(v)
	{}

	void compute_lumped_mass( DynVec& volumes ) const
	{
		volumes = m_dofDef.weights ;
	}

	Index nDOF() const { return m_dofDef.count() ; }

	void locate_by_pos_or_id( const Vec&, const Index id, typename Base::Location & loc ) const {
		loc = id ;
	}

	void interpolate( const Location& loc, typename Base::Interpolation& itp ) const {
		itp.nodes[0] = loc ;
		itp.coeffs[0] = 1 ;
	}
	void get_derivatives( const Location&, typename Base::Derivatives& dc_dx ) const {
		dc_dx.setZero() ;
	}

	void list_nodes( const Location& loc, typename Base::NodeList& list ) const {
		list[0] = loc ;
	}

	UnstructuredDOFIterator qpBegin() const {
		return UnstructuredDOFIterator( m_dofDef, 0 ) ;
	}

	UnstructuredDOFIterator qpEnd() const {
		return UnstructuredDOFIterator( m_dofDef, nDOF() ) ;
	}

	template <typename CellIterator>
	UnstructuredDOFIterator qpIterator( const Index &it ) const {
		return UnstructuredDOFIterator( m_dofDef, it ) ;
	}

protected:
	const DOFDefinition &m_dofDef ;
};

} //d6

#endif

