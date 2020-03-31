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

#include "RigidBodyData.hh"

#include "simu/RigidBody.hh"

#include "simu/FormBuilder.hh"
#include "simu/FormBuilder.impl.hh"

#include "geo/LevelSet.hh"

#include "geo/TensorField.hh"
#include "geo/Tensor.hh"

#include "geo/MeshImpl.hh"
#include "geo/MeshCell.hh"

#include <bogus/Core/Block.impl.hpp>

namespace d6 {

const Scalar RigidBodyData::s_splatRad = 1 ;

RigidBodyData::RigidBodyData( RigidBody& rb_, TensorField &s )
	: rb(rb_), stresses(s)
{
}

Scalar RigidBodyData::phi( const Vec &x ) const
{
	return std::min( 1., std::max(0., 1. + rb.levelSet().eval_at( x ) / s_splatRad ) );
}
void RigidBodyData::grad_phi(const Vec &x, Vec &grad) const
{
	rb.levelSet().grad_at( x, grad );
	grad /= s_splatRad ;
}

void RigidBodyData::compute_active( const Active& phaseNodes )
{
	const TensorField::ShapeFuncImpl &shape = stresses.shape() ;
	const MeshType &mesh = shape.mesh() ;

	typename TensorField::ShapeFuncType::NodeList nodelist ;
	typename MeshType::CellGeo geo ;

	nodes.reset( shape.nDOF() );

	for( const typename MeshType::Cell& cell : phaseNodes.cells )
	{
		mesh.get_geo( cell, geo );

		bool boundary = false ;
		bool interior = true  ;

		for( Index k = 0 ; k < MeshType::NV ; ++k  ) {
			const Scalar phi_k = phi( geo.vertex(k) ) ;

			if( phi_k >= 1. ) {
				boundary = true ;
			} else {
				interior = false ;
			}
		}

		if( boundary && !interior )
		{
			nodes.cells.push_back( cell ) ;
			shape.list_nodes( cell, nodelist );

			for( Index k = 0 ; k < nodelist.rows() ; ++k  ) {
				if( nodes.indices[ nodelist[k] ] == Active::s_Inactive ) {
					nodes.indices[ nodelist[k] ] = nodes.nNodes++ ;
				}
			}
		}
	}
}

void RigidBodyData::integrate(const PrimalShape& primalShape, const DualShape& dualShape,
							  const Active &primalNodes, const Active& dualNodes,
							  Index totNodes)
{
	typedef typename RBStresses::ShapeFuncImpl RBShapeFunc ;

	//TODO allow RBShapeFunc != PrimalShape
	static_assert( std::is_same< PrimalShape, RBShapeFunc >::value,
			"Different RB and primal shape func not allowed yet" ) ;

	typedef const typename PrimalShape::Interpolation& P_Itp ;
	typedef const typename PrimalShape::Derivatives&   P_Dcdx ;
	typedef const typename DualShape  ::Interpolation& D_Itp ;
	typedef const typename DualShape  ::Derivatives&   D_Dcdx ;

	const Index m = primalNodes.count() ;

	{
		typedef FormBuilder< DualShape, PrimalShape > Builder ;
		Builder builder( dualShape, primalShape ) ;

		builder.reset( totNodes );
		builder.addToIndexIf( dualNodes.indices, primalNodes.indices,
							  [&](const Vec& pos){ return phi( pos ) > 0 ; } );
		builder.makeCompressed();

		jacobian.clear() ;
		jacobian.setRows( totNodes );
		jacobian.setCols( m );
		jacobian.cloneIndex( builder.index() ) ;
		jacobian.setBlocksToZero() ;

		// Fraction
		intFraction.resize( dualNodes.count() ) ;
		intFraction.setZero() ;

#ifdef D6_UNSTRUCTURED_DUAL
		builder.integrate_qp(
#else
		builder.integrate_cell< form::Right >( primalNodes.cells.begin(), primalNodes.cells.end(),
#endif
					[&]( Scalar w, const Vec& pos, D_Itp l_itp, D_Dcdx, P_Itp r_itp, P_Dcdx )
		{
			const Scalar phiRb = phi(pos) ;
			if( phiRb <= 0 ) return ;

			Vec dphi_dx ;
			grad_phi( pos, dphi_dx ) ;

			bool active = true ;

			for( Index k = 0 ; k < l_itp.nodes.rows() ; ++ k ) {
				const Index idx = dualNodes.indices[ l_itp.nodes[k] ] ;
				if( idx == Active::s_Inactive ) {
					active = false ;
				} else {
					intFraction[ idx ] += w * l_itp.coeffs[k] * phiRb ;
				}
			}

			if( active ) {
				Builder:: addUTaunGphi( jacobian, w, l_itp, r_itp, dphi_dx, dualNodes.indices, primalNodes.indices ) ;
			}
		}
		);

	}

	{
		FormMat<SD,WD>::Type jacobian_2 ;

		typedef FormBuilder< RBShapeFunc, PrimalShape > Builder ;
		Builder builder( primalShape, primalShape ) ;

		builder.reset( totNodes );
		builder.addToIndex(   nodes.cells.begin(),  nodes.cells.end(),      nodes.indices, primalNodes.indices );
		builder.makeCompressed();

		jacobian_2.clear() ;
		jacobian_2.setRows( totNodes );
		jacobian_2.setCols( m );
		jacobian_2.cloneIndex( builder.index() ) ;
		jacobian_2.setBlocksToZero() ;

		builder.integrate_node(  nodes.cells.begin(), nodes.cells.end(), [&]( Scalar w, const Vec& pos, P_Itp l_itp, P_Itp r_itp )
		{
			Vec dphi_dx ;
			grad_phi( pos, dphi_dx ) ;

			Builder:: addUTauGphi ( jacobian_2, w, l_itp, r_itp, dphi_dx,      nodes.indices, primalNodes.indices ) ;
		}
		);

		jacobian += jacobian_2 ;

	}

}

void RigidBodyData::assemble_matrices( const PrimalShape& primalShape, const DualShape& dualShape,
									   const Active &primalNodes, const Active& dualNodes,
									   Index totNodes)

{
	typedef typename PrimalShape::MeshType MeshType ;
	const MeshType &mesh = primalShape.mesh() ;

	typename TensorField::ShapeFuncType::NodeList nodelist ;

	typename FormMat<WD,SD>::UncompressedType	proj ;
	proj.clear() ;
	proj.setRows( primalNodes.count() );
	proj.setCols( 1 );

	typename FormMat<WD,SD>::BlockT P ;
	P.topLeftCorner<WD,WD>().setIdentity() ;

	for( const typename MeshType::Cell& cell : nodes.cells )
	{
		typename PrimalShape::Location ploc ;
		ploc.cell = cell ;
		primalShape.list_nodes( ploc, nodelist );

		for( Index k = 0 ; k < nodelist.rows() ; ++k  ) {

			const Index glb_idx = nodelist[k] ;
			const Index row = primalNodes.indices[ glb_idx ] ;

			if( proj.majorIndex().size(row) == 0 ) {

				primalShape.locate_dof( ploc, k ) ;
				const Vec pos = mesh.pos( ploc ) ;
				const Vec dx = pos - rb.levelSet().origin() ;
				make_cross_mat( dx, P.block<WD,RD>(0,WD) ) ;
				proj.insert( row, 0  ) = P ;
			}
		}
	}
	proj.finalize() ;

	projection = proj.transpose() ;

	integrate( primalShape, dualShape, primalNodes, dualNodes, totNodes );
}


} //d6
