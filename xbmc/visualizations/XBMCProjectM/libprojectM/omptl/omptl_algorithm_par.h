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


#include <functional>
#include <utility>
#include <cmath>
#include <cstdlib>

#include "omptl_tools.h"
#include "omptl_numeric"

#include <iterator>

namespace omptl
{

/*
 * Not (yet) paralellized due to data dependance.
 */
template <class ForwardIterator>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last,
			      const unsigned P)
{
	return ::std::adjacent_find(first, last);
}

/*
 * Not (yet) paralellized due to data dependance.
 */
template <class ForwardIterator, class BinaryPredicate>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last,
                              BinaryPredicate binary_pred, const unsigned P)
{
	return ::std::adjacent_find(first, last, binary_pred);
}

template <class ForwardIterator, class T, class StrictWeakOrdering>
bool binary_search(ForwardIterator first, ForwardIterator last, const T& value,
                   StrictWeakOrdering comp, const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
		return ::std::binary_search(first, last, value, comp);

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	bool result = 0;
	#pragma omp parallel for reduction(|:result)
	for (int t = 0; t < int(P); ++t)
		result |= ::std::binary_search(partitions[t].first,
						partitions[t].second,
						value, comp);

	return result;
}

template <class ForwardIterator, class T>
bool binary_search(ForwardIterator first, ForwardIterator last, const T& value,
		   const unsigned P)
{
	typedef typename ::std::iterator_traits<ForwardIterator>::value_type VT;
	return ::omptl::binary_search(first, last, value, ::std::less<VT>());
}

template <class IteratorInTag, class IteratorOutTag>
struct Copy_
{
	template <class IteratorIn, class IteratorOut>
	static IteratorOut _copy(IteratorIn first, IteratorIn last,
				 IteratorOut result, const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::copy(first, last, result);

		::std::vector< ::std::pair<IteratorIn, IteratorIn> >
			source_partitions(P);
		::omptl::_partition_range(first, last, source_partitions, P);

		::std::vector<IteratorOut> dest_partitions(P);
		::omptl::_copy_partitions(source_partitions, result,
					  dest_partitions, P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			IteratorOut tmp;
			*( (t == int(P-1)) ? &result : &tmp )
				 = ::std::copy(source_partitions[t].first,
						source_partitions[t].second,
						dest_partitions[t]);
		}

		return result;
	}
};

template <class IteratorOutTag>
struct Copy_< ::std::input_iterator_tag, IteratorOutTag >
{
	template <class InputIterator, class OutputIterator>
	static OutputIterator _copy(InputIterator first, InputIterator last,
				    OutputIterator result, const unsigned P)
	{
		return ::std::copy(first, last, result);
	}
};

template <class IteratorInTag>
struct Copy_<IteratorInTag, ::std::output_iterator_tag>
{
	template <class InputIterator, class OutputIterator>
	static OutputIterator _copy(InputIterator first, InputIterator last,
				    OutputIterator result, const unsigned P)
	{
		return ::std::copy(first, last, result);
	}
};

template <class InputIterator, class OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last,
		    OutputIterator result, const unsigned P)
{
	return ::omptl::Copy_<
	    typename ::std::iterator_traits<InputIterator>::iterator_category,
	    typename ::std::iterator_traits<OutputIterator>::iterator_category>
		::_copy(first, last, result, P);
}

template <class BidirectionalIterator1, class BidirectionalIterator2>
BidirectionalIterator2 copy_backward(BidirectionalIterator1 first,
                                     BidirectionalIterator1 last,
                                     BidirectionalIterator2 result,
				     const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
		return ::std::copy_backward(first, last, result);

	::std::vector< ::std::pair<BidirectionalIterator1,
			BidirectionalIterator1> > source_partitions(P);
	::omptl::_partition_range(first, last, source_partitions, P);

	::std::vector<BidirectionalIterator2> dest_partitions(P);
	::omptl::_copy_partitions(source_partitions, result,
				  dest_partitions, P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
	{
		BidirectionalIterator2 tmp;
		*( (t == int(P-1)) ? &result : &tmp ) =
			::std::copy_backward(   source_partitions[t].first,
						source_partitions[t].second,
						dest_partitions[t] );
	}

	return result;
}

template <class IteratorTag>
struct Count_
{
	template <class Iterator, class EqualityComparable>
	static typename ::std::iterator_traits<Iterator>::difference_type
	count(Iterator first, Iterator last, const EqualityComparable& value,
 		const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::count(first, last, value);

		::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
		::omptl::_partition_range(first, last, partitions, P);

		typename ::std::iterator_traits<Iterator>::difference_type
			result = 0;
		#pragma omp parallel for reduction(+:result)
		for (int t = 0; t < int(P); ++t)
			result += ::std::count( partitions[t].first,
						partitions[t].second, value );

		return result;
	}
};

template <>
struct Count_< ::std::input_iterator_tag >
{
	template <class Iterator, class EqualityComparable>
	static typename ::std::iterator_traits<Iterator>::difference_type
	count(Iterator first, Iterator last, const EqualityComparable& value,
		const unsigned P)
	{
		return ::std::count(first, last, value);
	}
};

template <class InputIterator, class EqualityComparable>
typename ::std::iterator_traits<InputIterator>::difference_type
count(InputIterator first, InputIterator last,
      const EqualityComparable& value, const unsigned P)
{
	return ::omptl::Count_<typename
		::std::iterator_traits<InputIterator>::iterator_category>::
		count(first, last, value, P);
}

template <class InputIterator, class EqualityComparable, class Size>
void count(InputIterator first, InputIterator last,
           const EqualityComparable& value, Size& n, const unsigned P)
{
	n = ::omptl::count(first, last, value, P);
}


template <class IteratorTag>
struct Count_if_
{
	template <class Iterator, class Predicate>
	static typename ::std::iterator_traits<Iterator>::difference_type
	count_if(Iterator first, Iterator last, Predicate pred,
		const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::count_if(first, last, pred);

		::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
		::omptl::_partition_range(first, last, partitions, P);

		typename ::std::iterator_traits<Iterator>::difference_type
			result = 0;

		#pragma omp parallel for reduction(+:result)
		for (int t = 0; t < int(P); ++t)
			result += ::std::count_if(partitions[t].first,
						  partitions[t].second, pred);

		return result;
	}
};

template <>
struct Count_if_< ::std::input_iterator_tag >
{
	template <class InputIterator, class Predicate>
	typename ::std::iterator_traits<InputIterator>::difference_type
	static count_if(InputIterator first, InputIterator last,
			 Predicate pred, const unsigned P)
	{
		return ::std::count_if(first, last, pred);
	}
};

template <class InputIterator, class Predicate>
typename ::std::iterator_traits<InputIterator>::difference_type
count_if(InputIterator first, InputIterator last,
	 Predicate pred, const unsigned P)
{
	return ::omptl::Count_if_<typename
		::std::iterator_traits<InputIterator>::iterator_category>::
			count_if(first, last, pred, P);
}

template <class InputIterator, class Predicate, class Size>
void count_if(InputIterator first, InputIterator last,
              Predicate pred, Size& n, const unsigned P)
{
	n = ::omptl::count_if(first, last, pred, P);
}

template<class Iterator1Tag, class Iterator2Tag>
struct Equal_
{
	template <class Iterator1, class Iterator2, class BinaryPredicate>
	static bool _equal(Iterator1 first1, Iterator1 last1,
			   Iterator2 first2, BinaryPredicate binary_pred,
			   const unsigned P)
	{
		if (_linear_serial_is_faster(first1, last1, P))
			return ::std::equal(first1, last1, first2, binary_pred);

		::std::vector< ::std::pair<Iterator1, Iterator1> >
			source_partitions(P);
		::omptl::_partition_range(first1, last1, source_partitions, P);

		::std::vector<Iterator2> dest_partitions(P);
		::omptl::_copy_partitions(source_partitions, first2,
					dest_partitions, P);

		bool result = true;
		#pragma omp parallel for reduction(&:result)
		for (int t = 0; t < int(P); ++t)
			result &= ::std::equal( source_partitions[t].first,
						source_partitions[t].second,
						dest_partitions[t],binary_pred);

		return result;
	}
};

template<class Iterator2Tag>
struct Equal_<std::input_iterator_tag, Iterator2Tag>
{
	template <class InputIterator1, class Iterator2,
        	  class BinaryPredicate>
	static bool _equal(InputIterator1 first1, InputIterator1 last1,
			   Iterator2 first2, BinaryPredicate binary_pred,
			   const unsigned P)
	{
		return ::std::equal(first1, last1, first2, binary_pred);
	}
};

template<class Iterator1Tag>
struct Equal_<Iterator1Tag, std::input_iterator_tag>
{
	template <class Iterator1, class InputIterator2,
        	  class BinaryPredicate>
	static bool _equal(Iterator1 first1, Iterator1 last1,
			   InputIterator2 first2, BinaryPredicate binary_pred,
			   const unsigned P)
	{
		return ::std::equal(first1, last1, first2, binary_pred);
	}
};

template<>
struct Equal_<std::input_iterator_tag, std::input_iterator_tag>
{
	template <class InputIterator1, class InputIterator2,
        	  class BinaryPredicate>
	static bool _equal(InputIterator1 first1, InputIterator1 last1,
			   InputIterator2 first2, BinaryPredicate binary_pred,
			   const unsigned P)
	{
		return ::std::equal(first1, last1, first2, binary_pred);
	}
};

template <class InputIterator1, class InputIterator2,
          class BinaryPredicate>
bool equal(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, BinaryPredicate binary_pred, const unsigned P)
{
// 	return ::std::equal(first1, last1, first2, binary_pred);

	return ::omptl::Equal_<
	    typename ::std::iterator_traits<InputIterator1>::iterator_category,
	    typename ::std::iterator_traits<InputIterator2>::iterator_category>
		::_equal(first1, last1, first2, binary_pred, P);
}

template <class InputIterator1, class InputIterator2>
bool equal(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator1>::value_type VT;

	return ::omptl::equal(first1, last1, first2, ::std::equal_to<VT>());
}

//TODO
template <class ForwardIterator, class T, class StrictWeakOrdering>
::std::pair<ForwardIterator, ForwardIterator>
equal_range(ForwardIterator first, ForwardIterator last, const T& value,
            StrictWeakOrdering comp, const unsigned P)
{
	return ::std::equal_range(first, last, value, comp);
}

template <class ForwardIterator, class T>
::std::pair<ForwardIterator, ForwardIterator>
equal_range(ForwardIterator first, ForwardIterator last, const T& value,
            const unsigned P)
{
	typedef typename ::std::iterator_traits<ForwardIterator>::value_type VT;

	return ::omptl::equal_range(first, last, value, ::std::less<VT>(), P);
}

template <class ForwardIterator, class T>
void fill(ForwardIterator first, ForwardIterator last,
	  const T& value, const unsigned P)
{
	assert(P > 0u);
	if (_linear_serial_is_faster(first, last, P))
	{
		::std::fill(first, last, value);
		return;
	}
	assert(std::distance(first, last) >= 0);
	assert(2*(int)P <= std::distance(first, last));

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		::std::fill(partitions[t].first, partitions[t].second, value);
}

template <class IteratorTag>
struct Fill_n_
{
	template <class Iterator, class Size, class T>
	static Iterator fill_n(Iterator first, Size n, const T& value,
				const unsigned P)
	{
		assert(P > 0u);
		Iterator last = first;
		std::advance(last, n);
		if (_linear_serial_is_faster(first, last, P))
			return ::std::fill_n(first, n, value);

		const Size range = (n / P) + ( (n % P) ? 1 : 0 );
		::std::vector<Size> ranges(P);
		::std::fill_n(ranges.begin(), P - 1, range);
		ranges[P - 1] = n - (P - 1) * range;

		::std::vector<Iterator> partitions(P);
		partitions[0] = first;
		for (unsigned int i = 1; i < P; ++i)
		{
			partitions[i] = partitions[i - 1];
			::std::advance(partitions[i], range);
		}

		Iterator result;
		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			Iterator tmp;
			*( (t == int(P-1)) ? &result : &tmp )
				 = ::std::fill_n(partitions[t],
						   ranges[t], value);
		}

		return result;
	}
};

template <>
struct Fill_n_< ::std::output_iterator_tag >
{
	template <class OutputIterator, class Size, class T>
	static OutputIterator fill_n(OutputIterator first, Size n,
					 const T& value, const unsigned P)
	{
		return ::std::fill_n(first, n, value);
	}
};

template <class OutputIterator, class Size, class T>
OutputIterator fill_n(OutputIterator first, Size n,
		      const T& value, const unsigned P)
{
	return ::omptl::Fill_n_<typename
		    ::std::iterator_traits<OutputIterator>::iterator_category>::
		fill_n(first, n, value, P);
}


template <class IteratorTag>
struct Find_
{
	template <class Iterator, class EqualityComparable>
	static Iterator find(Iterator first, Iterator last,
				const EqualityComparable& value,
				const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::find(first, last, value);

		::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
		::omptl::_partition_range(first, last, partitions, P);

		std::vector<Iterator> results(P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			results[t] = ::std::find(partitions[t].first,
						 partitions[t].second, value);
			if (results[t] == partitions[t].second)
				results[t] = last;
		}

		typename std::vector<Iterator>::iterator result =
		    ::std::find_if(results.begin(),results.end(),
			::std::bind2nd(::std::not_equal_to<Iterator>(), last) );

		if ( result != results.end() )
			return *result;

		return last;
	}
};

template <>
struct Find_< ::std::input_iterator_tag >
{
	template<class InputIterator, class EqualityComparable>
	static InputIterator find(InputIterator first, InputIterator last,
			const EqualityComparable& value, const unsigned P)
	{
		return std::find(first, last, value);
	}
};

template<class InputIterator, class EqualityComparable>
InputIterator find(InputIterator first, InputIterator last,
                   const EqualityComparable& value, const unsigned P)
{
	return ::omptl::Find_< typename
		::std::iterator_traits<InputIterator>::iterator_category>::
			find(first, last, value, P);
}

template <class IteratorTag>
struct Find_if_
{
	template <class Iterator, class Predicate>
	static Iterator find_if(Iterator first, Iterator last, Predicate pred,
				const unsigned P, IteratorTag)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::find_if(first, last, pred);

		::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
		::omptl::_partition_range(first, last, partitions, P);

		::std::vector<Iterator> results(P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			results[t] = ::std::find_if(partitions[t].first,
						    partitions[t].second, pred);

			if (results[t] == partitions[t].second)
				results[t] = last;
		}

		const typename ::std::vector<Iterator>::iterator result
			 = ::std::find_if(results.begin(), results.end(),
			::std::bind2nd(::std::not_equal_to<Iterator>(), last) );

		if ( result != results.end() )
			return *result;

		return last;
	}
};

template <>
struct Find_if_< ::std::input_iterator_tag >
{
	template <class InputIterator, class Predicate>
	static InputIterator _find_if(InputIterator first, InputIterator last,
					Predicate pred, const unsigned P)
	{
		return ::std::find_if(first, last, pred);
	}
};

template<class InputIterator, class Predicate>
InputIterator find_if(InputIterator first, InputIterator last,
                      Predicate pred, const unsigned P)
{
	return ::omptl::Find_if_<typename
		::std::iterator_traits<InputIterator>::iterator_category>::
			find_if(first, last, pred, P);
}

// TODO
template <class ForwardIterator1, class ForwardIterator2,
          class BinaryPredicate>
ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
			  ForwardIterator2 first2, ForwardIterator2 last2,
			  BinaryPredicate comp, const unsigned P)
{
	return ::std::find_end(first1, last1, first2, last2, comp);
}

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
			  ForwardIterator2 first2, ForwardIterator2 last2,
			  const unsigned P)
{
// typedef typename ::std::iterator_traits<ForwardIterator1>::value_type VT;
// return ::omptl::find_end(first1, last1, first2, last2, ::std::less<VT>());
	return ::std::find_end(first1, last1, first2, last2);
}

// find_first_of suffers from a loss of efficiency, and potentially a loss of
// performance when executed in parallel!
template <class IteratorTag>
struct Find_first_of_
{
	template <class Iterator, class ForwardIterator, class BinaryPredicate>
	static Iterator
	find_first_of(Iterator first1, Iterator last1,
			ForwardIterator first2, ForwardIterator last2,
			BinaryPredicate comp, const unsigned P)
	{
		if (_linear_serial_is_faster(first1, last1, P))
			return ::std::find_first_of(first1, last1,
						first2, last2, comp);

		::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
		::omptl::_partition_range(first1, last1, partitions, P);

		::std::vector<Iterator> results(P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			results[t] = ::std::find_first_of(partitions[t].first,
							  partitions[t].second,
							  first2, last2, comp);
			if (results[t] == partitions[t].second)
				results[t] = last1;
		}

		const typename ::std::vector<Iterator>::iterator result
			= ::std::find_if(results.begin(),results.end(),
			::std::bind2nd(::std::not_equal_to<Iterator>(), last1));

		if ( result != results.end() )
			return *result;

		return last1;
	}
};

template <>
struct Find_first_of_< ::std::input_iterator_tag >
{
	template <class InputIterator, class ForwardIterator,
		  class BinaryPredicate>
	static InputIterator
	find_first_of(InputIterator first1, InputIterator last1,
			ForwardIterator first2, ForwardIterator last2,
			BinaryPredicate comp, const unsigned P)
	{
		return ::std::find_first_of(first1, last1, first2, last2, comp);
	}
};

template <class InputIterator, class ForwardIterator, class BinaryPredicate>
InputIterator find_first_of(InputIterator first1, InputIterator last1,
			ForwardIterator first2, ForwardIterator last2,
			BinaryPredicate comp, const unsigned P)
{
	return ::omptl::Find_first_of_<typename
		::std::iterator_traits<InputIterator>::iterator_category>::
		find_first_of(first1, last1, first2, last2, comp, P);
}

template <class InputIterator, class ForwardIterator>
InputIterator find_first_of(InputIterator first1, InputIterator last1,
			ForwardIterator first2, ForwardIterator last2,
			const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator>::value_type VT;
	return ::omptl::find_first_of(first1, last1, first2, last2,
					::std::equal_to<VT>());
}

template <class IteratorTag>
struct For_each_
{
	template <class Iterator, class UnaryFunction>
	static UnaryFunction for_each(Iterator first, Iterator last,
			UnaryFunction f, const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::for_each(first, last, f);

		::std::vector< ::std::pair<Iterator, Iterator> > partitions(P);
		::omptl::_partition_range(first, last, partitions, P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
			::std::for_each(partitions[t].first,
					partitions[t].second, f);

		return f;
	}
};

template <>
struct For_each_< ::std::input_iterator_tag >
{
	template <class InputIterator, class UnaryFunction>
	static UnaryFunction for_each(InputIterator first, InputIterator last,
					UnaryFunction f, const unsigned P)
	{
		return ::std::for_each(first, last, f);
	}
};

template <class InputIterator, class UnaryFunction>
UnaryFunction for_each(InputIterator first, InputIterator last,
		       UnaryFunction f, const unsigned P)
{
	return ::omptl::For_each_<typename
		::std::iterator_traits<InputIterator>::iterator_category>::
			for_each(first, last, f, P);
}

template <class ForwardIterator, class Generator>
void generate(ForwardIterator first, ForwardIterator last, Generator gen)
{
	::std::generate(first, last, gen);
}

template <class ForwardIterator, class Generator>
void par_generate(ForwardIterator first, ForwardIterator last,
		  Generator gen, const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
	{
		::std::generate(first, last, gen);
		return;
	}

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		::std::generate(partitions[t].first, partitions[t].second, gen);
}

template <class RandomAccessIterator, class StrictWeakOrdering>
void push_heap(RandomAccessIterator first, RandomAccessIterator last,
               StrictWeakOrdering comp, const unsigned P)
{
	return ::std::push_heap(first, last, comp);
}

template <class RandomAccessIterator>
void push_heap(RandomAccessIterator first, RandomAccessIterator last,
               const unsigned P)
{
// 	::std::less<typename
// 	::std::iterator_traits<RandomAccessIterator>::value_type>(),
	return ::std::push_heap(first, last);
}

template <class RandomAccessIterator, class StrictWeakOrdering>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last,
                     StrictWeakOrdering comp, const unsigned P)
{
	return ::std::pop_heap(first, last, comp);
}

template <class RandomAccessIterator>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last,
                     const unsigned P)
{
// 	::std::less<typename
// 	::std::iterator_traits<RandomAccessIterator>::value_type>
	return ::std::pop_heap(first, last);
}

template <class RandomAccessIterator, class StrictWeakOrdering>
void make_heap(RandomAccessIterator first, RandomAccessIterator last,
               StrictWeakOrdering comp, const unsigned P)
{
	return ::std::make_heap(first, last, comp);
}

template <class RandomAccessIterator>
void make_heap(RandomAccessIterator first, RandomAccessIterator last,
               const unsigned P)
{
// 	::std::less<typename
// 		::std::iterator_traits<RandomAccessIterator>::value_type>(),
	return ::std::make_heap(first, last);
}

template <class RandomAccessIterator, class StrictWeakOrdering>
void sort_heap(RandomAccessIterator first, RandomAccessIterator last,
               StrictWeakOrdering comp, const unsigned P)
{
	return ::std::sort_heap(first, last, comp);
}

template <class RandomAccessIterator>
void sort_heap(RandomAccessIterator first, RandomAccessIterator last,
               const unsigned P)
{
// 	::std::less<typename
// 		::std::iterator_traits<RandomAccessIterator>::value_type>
	return ::std::sort_heap(first, last);
}

template <class Iterator1Tag, class Iterator2Tag>
struct Includes_
{
	template <class Iterator1, class Iterator2, class StrictWeakOrdering>
	static bool includes(Iterator1 first1, Iterator1 last1,
			     Iterator2 first2, Iterator2 last2,
			     StrictWeakOrdering comp, const unsigned P)
	{
		if (_linear_serial_is_faster(first2, last2, P))
			return ::std::includes(first1, last1,
						first2, last2, comp);

		/*
		 * Includes is parallelized by splitting the second range
		 * (needles), rather than the first (the haystack).
		 */
		::std::vector< ::std::pair<Iterator2, Iterator2> >partitions(P);
		::omptl::_partition_range(first2, last2, partitions, P);

		bool result = true;

		// Hence, all needles should be found in the haystack
		#pragma omp parallel for reduction(&:result)
		for (int t = 0; t < int(P); ++t)
			result &= ::std::includes(first1, last1,
						partitions[t].first,
						partitions[t].second, comp);

		return result;
	}
};

template <class Iterator2Tag>
struct Includes_< ::std::input_iterator_tag, Iterator2Tag >
{
	template <class InputIterator1, class Iterator2,
		  class StrictWeakOrdering>
	static bool includes(InputIterator1 first1, InputIterator1 last1,
			     Iterator2 first2, Iterator2 last2,
			     StrictWeakOrdering comp, const unsigned P)
	{
		return ::std::includes(first1, last1, first2, last2, comp);
	}
};

template <class Iterator1Tag>
struct Includes_<Iterator1Tag, ::std::input_iterator_tag>
{
	template <class Iterator1, class InputIterator2,
		  class StrictWeakOrdering>
	static bool includes(Iterator1 first1, Iterator1 last1,
			     InputIterator2 first2, InputIterator2 last2,
			     StrictWeakOrdering comp, const unsigned P)
	{
		return ::std::includes(first1, last1, first2, last2, comp);
	}
};

template <>
struct Includes_< ::std::input_iterator_tag, ::std::input_iterator_tag >
{
	template <class InputIterator1, class InputIterator2,
		  class StrictWeakOrdering>
	static bool includes(InputIterator1 first1, InputIterator1 last1,
			     InputIterator2 first2, InputIterator2 last2,
			     StrictWeakOrdering comp, const unsigned P)
	{
		return ::std::includes(first1, last1, first2, last2, comp);
	}
};

template <class InputIterator1, class InputIterator2, class StrictWeakOrdering>
bool includes(InputIterator1 first1, InputIterator1 last1,
              InputIterator2 first2, InputIterator2 last2,
              StrictWeakOrdering comp, const unsigned P)
{
	typedef typename
		::std::iterator_traits<InputIterator1>::iterator_category IC1;
	typedef typename
		::std::iterator_traits<InputIterator2>::iterator_category IC2;

 	return ::omptl::Includes_<IC1, IC2>
			::includes(first1, last1, first2, last2, comp, P);
}

template <class InputIterator1, class InputIterator2>
bool includes(InputIterator1 first1, InputIterator1 last1,
              InputIterator2 first2, InputIterator2 last2,
              const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator1>::value_type VT;
	return ::omptl::includes(first1, last1, first2, last2,
				 ::std::less<VT>());
}

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                             InputIterator2 first2, InputIterator2 last2,
                             BinaryPredicate comp, const unsigned P)
{
	return ::std::lexicographical_compare(first1, last1,
					      first2, last2, comp);
}

template <class InputIterator1, class InputIterator2>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                             InputIterator2 first2, InputIterator2 last2,
                             const unsigned P)
{
// 	::std::less<typename
// 		::std::iterator_traits<InputIterator1>::value_type>
	return ::std::lexicographical_compare(first1, last1, first2, last2);
}

template <class ForwardIterator, class T, class StrictWeakOrdering>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last,
                            const T& value, StrictWeakOrdering comp,
			    const unsigned P)
{
	if (_logn_serial_is_faster(first, last, P))
		return ::std::lower_bound(first, last, value, comp);

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	::std::vector<ForwardIterator> results(P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		results[t] = ::std::lower_bound(partitions[t].first,
						partitions[t].second,
						value, comp);

	const typename ::std::vector<ForwardIterator>::iterator result =
		::std::find_if(results.begin(), results.end(),
		::std::bind2nd(::std::not_equal_to<ForwardIterator>(), last) );

	if (result != results.end())
		return *result;

	return last;
}

template <class ForwardIterator, class T>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last,
                            const T& value, const unsigned P)
{
	return ::omptl::lower_bound(first, last, value, ::std::less<T>(), P);
}

// Not parallelized, dependencies between data.
template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator merge(InputIterator1 first1, InputIterator1 last1,
                     InputIterator2 first2, InputIterator2 last2,
                     OutputIterator result,
		     StrictWeakOrdering comp, const unsigned P)
{
	return ::std::merge(first1, last1, first2, last2, result, comp);
}

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator merge(InputIterator1 first1, InputIterator1 last1,
                     InputIterator2 first2, InputIterator2 last2,
                     OutputIterator result, const unsigned P)
{
// 	::std::less<typename
// 		::std::iterator_traits<InputIterator1>::value_type>
	return ::std::merge(first1, last1, first2, last2, result);
}

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last,
                            BinaryPredicate comp, const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
		return ::std::min_element(first, last, comp);

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	::std::vector<ForwardIterator> results(P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		results[t] = ::std::min_element(partitions[t].first,
					 	partitions[t].second, comp);

	ForwardIterator result = results[0];
	for (unsigned int i = 1; i < P; ++i)
		if ( (result != last) && (results[i] != last) &&
		      comp(*results[i], *result) )
			result = results[i];

	return result;
}

template <class ForwardIterator>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last,
			    const unsigned P)
{
	typedef typename
		::std::iterator_traits<ForwardIterator>::value_type value_type;
	return ::omptl::min_element(first, last, ::std::less<value_type>(), P);
}

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator max_element(ForwardIterator first, ForwardIterator last,
                            BinaryPredicate comp, const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
		return ::std::max_element(first, last, comp);

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	::std::vector<ForwardIterator> results(P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		results[t] = ::std::max_element(partitions[t].first,
					 	partitions[t].second, comp);

	ForwardIterator result = results[0];
	for (unsigned int i = 1; i < P; ++i)
	{
		if ( (result != last) && (results[i] != last) &&
		      comp(*result, *results[i]) )
			result = results[i];
	}

	return result;
}

template <class ForwardIterator>
ForwardIterator max_element(ForwardIterator first, ForwardIterator last,
			    const unsigned P)
{
	typedef typename
		::std::iterator_traits<ForwardIterator>::value_type value_type;
	return ::omptl::max_element(first, last, ::std::less<value_type>(), P);
}

template <class Iterator1Tag, class Iterator2Tag>
struct Mismatch_
{
	template <class Iterator1, class Iterator2, class BinaryPredicate>
	static ::std::pair<Iterator1, Iterator2>
	mismatch(Iterator1 first1, Iterator1 last1, Iterator2 first2,
		BinaryPredicate binary_pred, const unsigned P)
	{
		if (_linear_serial_is_faster(first1, last1, P))
			return ::std::mismatch(first1, last1,
						first2, binary_pred);

		::std::vector< ::std::pair<Iterator1, Iterator1> >
			source_partitions(P);
		::omptl::_partition_range(first1, last1, source_partitions, P);

		::std::vector<Iterator2> dest_partitions(P);
		::omptl::_copy_partitions(source_partitions, first2,
					dest_partitions, P);

		::std::vector< ::std::pair<Iterator1, Iterator2> > results(P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
			results[t] = ::std::mismatch(source_partitions[t].first,
						source_partitions[t].second,
						dest_partitions[t],
						binary_pred);

		// This could have been done more elegantly with select1st
		for (unsigned int i = 0; i < P - 1; ++i)
			if (results[i].first != source_partitions[i].second)
				return results[i];

		return results[P - 1];
	}
};


template <class Iterator1Tag>
struct Mismatch_<Iterator1Tag, ::std::input_iterator_tag >
{
	template <class InputIterator1, class InputIterator2,
		  class BinaryPredicate>
	static ::std::pair<InputIterator1, InputIterator2>
	mismatch(InputIterator1 first1, InputIterator1 last1,
		 InputIterator2 first2, BinaryPredicate binary_pred,
		 const unsigned P)
	{
		return ::std::mismatch(first1, last1, first2, binary_pred);
	}
};

template <class Iterator2Tag>
struct Mismatch_< ::std::input_iterator_tag, Iterator2Tag >
{
	template <class InputIterator1, class InputIterator2,
		  class BinaryPredicate>
	static ::std::pair<InputIterator1, InputIterator2>
	mismatch(InputIterator1 first1, InputIterator1 last1,
		 InputIterator2 first2, BinaryPredicate binary_pred,
 		 const unsigned P)
	{
		return ::std::mismatch(first1, last1, first2, binary_pred);
	}
};

template <>
struct Mismatch_< ::std::input_iterator_tag, ::std::input_iterator_tag >
{
	template <class InputIterator1, class InputIterator2,
		  class BinaryPredicate>
	static ::std::pair<InputIterator1, InputIterator2>
	mismatch(InputIterator1 first1, InputIterator1 last1,
		 InputIterator2 first2, BinaryPredicate binary_pred,
		 const unsigned P)
	{
		return ::std::mismatch(first1, last1, first2, binary_pred);
	}
};

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
::std::pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
         BinaryPredicate binary_pred, const unsigned P)
{
	return ::omptl::Mismatch_<
	typename ::std::iterator_traits<InputIterator1>::iterator_category,
	typename ::std::iterator_traits<InputIterator2>::iterator_category>::
		mismatch(first1, last1, first2, binary_pred, P);
}

template <class InputIterator1, class InputIterator2>
::std::pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1, InputIterator1 last1,
	 InputIterator2 first2, const unsigned P)
{
	typedef typename ::std::iterator_traits<InputIterator1>::value_type VT;
	return ::omptl::mismatch(first1, last1, first2,::std::equal_to<VT>(),P);
}

// TODO How can this be parallelized ?
template <class RandomAccessIterator, class StrictWeakOrdering>
void nth_element(RandomAccessIterator first, RandomAccessIterator nth,
                 RandomAccessIterator last,
		 StrictWeakOrdering comp, const unsigned P)
{
	::std::nth_element(first, nth, last, comp);
}

template <class RandomAccessIterator>
void nth_element(RandomAccessIterator first, RandomAccessIterator nth,
                 RandomAccessIterator last, const unsigned P)
{
// 	typedef typename
// 		::std::iterator_traits<RandomAccessIterator>::value_type
// 	::std::less<VT>

	::std::nth_element(first, nth, last);
}

template <class RandomAccessIterator, class StrictWeakOrdering>
void partial_sort(RandomAccessIterator first,
                  RandomAccessIterator middle,
                  RandomAccessIterator last,
                  StrictWeakOrdering comp, const unsigned P)
{
	const typename
	::std::iterator_traits<RandomAccessIterator>::difference_type
		N = ::std::distance(first, last);
	assert(N >= 0);

	if (2*(int)P < N)
	{
		::omptl::_pivot_range(first, last, *middle);
		::omptl::sort(first, middle, comp, P);
	}
	else
		::std::partial_sort(first, last, middle);
}

template <class RandomAccessIterator>
void partial_sort(RandomAccessIterator first, RandomAccessIterator middle,
                  RandomAccessIterator last, const unsigned P)
{

	typedef typename
		::std::iterator_traits<RandomAccessIterator>::value_type VT;
	::omptl::partial_sort(first, middle, last, ::std::less<VT>(), P);
}


// Not parallelized due to dependencies.
template <class InputIterator, class RandomAccessIterator,
          class StrictWeakOrdering>
RandomAccessIterator
partial_sort_copy(InputIterator first, InputIterator last,
                  RandomAccessIterator result_first,
                  RandomAccessIterator result_last, StrictWeakOrdering comp,
		  const unsigned P)
{
	return ::std::partial_sort_copy(first, last,
					result_first, result_last, comp);
}

// Not parallelized due to dependencies.
template <class InputIterator, class RandomAccessIterator>
RandomAccessIterator
partial_sort_copy(InputIterator first, InputIterator last,
                  RandomAccessIterator result_first,
                  RandomAccessIterator result_last, const unsigned P)
{
// 		::std::less<typename
// ::std::iterator_traits<InputIterator>::value_type>(),

	return ::std::partial_sort_copy(first, last,
					result_first, result_last);
}

// Not (yet) parallelized, not straightforward due to possible dependencies
// between subtasks.
template <class ForwardIterator, class Predicate>
ForwardIterator partition(ForwardIterator first, ForwardIterator last,
			  Predicate pred, const unsigned P)
{
	return ::std::partition(first, last, pred);
}

// Not (yet) parallelized, not straightforward due to possible dependencies
// between subtasks.
template <class ForwardIterator, class Predicate>
ForwardIterator stable_partition(ForwardIterator first, ForwardIterator last,
                                 Predicate pred, const unsigned P)
{
	return ::std::stable_partition(first, last, pred);
}

template <class BidirectionalIterator, class StrictWeakOrdering>
bool next_permutation(BidirectionalIterator first, BidirectionalIterator last,
                      StrictWeakOrdering comp, const unsigned P)
{
	return ::std::next_permutation(first, last, comp);
}

template <class BidirectionalIterator>
bool next_permutation(BidirectionalIterator first, BidirectionalIterator last,
                      const unsigned P)
{
// ::std::less<typename
// ::std::iterator_traits<BidirectionalIterator>::value_type>
	return ::std::next_permutation(first, last);
}

template <class BidirectionalIterator, class StrictWeakOrdering>
bool prev_permutation(BidirectionalIterator first, BidirectionalIterator last,
                      StrictWeakOrdering comp, const unsigned P)
{
	return ::std::prev_permutation(first, last, comp);
}

template <class BidirectionalIterator>
bool prev_permutation(BidirectionalIterator first, BidirectionalIterator last,
                      const unsigned P)
{
// 		::std::less<typename
// ::std::iterator_traits<BidirectionalIterator>::value_type>(),
	return ::std::prev_permutation(first, last);
}


template <class RandomAccessIterator>
void random_shuffle(RandomAccessIterator first, RandomAccessIterator last,
                    const unsigned P)
{
	::std::random_shuffle(first, last);
}

template <class RandomAccessIterator, class RandomNumberGenerator>
void random_shuffle(RandomAccessIterator first, RandomAccessIterator last,
                    RandomNumberGenerator& rgen, const unsigned P)
{
	::std::random_shuffle(first, last, rgen);
}

// Not (yet) parallelized, not straightforward due to possible dependencies
// between subtasks.
template <class ForwardIterator, class T>
ForwardIterator remove( ForwardIterator first, ForwardIterator last,
			const T& value, const unsigned P)
{
	return ::std::remove(first, last, value);
}

// Not (yet) parallelized, not straightforward due to possible dependencies
// between subtasks.
template <class ForwardIterator, class Predicate>
ForwardIterator remove_if(ForwardIterator first, ForwardIterator last,
			  Predicate pred, const unsigned P)
{
	return ::std::remove_if(first, last, pred);
}

// Not parallelized due to possible complications with OutputIterators.
// No par_remove_copy exists due to possible dependencies between subtasks.
template <class InputIterator, class OutputIterator, class T>
OutputIterator remove_copy(InputIterator first, InputIterator last,
			      OutputIterator result, const T& value,
			   const unsigned P)
{
	return ::std::remove_copy(first, last, result, value);
}

// Not parallelized due to possible complications with OutputIterators.
// No par_remove_copy_if exists due to possible dependencies between subtasks.
template <class InputIterator, class OutputIterator, class Predicate>
OutputIterator remove_copy_if(InputIterator first, InputIterator last,
			      OutputIterator result, Predicate pred,
			      const unsigned P)
{
	return ::std::remove_copy(first, last, result, pred);
}

template <class ForwardIterator, class T>
void replace(ForwardIterator first, ForwardIterator last, const T& old_value,
             const T& new_value, const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
	{
		::std::replace(first, last, old_value, new_value);
		return;
	}

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		::std::replace(partitions[t].first, partitions[t].second,
				old_value, new_value);
}

template <class Iterator1Tag, class Iterator2Tag>
struct Replace_copy_if_
{
	template <class Iterator1, class Iterator2, class Predicate, class T>
	static Iterator2
	replace_copy_if(Iterator1 first, Iterator1 last,
		     Iterator2 result, Predicate pred,
		     const T& new_value, const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::replace_copy_if(first, last, result,
							pred, new_value);

		::std::vector< ::std::pair<Iterator1, Iterator1> >
			source_partitions(P);
		::omptl::_partition_range(first, last, source_partitions, P);
		::std::vector<Iterator2> dest_partitions(P);
		::omptl::_copy_partitions(source_partitions, result,
					  dest_partitions, P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			Iterator2 tmp;
			*( (t == int(P-1)) ? &result : &tmp )
				 = ::std::replace_copy_if(
						source_partitions[t].first,
						source_partitions[t].second,
						dest_partitions[t],
						pred, new_value);
		}

		return result;
	}

};

template <class Iterator2Tag>
struct Replace_copy_if_< ::std::input_iterator_tag, Iterator2Tag>
{
	template <class Iterator1, class Iterator2,
		  class Predicate, class T>
	static Iterator2
	replace_copy_if(Iterator1 first, Iterator1 last,
		     Iterator2 result, Predicate pred,
		     const T& new_value, const unsigned P)
	{
		return ::std::replace_copy_if(first, last, result,
						pred, new_value);
	}
};

template <class Iterator1Tag>
struct Replace_copy_if_< Iterator1Tag, ::std::output_iterator_tag>
{
	template <class Iterator1, class OutputIterator,
		  class Predicate, class T>
	static OutputIterator
	replace_copy_if(Iterator1 first, Iterator1 last,
		     OutputIterator result, Predicate pred,
		     const T& new_value, const unsigned P)
	{
		return ::std::replace_copy_if(first, last, result,
						pred, new_value);
	}
};

template <>
struct Replace_copy_if_< ::std::input_iterator_tag, ::std::output_iterator_tag>
{
	template <class InputIterator, class OutputIterator,
		  class Predicate, class T>
	static OutputIterator
	replace_copy_if(InputIterator first, InputIterator last,
		     OutputIterator result, Predicate pred,
		     const T& new_value, const unsigned P)
	{
		return ::std::replace_copy_if(first, last, result,
						pred, new_value);
	}
};

template <class InputIterator, class OutputIterator, class Predicate, class T>
OutputIterator replace_copy_if(InputIterator first, InputIterator last,
                               OutputIterator result, Predicate pred,
                               const T& new_value, const unsigned P)
{
	return ::omptl::Replace_copy_if_<
	typename ::std::iterator_traits< InputIterator>::iterator_category,
	typename ::std::iterator_traits<OutputIterator>::iterator_category>
		::replace_copy_if(first, last, result, pred, new_value, P);
}

template <class InputIterator, class OutputIterator, class T>
OutputIterator replace_copy(InputIterator first, InputIterator last,
                            OutputIterator result, const T& old_value,
                            const T& new_value, const unsigned P)
{
	return ::omptl::replace_copy_if(first, last, result,
				::std::bind2nd(::std::equal_to<T>(), old_value),
				new_value, P);
}

template <class ForwardIterator, class Predicate, class T>
void replace_if(ForwardIterator first, ForwardIterator last, Predicate pred,
                const T& new_value, const unsigned P)
{
	if (_linear_serial_is_faster(first, last, P))
		return ::std::replace_if(first, last, pred, new_value);

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		::std::replace_if(partitions[t].first, partitions[t].second,
				  pred, new_value);
}

// TODO
template <class BidirectionalIterator>
void reverse(BidirectionalIterator first, BidirectionalIterator last,
	     const unsigned P)
{
	::std::reverse(first, last);
}

// TODO
template <class BidirectionalIterator, class OutputIterator>
OutputIterator reverse_copy(BidirectionalIterator first,
			    BidirectionalIterator last,
			    OutputIterator result,
			    const unsigned P)
{
	return ::std::reverse_copy(first, last, result);
}

// TODO
template <class ForwardIterator>
ForwardIterator rotate( ForwardIterator first, ForwardIterator middle,
			ForwardIterator last,
			const unsigned P)
{
	return ::std::rotate(first, middle, last);
}

// TODO
template <class ForwardIterator, class OutputIterator>
OutputIterator rotate_copy(ForwardIterator first, ForwardIterator middle,
                           ForwardIterator last, OutputIterator result,
			   const unsigned P)
{
	return ::std::rotate(first, middle, last, result);
}
/*
This can't be right - partitioning the range might cut valid subsequences
in [first1-last1]
template <class ForwardIterator1, class ForwardIterator2,
	  class BinaryPredicate>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                        ForwardIterator2 first2, ForwardIterator2 last2,
                        BinaryPredicate binary_pred, const unsigned P)
{
	if (_linear_serial_is_faster(first1, last1, P))
		return ::std::search(first1, last1, first2, last2,
					 binary_pred);

	::std::vector< ::std::pair<ForwardIterator1, ForwardIterator1> >
		partitions(P);
	::omptl::_partition_range(first1, last1, partitions, P);

	::std::vector<ForwardIterator1> results(P);

	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
	{
		results[t] = ::std::search(partitions[t].first,
					   partitions[t].second,
					   first2, last2, binary_pred);


	}

	const typename ::std::vector<ForwardIterator1>::iterator
		result = ::std::find_if(results.begin(), results.end(),
		::std::bind2nd(::std::not_equal_to<ForwardIterator1>(),
				last1));

	if (result != results.end())
		return *result;

	return last1;
}
*/

template <class ForwardIterator1, class ForwardIterator2,
	  class BinaryPredicate>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                        ForwardIterator2 first2, ForwardIterator2 last2,
                        BinaryPredicate binary_pred, const unsigned P)
{
	return ::std::search(first1, last1, first2, last2, binary_pred);
}

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                        ForwardIterator2 first2, ForwardIterator2 last2,
                        const unsigned P)
{
// 	typedef typename
// 		::std::iterator_traits<ForwardIterator1>::value_type VT;
// 	return ::omptl::search(first1, last1, first2, last2,
// 				::std::equal_to<VT>(), P);

	return ::std::search(first1, last1, first2, last2);
}

// TODO
template <class ForwardIterator, class Integer,
          class T, class BinaryPredicate>
ForwardIterator search_n(ForwardIterator first, ForwardIterator last,
                         Integer count, const T& value,
                         BinaryPredicate binary_pred, const unsigned P)
{
	return ::std::search_n(first, last, count, value, binary_pred);
}

template <class ForwardIterator, class Integer, class T>
ForwardIterator search_n(ForwardIterator first, ForwardIterator last,
			 Integer count, const T& value, const unsigned P)
{
// ::std::equal_to<typename
// ::std::iterator_traits<ForwardIterator>::value_type>
	return ::std::search_n(first, last, count, value);
}

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result, StrictWeakOrdering comp,
				const unsigned P)
{
	return ::std::set_difference(first1, last1,
				     first2, last2, result, comp);
}

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result, const unsigned P)
{
	return ::std::set_difference(first1, last1, first2, last2, result);
}

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result, StrictWeakOrdering comp,
			 	const unsigned P)
{
	return ::std::set_intersection( first1, last1,
					first2, last2, result, comp);
}

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result, const unsigned P)
{
	return ::std::set_intersection( first1, last1, first2, last2, result);
}

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator
set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result, StrictWeakOrdering comp,
			 const unsigned P)
{
	return ::std::set_symmetric_difference( first1, last1,
						first2, last2, result, comp);
}

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator
set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result, const unsigned P)
{
	return ::std::set_symmetric_difference( first1, last1,
						first2, last2, result);
}

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator set_union(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result, StrictWeakOrdering comp,
			 const unsigned P)
{
	return ::std::set_union(first1, last1, first2, last2, result, comp);
}

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_union(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result, const unsigned P)
{
	return ::std::set_union(first1, last1, first2, last2, result);
}

template<typename RandomAccessIterator, class StrictWeakOrdering>
void sort(RandomAccessIterator first, RandomAccessIterator last,
	  StrictWeakOrdering comp, const unsigned P)
{
	if ( ::omptl::_nlogn_serial_is_faster(first, last, P) )
	{
		::std::sort(first, last, comp);
		return;
	}

	// Generate pivots
	typedef typename
	    ::std::iterator_traits<RandomAccessIterator>::value_type value_type;

	::std::vector<value_type> pivots;
	_find_pivots(first, last, pivots, P);

	// Sort sufficiently to respect pivot order
	::std::vector< ::std::pair<RandomAccessIterator, RandomAccessIterator> >
		partitions(P);
	::omptl::_partition_range_by_pivots(first, last, pivots,
					    partitions, comp, P);

	// Sort
	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		::std::sort(partitions[t].first, partitions[t].second, comp);
}

template<typename RandomAccessIterator>
void sort(RandomAccessIterator first, RandomAccessIterator last,
	  const unsigned P)
{
	typedef typename
		::std::iterator_traits<RandomAccessIterator>::value_type VT;
	::omptl::sort(first, last, std::less<VT>(), P);
}

/*
template<typename RandomAccessIterator, class StrictWeakOrdering>
void _par_stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	StrictWeakOrdering comp, const unsigned P)
{
	if ( ::omptl::_nlogn_serial_is_faster(first, last, P) )
	{
		::std::stable_sort(first, last, comp);
		return;
	}

	// Generate pivots
	::std::vector<typename
		::std::iterator_traits<RandomAccessIterator>::value_type>
			pivots;
	_find_pivots(first, last, pivots, P);

	// Sort sufficiently to respect pivot order
	::std::vector< ::std::pair<RandomAccessIterator, RandomAccessIterator> >
		partitions(P);
	::omptl::_partition_range_stable_by_pivots(first, last, pivots,
						   partitions, comp, P);

	// Sort
	#pragma omp parallel for // default(none) shared(partitions)
	for (int t = 0; t < int(P); ++t)
		::std::stable_sort(partitions[t].first,
				   partitions[t].second, comp);
}

template<typename RandomAccessIterator, class StrictWeakOrdering>
void _stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	StrictWeakOrdering comp, const unsigned P)
{
	::std::stable_sort(first, last, comp);
}

template<typename RandomAccessIterator>
void _stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	std::less<typename
::std::iterator_traits<RandomAccessIterator>::value_type>
	 comp, const unsigned P)
{
	::omptl::_par_stable_sort(first, last, comp, P);
}

// template<typename RandomAccessIterator>
// void _stable_sort(RandomAccessIterator first, RandomAccessIterator last,
// 	std::greater<
// 	typename ::std::iterator_traits<RandomAccessIterator>::value_type> comp,
//  	const unsigned P)
// {
// 	::omptl::_par_stable_sort(first, last, comp, P);
// }
*/

template<typename RandomAccessIterator, class StrictWeakOrdering>
void stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	StrictWeakOrdering comp, const unsigned P)
{
	::std::stable_sort(first, last, comp);
}

template<typename RandomAccessIterator>
void stable_sort(RandomAccessIterator first, RandomAccessIterator last,
		 const unsigned P)
{
	typedef typename
		::std::iterator_traits<RandomAccessIterator>::value_type VT;
	::omptl::stable_sort(first, last, std::less<VT>(), P);
}

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator2 swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1,
                             ForwardIterator2 first2, const unsigned P)
{
	if (_linear_serial_is_faster(first1, last1, P))
		return ::std::swap_ranges(first1, last1, first2);

	::std::vector< ::std::pair<ForwardIterator1, ForwardIterator1> >
		source_partitions(P);
	::omptl::_partition_range(first1, last1, source_partitions, P);

	::std::vector<ForwardIterator2> dest_partitions(P);
	::omptl::_copy_partitions(source_partitions, first2,
				  dest_partitions, P);

	ForwardIterator2 result;
	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
	{
		ForwardIterator2 tmp;
		*( (t == int(P-1)) ? &result : &tmp )
		 	= ::std::swap_ranges(source_partitions[t].first,
					     source_partitions[t].second,
					     dest_partitions[t]);
	}

	return result;
}

template <class IteratorInTag, class IteratorOutTag>
struct Transform_
{
	template <class IteratorIn, class IteratorOut, class UnaryFunction>
	static IteratorOut transform(IteratorIn first, IteratorIn last,
					IteratorOut result, UnaryFunction op,
					const unsigned P)
	{
		if (_linear_serial_is_faster(first, last, P))
			return ::std::transform(first, last, result, op);

		::std::vector< ::std::pair<IteratorIn, IteratorIn> >
			source_partitions(P);
		::omptl::_partition_range(first, last, source_partitions, P);

		::std::vector<IteratorOut> dest_partitions(P);
		::omptl::_copy_partitions(source_partitions, result,
					  dest_partitions, P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			IteratorOut tmp;
			*( (t == int(P-1)) ? &result : &tmp )
				= ::std::transform(source_partitions[t].first,
						source_partitions[t].second,
						dest_partitions[t], op);
		}

		return result;
	}
};

template <class IteratorInTag>
struct Transform_<IteratorInTag, ::std::output_iterator_tag>
{
	template <class InputIterator, class OutputIterator,
		  class UnaryFunction>
	static OutputIterator transform(InputIterator first, InputIterator last,
					OutputIterator result, UnaryFunction op,
			 		const unsigned P)
	{
		return ::std::transform(first, last, result, op);
	}
};

template <class IteratorOutTag>
struct Transform_< ::std::input_iterator_tag, IteratorOutTag >
{
	template <class InputIterator, class OutputIterator,
		  class UnaryFunction>
	OutputIterator transform(InputIterator first, InputIterator last,
				 OutputIterator result, UnaryFunction op,
				 const unsigned P)
	{
		return ::std::transform(first, last, result, op);
	}
};

template <>
struct Transform_< ::std::input_iterator_tag, ::std::output_iterator_tag >
{
	template <class InputIterator, class OutputIterator,
		  class UnaryFunction>
	OutputIterator transform(InputIterator first, InputIterator last,
				OutputIterator result, UnaryFunction op,
				const unsigned P)
	{
		return ::std::transform(first, last, result, op);
	}
};

template <class InputIterator, class OutputIterator, class UnaryFunction>
OutputIterator transform(InputIterator first, InputIterator last,
                         OutputIterator result, UnaryFunction op,
			 const unsigned P)
{
	return ::omptl::Transform_<
	typename ::std::iterator_traits< InputIterator>::iterator_category,
	typename ::std::iterator_traits<OutputIterator>::iterator_category>::
		transform(first, last, result, op, P);
}

template <class Iterator1Tag, class Iterator2Tag, class IteratorOutTag>
struct Transform2_
{
	template <class Iterator1, class Iterator2, class IteratorOut,
		class BinaryFunction>
	static IteratorOut transform(Iterator1 first1, Iterator1 last1,
				Iterator2 first2, IteratorOut result,
				BinaryFunction binary_op, const unsigned P)
	{
		if (_linear_serial_is_faster(first1, last1, P))
			return ::std::transform(first1, last1, first2,
						result, binary_op);

		::std::vector< ::std::pair<Iterator1, Iterator1> >
			source_partitions1(P);
		::omptl::_partition_range(first1, last1, source_partitions1, P);

		::std::vector<Iterator2> source_partitions2(P);
		::omptl::_copy_partitions(source_partitions1, first2,
					source_partitions2 , P);

		::std::vector<IteratorOut> dest_partitions(P);
		::omptl::_copy_partitions(source_partitions1, result,
					  dest_partitions, P);

		#pragma omp parallel for
		for (int t = 0; t < int(P); ++t)
		{
			IteratorOut tmp;
			*( (t == int(P-1)) ? &result : &tmp ) =
				::std::transform(source_partitions1[t].first,
						 source_partitions1[t].second,
						 source_partitions2[t],
						 dest_partitions[t], binary_op);
		}

		return result;
	}
};

template <class Iterator2Tag, class IteratorOutTag>
struct Transform2_< ::std::input_iterator_tag, Iterator2Tag, IteratorOutTag >
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		 InputIterator2 first2, OutputIterator result,
		 BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <class Iterator1Tag, class IteratorOutTag>
struct Transform2_< Iterator1Tag, ::std::input_iterator_tag, IteratorOutTag >
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, OutputIterator result,
		  BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <class Iterator1Tag, class Iterator2Tag>
struct Transform2_< Iterator1Tag, Iterator2Tag, ::std::output_iterator_tag>
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, OutputIterator result,
		  BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <class IteratorOutTag>
struct Transform2_< ::std::input_iterator_tag,
		    ::std::input_iterator_tag, IteratorOutTag >
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, OutputIterator result,
		  BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <class Iterator1Tag>
struct Transform2_< Iterator1Tag, ::std:: input_iterator_tag,
		    ::std::output_iterator_tag >
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, OutputIterator result,
		  BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <class Iterator2Tag>
struct Transform2_< ::std:: input_iterator_tag, Iterator2Tag,
		    ::std::output_iterator_tag >
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, OutputIterator result,
		  BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <>
struct Transform2_< ::std:: input_iterator_tag, ::std:: input_iterator_tag,
		    ::std::output_iterator_tag >
{
	template <class InputIterator1, class InputIterator2,
		  class OutputIterator, class BinaryFunction>
	static OutputIterator
	transform(InputIterator1 first1, InputIterator1 last1,
		  InputIterator2 first2, OutputIterator result,
		  BinaryFunction binary_op, const unsigned P)
	{
		return ::std::transform(first1, last1, first2,
					result, binary_op);
	}
};

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class BinaryFunction>
OutputIterator transform(InputIterator1 first1, InputIterator1 last1,
                         InputIterator2 first2, OutputIterator result,
                         BinaryFunction binary_op, const unsigned P)
{
	return ::omptl::Transform2_<
	typename ::std::iterator_traits<InputIterator1>::iterator_category,
	typename ::std::iterator_traits<InputIterator2>::iterator_category,
	typename ::std::iterator_traits<OutputIterator>::iterator_category>::
		transform(first1, last1, first2, result, binary_op, P);
}

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator unique(ForwardIterator first, ForwardIterator last,
                       BinaryPredicate binary_pred, const unsigned P)
{
	return ::std::unique(first, last, binary_pred);
}

template <class ForwardIterator>
ForwardIterator unique(ForwardIterator first, ForwardIterator last,
		       const unsigned P)
{
// 		       ::std::equal_to<typename
// ::std::iterator_traits<ForwardIterator>::value_type>(),
	return ::std::unique(first, last);
}

template <class InputIterator, class OutputIterator, class BinaryPredicate>
OutputIterator unique_copy(InputIterator first, InputIterator last,
                           OutputIterator result, BinaryPredicate binary_pred,
			   const unsigned P)
{
	return ::std::unique_copy(first, last, result, binary_pred);
}

template <class InputIterator, class OutputIterator>
OutputIterator unique_copy(InputIterator first, InputIterator last,
                           OutputIterator result, const unsigned P)
{
// 		       ::std::equal_to<typename
// ::std::iterator_traits<InputIterator>::value_type>(),
	return ::std::unique_copy(first, last, result);
}

template <class ForwardIterator, class T, class StrictWeakOrdering>
ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last,
                            const T& value, StrictWeakOrdering comp,
			    const unsigned P)
{
	if (_logn_serial_is_faster(first, last, P))
		return ::std::upper_bound(first, last, value, comp);

	::std::vector< ::std::pair<ForwardIterator, ForwardIterator> >
		partitions(P);
	::omptl::_partition_range(first, last, partitions, P);

	::std::vector<ForwardIterator> results(P);
	#pragma omp parallel for
	for (int t = 0; t < int(P); ++t)
		results[t] = ::std::upper_bound(partitions[t].first,
					 	partitions[t].second,
						value, comp);

	// There has to be a better way...
	for (unsigned int i = P - 1; i > 0; --i)
		if (results[i] != partitions[i].second)
			return results[i];

	return results[0];
}

template <class ForwardIterator, class T>
ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last,
                            const T& value, const unsigned P)
{
	typedef typename ::std::iterator_traits<ForwardIterator>::value_type VT;
	return ::omptl::upper_bound(first, last, value, ::std::less<VT>(), P);
}

} /* namespace omptl */

