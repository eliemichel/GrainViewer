
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

#include "TensorField.hh"
#include "FieldBase.impl.hh"

#include "geo/Tensor.hh"

#include "instanciations.hh"

namespace d6
{

template <typename ShapeFuncT>
void AbstractTensorField< ShapeFuncT >::get_sym_tensor(const Location &x, Mat &tensor) const
{
	const typename Base::ValueType& v = Base::eval_at(x) ;
	tensor_view( v ).get( tensor ) ;
}

template <typename ShapeFuncT>
void AbstractTensorField< ShapeFuncT >::add_sym_tensor(const Location &x, Mat &tensor) const
{
	const typename Base::ValueType& v = Base::eval_at(x) ;
	tensor_view( v ).add( tensor ) ;
}

template <typename ShapeFuncT>
void AbstractSkewTsField< ShapeFuncT >::get_spi_tensor(const Location &x, Mat &tensor) const
{
	const typename Base::ValueType& v = Base::eval_at(x) ;
	tensor_view( v ).get( tensor ) ;
}

template <typename ShapeFuncT>
void AbstractSkewTsField< ShapeFuncT >::add_spi_tensor(const Location &x, Mat &tensor) const
{
	const typename Base::ValueType& v = Base::eval_at(x) ;
	tensor_view( v ).add( tensor ) ;
}

#define INSTANTIATE( Shape ) \
	template class FieldBase< AbstractTensorField< Shape > > ; \
	template class AbstractTensorField< Shape > ; \
	template class FieldBase< AbstractSkewTsField< Shape > > ; \
	template class AbstractSkewTsField< Shape > ;

EXPAND_INSTANTIATIONS
#undef INSTANTIATE

}

