// Copyright (C) 2007 Fokko Beekhof
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

#include <cmath>

namespace omptl
{

// Extentions

template <class Iterator,class T,class UnaryFunction, class BinaryFunction>
T transform_accumulate(Iterator first, Iterator last, T init,
			UnaryFunction unary_op, BinaryFunction binary_op,
			const unsigned P = _Pfunc::Pfunc());

template <class Iterator, class T, class UnaryFunction>
T transform_accumulate(Iterator first, Iterator last, T init,
		UnaryFunction unary_op, const unsigned P = _Pfunc::Pfunc());

// "Manhattan" distance
template <class InputIterator1, class InputIterator2>
typename ::std::iterator_traits<InputIterator1>::value_type
L1(InputIterator1 first1, InputIterator1 last1,
   InputIterator2 first2, const unsigned P = _Pfunc::Pfunc());

// "Euclidean" distance
template <class InputIterator1, class InputIterator2>
typename ::std::iterator_traits<InputIterator1>::value_type
L2(InputIterator1 first1, InputIterator1 last1,
   InputIterator2 first2, const unsigned P = _Pfunc::Pfunc());

// "Euclidean" length
template <class InputIterator>
typename ::std::iterator_traits<InputIterator>::value_type
L2(InputIterator first, InputIterator last, const unsigned P = _Pfunc::Pfunc());

} // namespace

#ifdef _OPENMP
  #include "omptl_numeric_extentions_par.h"
#else
  #include "omptl_numeric_extentions_ser.h"
#endif

namespace omptl
{

// "Manhattan" distance
template <class InputIterator1, class InputIterator2>
typename ::std::iterator_traits<InputIterator1>::value_type
L1(InputIterator1 first1, InputIterator1 last1,
   InputIterator2 first2, const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator1>::value_type VT;
	return ::omptl::inner_product(first1, last1, first2, VT(0),
					std::plus<VT>(), std::minus<VT>(), P);
}

template <typename T>
struct _MinusSq
{
	T operator()(const T &lhs, const T &rhs) const
	{
		const T d = lhs - rhs;
		return d*d;
	}
};

// "Euclidean" distance
template <class InputIterator1, class InputIterator2>
typename ::std::iterator_traits<InputIterator1>::value_type
L2(InputIterator1 first1, InputIterator1 last1,
   InputIterator2 first2, const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator1>::value_type VT;
	return ::std::sqrt(::omptl::inner_product(first1, last1, first2, VT(0),
					std::plus<VT>(), _MinusSq<VT>(), P));
}

template <typename T>
struct _Sq
{
	T operator()(const T &d) const
	{
		return d*d;
	}
};

// "Euclidean" length
template <class InputIterator>
typename ::std::iterator_traits<InputIterator>::value_type
L2(InputIterator first, InputIterator last, const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator>::value_type VT;
	return ::std::sqrt(::omptl::transform_accumulate(first, last, VT(0),
						_Sq<VT>(), std::plus<VT>(), P));
}

} /* namespace _OMPTL_EXTENTION_NAMESPACE */
