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

#ifndef D6_PHASE_FIELDS_HH
#define D6_PHASE_FIELDS_HH

#include "geo/geo.fwd.hh"

#include <memory>

namespace d6 {

typedef MeshImpl PrimalMesh ;

typedef Linear<PrimalMesh> PrimalShape ;
//typedef   P2<PrimalMesh> PrimalShape ;

#ifdef D6_UNSTRUCTURED_DUAL
typedef UnstructuredDOFs      DualMesh ;
typedef UnstructuredShapeFunc DualShape ;
#else
typedef MeshImpl   DualMesh ;

#ifdef D6_DG_STRESSES
typedef DGLinear<  DualMesh> DualShape ;
//typedef DGConstant<  DualMesh> DualShape ;
#else
typedef   Linear<  DualMesh> DualShape ;
#endif
#endif

typedef AbstractScalarField< PrimalShape > PrimalScalarField ;
typedef AbstractVectorField< PrimalShape > PrimalVectorField ;
typedef AbstractTensorField< PrimalShape > PrimalTensorField ;

typedef AbstractScalarField<   DualShape > DualScalarField ;
typedef AbstractTensorField<   DualShape > DualTensorField ;
typedef AbstractSkewTsField<   DualShape > DualSkewTsField ;

typedef PrimalTensorField RBStresses ;



} //d6


#endif
