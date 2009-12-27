// Copyright (C) 2006 Fokko Beekhof
// Email contact: Fokko.Beekhof@cui.unige.ch

// The OMPTL library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef OMPTL_NUMERIC
#define OMPTL_NUMERIC 1

#include <numeric>
#include "omptl"

namespace omptl
{

template <class InputIterator, class T>
T accumulate(InputIterator first, InputIterator last, T init,
		const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class T, class BinaryFunction>
T accumulate(InputIterator first, InputIterator last, T init,
	     BinaryFunction binary_op,
	     const unsigned P = _Pfunc::Pfunc());

/*
 * Not (yet) paralellized due to data dependance.
 */
template <class InputIterator, class OutputIterator, class BinaryFunction>
OutputIterator
adjacent_difference(InputIterator first, InputIterator last,
		    OutputIterator result, BinaryFunction binary_op,
		    const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator>
OutputIterator adjacent_difference(InputIterator first, InputIterator last,
				   OutputIterator result,
				   const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class T,
          class BinaryFunction1, class BinaryFunction2>
T inner_product(InputIterator1 first1, InputIterator1 last1,
		InputIterator2 first2, T init,
		BinaryFunction1 binary_op1, BinaryFunction2 binary_op2,
		const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class T>
T inner_product(InputIterator1 first1, InputIterator1 last1,
		InputIterator2 first2, T init,
		const unsigned P = _Pfunc::Pfunc());

// Not paralellized due to dependencies and complications with OutputIterators.
template <class InputIterator, class OutputIterator,
	  class BinaryOperation>
OutputIterator partial_sum(InputIterator first, InputIterator last,
			   OutputIterator result, BinaryOperation binary_op,
			   const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator>
OutputIterator partial_sum(InputIterator first, InputIterator last,
		OutputIterator result, const unsigned P = _Pfunc::Pfunc());

} // namespace omptl

#ifdef _OPENMP
  #include "omptl_numeric_par.h"
#else
  #include "omptl_numeric_ser.h"
#endif

#include "omptl_numeric_extentions.h"

#endif /* OMPTL_NUMERIC */
