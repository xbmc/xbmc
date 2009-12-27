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

#ifndef OMPTL_NUMERIC_H
#define OMPTL_NUMERIC_H 1

#include <utility>
#include <functional>
#include <iterator>

#include "omptl_algorithm"
#include "omptl_tools.h"

namespace omptl
{

template <class IteratorTag>
struct _Accumulate
{
	template <class InputIterator, class T, class BinaryFunction>
	static T accumulate(InputIterator first, InputIterator last, T init,
			T par_init, BinaryFunction binary_op, const unsigned P)
	{
		assert(P > 0u);
		if (_linear_serial_is_faster(first, last, P))
			return ::std::accumulate(first, last, init, binary_op);
		assert(2*(int)P <= std::distance(first, last));

		::std::vector< ::std::pair<InputIterator, InputIterator> >
			partitions(P);
		::omptl::_partition_range(first, last, partitions, P);

		::std::vector<T> results(P);
		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
			results[t] = ::std::accumulate( partitions[t].first,
							partitions[t].second,
							par_init, binary_op);

		return ::std::accumulate(results.begin(), results.end(),
					 init, binary_op);
	}
};

template <>
struct _Accumulate< ::std::input_iterator_tag >
{
	template <class InputIterator, class T, class BinaryFunction>
	static T accumulate(InputIterator first, InputIterator last, T init,
			T par_init, BinaryFunction binary_op, const unsigned P)
	{
		return ::std::accumulate(first, last, init, binary_op);
	}
};

template <class InputIterator, class BinaryFunction>
struct _AccumulateOp
{
	template <class T>
	static T accumulate(InputIterator first, InputIterator last, T init,
			    BinaryFunction binary_op, const unsigned P)
	{
		assert(P > 0u);
		return ::std::accumulate(first, last, init, binary_op);
	}
};

template <class InputIterator>
struct _AccumulateOp<InputIterator,
    ::std::plus<typename ::std::iterator_traits<InputIterator>::value_type> >
{
	typedef ::std::plus<typename
			::std::iterator_traits<InputIterator>::value_type>
		BinaryFunction;

	template <class T>
	static T accumulate(InputIterator first, InputIterator last, T init,
			BinaryFunction binary_op, const unsigned P)
	{
		assert(P > 0u);
		return ::omptl::_Accumulate< typename
		::std::iterator_traits<InputIterator>::iterator_category>::
			accumulate(first, last, init, T(0), binary_op, P);
	}
};

template <class InputIterator>
struct _AccumulateOp<InputIterator,
    ::std::multiplies<typename
::std::iterator_traits<InputIterator>::value_type> >
{
	typedef ::std::multiplies<typename
			::std::iterator_traits<InputIterator>::value_type>
		BinaryFunction;

	template <class T>
	static T accumulate(InputIterator first, InputIterator last, T init,
			BinaryFunction binary_op, const unsigned P)
	{
		assert(P > 0u);
		return ::omptl::_Accumulate<typename
		::std::iterator_traits<InputIterator>::iterator_category>::
			accumulate(first, last, init, T(1), binary_op, P);
	}
};

template <class InputIterator, class T, class BinaryFunction>
T accumulate(InputIterator first, InputIterator last, T init,
             BinaryFunction binary_op, const unsigned P)
{
	assert(P > 0u);
	return ::omptl::_AccumulateOp<InputIterator, BinaryFunction>::
		accumulate(first, last, init, binary_op, P);
}

template <class InputIterator, class T>
T accumulate(InputIterator first, InputIterator last, T init, const unsigned P)
{
	assert(P > 0u);
	typedef typename std::iterator_traits<InputIterator>::value_type VT;

	return ::omptl::accumulate(first, last, init, std::plus<VT>(), P);
}

template <class InputIterator, class OutputIterator, class BinaryFunction>
OutputIterator adjacent_difference(InputIterator first, InputIterator last,
				   OutputIterator result,
				   BinaryFunction binary_op, const unsigned P)
{
	return ::std::adjacent_difference(first, last, result, binary_op);
}

template <class InputIterator, class OutputIterator>
OutputIterator adjacent_difference(InputIterator first, InputIterator last,
				   OutputIterator result, const unsigned P)
{
// ::std::minus<typename ::std::iterator_traits<InputIterator>::value_type>()
	return ::std::adjacent_difference(first, last, result);
}

template <class Iterator1Tag, class Iterator2Tag>
struct _InnerProduct
{
	template <class InputIterator1, class InputIterator2, class T,
        	  class BinaryFunction1, class BinaryFunction2>
	static T inner_product( InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, T init,
				BinaryFunction1 binary_op1,
				BinaryFunction2 binary_op2, const unsigned P)
	{
		return ::std::inner_product(first1, last1, first2, init,
					    binary_op1, binary_op2);
	}

	template <class Iterator1, class Iterator2, class T,
		  class BinaryFunction2>
	static T inner_product(Iterator1 first1, Iterator1 last1,
				Iterator2 first2, T init,
				::std::plus<T> binary_op1,
				BinaryFunction2 binary_op2, const unsigned P)
	{
		assert(P > 0u);
		if (_linear_serial_is_faster(first1, last1, P))
			return ::std::inner_product(first1, last1, first2,
						init, binary_op1, binary_op2);

		assert(2*(int)P <= std::distance(first1, last1));

		::std::vector< ::std::pair<Iterator1, Iterator1> >
			partitions1(P);
		::omptl::_partition_range(first1, last1, partitions1, P);
		::std::vector<Iterator2> partitions2(P);
		::omptl::_copy_partitions(partitions1, first2, partitions2, P);

		#pragma omp parallel for reduction(+:init)
		for (int t = 0; t < int(P); ++t)
			init += ::std::inner_product( partitions1[t].first,
							partitions1[t].second,
							partitions2[t], T(0.0),
							binary_op1, binary_op2);

		return init;
	}
};

template <class Iterator2Tag>
struct _InnerProduct< ::std::input_iterator_tag, Iterator2Tag>
{
	template <class InputIterator1, class InputIterator2, class T,
        	  class BinaryFunction1, class BinaryFunction2>
	static T inner_product(InputIterator1 first1, InputIterator1 last1,
                InputIterator2 first2, T init,
		BinaryFunction1 binary_op1, BinaryFunction2 binary_op2,
		const unsigned P)
	{
		return ::std::inner_product(first1, last1, first2, init,
				    binary_op1, binary_op2);
	}
};

template <class Iterator1Tag>
struct _InnerProduct<Iterator1Tag, ::std::input_iterator_tag>
{
	template <class InputIterator1, class InputIterator2, class T,
        	  class BinaryFunction1, class BinaryFunction2>
	static T inner_product(InputIterator1 first1, InputIterator1 last1,
                InputIterator2 first2, T init,
		BinaryFunction1 binary_op1, BinaryFunction2 binary_op2,
		const unsigned P)
	{
		return ::std::inner_product(first1, last1, first2, init,
				    binary_op1, binary_op2);
	}
};

template <>
struct _InnerProduct< ::std::input_iterator_tag, ::std::input_iterator_tag >
{
	template <class InputIterator1, class InputIterator2, class T,
        	  class BinaryFunction1, class BinaryFunction2>
	static T inner_product(InputIterator1 first1, InputIterator1 last1,
                InputIterator2 first2, T init,
		BinaryFunction1 binary_op1, BinaryFunction2 binary_op2,
		const unsigned P)
	{
		return ::std::inner_product(first1, last1, first2, init,
				    binary_op1, binary_op2);
	}
};


template <class InputIterator1, class InputIterator2, class T,
          class BinaryFunction1, class BinaryFunction2>
T inner_product(InputIterator1 first1, InputIterator1 last1,
                InputIterator2 first2, T init,
		BinaryFunction1 binary_op1, BinaryFunction2 binary_op2,
		const unsigned P)
{
	return _InnerProduct<
	typename ::std::iterator_traits<InputIterator1>::iterator_category,
	typename ::std::iterator_traits<InputIterator2>::iterator_category>
			::inner_product(first1, last1, first2, init,
				    binary_op1, binary_op2, P);
}

template <class InputIterator1, class InputIterator2, class T>
T inner_product(InputIterator1 first1, InputIterator1 last1,
                InputIterator2 first2, T init, const unsigned P)
{
	return ::omptl::inner_product(first1, last1, first2, init,
				::std::plus<T>(), ::std::multiplies<T>(), P);
}

template <class InputIterator, class OutputIterator, class BinaryOperation>
OutputIterator partial_sum(InputIterator first, InputIterator last,
                           OutputIterator result, BinaryOperation binary_op,
			   const unsigned P)
{
	return ::std::partial_sum(first, last, result, binary_op);
}

template <class InputIterator, class OutputIterator>
OutputIterator partial_sum(InputIterator first, InputIterator last,
                           OutputIterator result, const unsigned P)
{
// ::std::plus<typename ::std::iterator_traits<InputIterator>::value_type>(),
	return ::std::partial_sum(first, last, result);
}

} // namespace omptl

#endif /* OMPTL_NUMERIC_H */
