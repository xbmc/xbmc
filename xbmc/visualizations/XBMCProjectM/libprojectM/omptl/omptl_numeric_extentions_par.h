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

namespace omptl
{

// transform_accumulate
template <class Iterator, class T, class UnaryFunction, class BinaryFunction>
T _ser_transform_accumulate(Iterator first, Iterator last, T init,
			UnaryFunction unary_op, BinaryFunction binary_op)
{
	// serial version
	while (first != last)
	{
		init = binary_op(unary_op(*first), init);
		++first;
	}

	return init;
}

template <class Iterator,class T, class UnaryFunction, class BinaryFunction>
T _par_transform_accumulate(Iterator first, Iterator last,
			const T init, const T par_init,
			UnaryFunction unary_op, BinaryFunction binary_op,
			const unsigned P = omp_get_max_threads())
{
	assert(P > 0u);
	if (_linear_serial_is_faster(first, last, P))
		return _ser_transform_accumulate(first, last, init,
						unary_op, binary_op);
	assert(2*(int)P <= std::distance(first, last));

	::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	::std::vector<T> results(P);
	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		results[t] = _ser_transform_accumulate(partitions[t].first,
					partitions[t].second, par_init,
					unary_op, binary_op);

	return ::std::accumulate(results.begin(), results.end(),
				 init, binary_op);
}

template <class Iterator, class T, class UnaryFunction>
T _transform_accumulate(Iterator first, Iterator last,
		const T init, UnaryFunction unary_op,
		::std::plus<typename UnaryFunction::result_type> binary_op,
		const unsigned P = omp_get_max_threads())
{
	return ::omptl::_par_transform_accumulate(first, last, init,
				typename UnaryFunction::result_type(0),
					 unary_op, binary_op, P);
}

template <class Iterator, class T, class UnaryFunction>
T _transform_accumulate(Iterator first, Iterator last, const T init,
		UnaryFunction unary_op,
		::std::multiplies<typename UnaryFunction::result_type>binary_op,
		const unsigned P = omp_get_max_threads())
{
	return ::omptl::_par_transform_accumulate(first, last, init,
				typename UnaryFunction::result_type(1),
					 unary_op, binary_op, P);
}

template <class Iterator, class T, class UnaryFunction, class BinaryFunction>
T _transform_accumulate(Iterator first, Iterator last, const T init,
			UnaryFunction unary_op, BinaryFunction binary_op,
			const unsigned P = omp_get_max_threads())
{
	return ::omptl::_ser_transform_accumulate(first, last, init,
						  unary_op,binary_op);
}

template <class IteratorTag>
struct _TransformAccumulate
{
	template <class Iterator, class T, class UnaryFunction,
		class BinaryFunction>
	static typename BinaryFunction::result_type
	transform_accumulate(Iterator first, Iterator last, const T init,
			UnaryFunction unary_op, BinaryFunction binary_op,
			const unsigned P = omp_get_max_threads())
	{
		return ::omptl::_transform_accumulate(first, last, init,
							unary_op, binary_op, P);
	}
};

template <>
struct _TransformAccumulate< ::std::input_iterator_tag >
{
	template <class Iterator, class T, class UnaryFunction,
		class BinaryFunction>
	static typename BinaryFunction::result_type
	transform_accumulate(Iterator first, Iterator last, const T init,
			UnaryFunction unary_op, BinaryFunction binary_op,
			const unsigned P = omp_get_max_threads())
	{
		return ::omptl::_ser_transform_accumulate(first, last, init,
							  unary_op, binary_op);
	}
};

template <class Iterator, class T, class UnaryFunction, class BinaryFunction>
T transform_accumulate(Iterator first, Iterator last, const T init,
			UnaryFunction unary_op, BinaryFunction binary_op,
			const unsigned P = omp_get_max_threads())
{
	return ::omptl::_TransformAccumulate
	<typename ::std::iterator_traits<Iterator>::iterator_category>
		::transform_accumulate(first, last, init,
					unary_op, binary_op, P);
}

template <class Iterator, class T, class UnaryFunction>
T transform_accumulate(Iterator first, Iterator last,
			const T init, UnaryFunction unary_op,
			const unsigned P=omp_get_max_threads())
{
	typedef typename UnaryFunction::result_type RT;
	return ::omptl::transform_accumulate(first, last, init, unary_op,
					     ::std::plus<RT>(), P);
}

} /* namespace omptl */
