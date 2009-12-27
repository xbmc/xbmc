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

#ifndef OMPTL_TOOLS_H
#define OMPTL_TOOLS_H 1

#include <utility>
#include <vector>
#include <cassert>
#include <algorithm>
#include <climits>

namespace omptl
{

// Log of the number of operations that is expected to run faster in a single
// thread.
const unsigned C = 12;

template <typename T>
T log2N_(T n)
{
	if (n == 0)
		return 0;

	const unsigned b[] =
#if (WORD_BIT == 32)
		{0x2u, 0xCu, 0xF0u, 0xFF00u, 0xFFFF0000u};
#else // 64-bit
		{0x2u, 0xCu, 0xF0u, 0xFF00u, 0xFFFF0000u, 0xFFFFFFFF00000000u};
	assert(WORD_BIT == 64);
#endif
	const T S[] = {1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u};

	T result = 0u; // result of log2(v) will go here
	for (int i = static_cast<int>(sizeof(T)); i >= 0; --i)
	{
		if (n & b[i])
		{
			n >>= S[i];
			result |= S[i];
		}
	}

	return result;
}

template<typename Iterator>
bool _linear_serial_is_faster(Iterator first, Iterator last,
			     const unsigned P)
{
	assert(P > 0u);
	assert(::std::distance(first, last) >= 0);
	const unsigned N = ::std::distance(first, last);

	return (N < 2u*P) || (log2N_(N) < C);
}

template<typename Iterator>
bool _logn_serial_is_faster(Iterator first, Iterator last,
			    const unsigned P)
{
	assert(P > 0u);
	assert(::std::distance(first, last) >= 0);
	const unsigned N = ::std::distance(first, last);

	return (N < 2u*P) || (log2N_(N) < (1 << C));
}

template<typename Iterator>
bool _nlogn_serial_is_faster(Iterator first, Iterator last,
			    const unsigned P)
{
	assert(P > 0u);
	assert(::std::distance(first, last) >= 0);
	const unsigned N = ::std::distance(first, last);

	return (N < 2u*P) || (N*log2N_(N) < (1 << C));
}

template<typename Iterator1, typename Iterator2>
void _copy_partitions(const ::std::vector< ::std::pair<Iterator1, Iterator1> >
			&source_partitions, Iterator2 first,
		::std::vector<Iterator2> &dest_partitions, const unsigned P)
{
	assert(source_partitions.size() == P);
	assert(dest_partitions.size() == P);
	for (unsigned i = 0; i < P; ++i)
	{
		dest_partitions[i] = first;

		// The last "advance" is very important, it may create space
		// if it is an InsertIterator or something like that.
		::std::advance(first, ::std::distance(
						source_partitions[i].first,
						source_partitions[i].second) );
	}
}

// Divide a given range into P partitions
template<typename Iterator>
void _partition_range(Iterator first, Iterator last,
		::std::vector< ::std::pair<Iterator, Iterator> > &partitions,
		const unsigned P)
{
	assert(partitions.size() == P);

	typedef ::std::pair<Iterator, Iterator> Partition;

	const unsigned N = ::std::distance(first, last);
	const unsigned range = N / P + ((N%P)? 1 : 0);
	assert(2u*P <= N);
	assert(range <= N);

	// All but last partition have same range
	Iterator currentLast = first;
	::std::advance(currentLast, range);
	for (unsigned i = 0; i < P - 1; ++i)
	{
		partitions[i] = Partition(first, currentLast);
		first = currentLast;
		::std::advance(currentLast, range);
	}

	// Last range may be shorter
	assert(::std::distance(first, last) <= range);
	partitions[P - 1] = Partition(first, last);
}

// Given a range, re-arrange the items such that all elements smaller than
// the pivot precede all other values. Returns an Iterator to the first
// element not smaller than the pivot.
template<typename Iterator, class StrictWeakOrdering>
Iterator _stable_pivot_range(Iterator first, Iterator last,
	const typename ::std::iterator_traits<Iterator>::value_type pivot,
	StrictWeakOrdering comp = std::less<
		typename ::std::iterator_traits<Iterator>::value_type>())
{
	Iterator pivotIt = last;
	while (first < last)
	{
		if (comp(*first, pivot))
			++first;
		else
		{
			Iterator high = first;
			while ( (++high < last) && !comp(*high, pivot) )
				/* nop */;
			if (high < last)
				::std::iter_swap(first, last);
			first = pivotIt = ++high;
		}
	}

	return pivotIt;
}

template<typename Iterator, class StrictWeakOrdering>
Iterator _pivot_range(Iterator first, Iterator last,
	const typename ::std::iterator_traits<Iterator>::value_type pivot,
	StrictWeakOrdering comp)
{
	while (first < last)
	{
		if (comp(*first, pivot))
			++first;
		else
		{
			while ( (first < --last) && !comp(*last, pivot) )
				/* nop */;
			::std::iter_swap(first, last);
		}
	}

	return last;
}

template<typename Iterator, class StrictWeakOrdering>
void _partition_range_by_pivots(Iterator first, Iterator last,
	const ::std::vector<typename
		    ::std::iterator_traits<Iterator>::value_type> &pivots,
	::std::vector< ::std::pair<Iterator, Iterator> > &partitions,
	StrictWeakOrdering comp, const unsigned P)
{
	assert(partitions.size() == P);
	typedef ::std::pair<Iterator, Iterator> Partition;
	::std::vector<Iterator> ptable(P);
	::std::vector<typename ::std::iterator_traits<Iterator>::value_type>
		pvts(pivots.size());

	::std::vector<Iterator> borders;

	::std::vector<bool> used(pivots.size());
	::std::fill(used.begin(), used.end(), false);

	borders.push_back(first);
	borders.push_back(last);
	partitions[0].first  = first;
	partitions[0].second = last;
	for (unsigned p = 1; (1 << p) <= (int)P; ++p)
	{
		const int PROC = (1 << p);
		const int PIVOTS = (1 << (p-1));
		assert(PIVOTS <= (int)pivots.size());

		#pragma omp parallel for //default(none) shared(used, pvts)
		for (int t = 0; t < PIVOTS; ++t)
		{
			const int index = int(P / PROC) +
						2 * t * int(P / PROC) - 1;
			assert(index < (int)pivots.size());
			assert(!used[index]);
			used[index] = true;
			pvts[t] = pivots[index];
/*::std::cout << "pvts T: " << t << " --> " << index <<
	" " << pvts[t] << ::std::endl;*/
		}

		#pragma omp parallel for //default(none) private(comp)
// 				shared(ptable, pvts, partitions)
		for (int t = 0; t < PIVOTS; ++t)
			ptable[t] = _pivot_range(partitions[t].first,
						 partitions[t].second,
						 pvts[t], comp);

		for (int i = 0; i < PIVOTS; ++i)
		{
// ::std::cout << "ADD: " << ::std::distance(first, ptable[t]) << ::std::endl;
			borders.push_back(ptable[i]);
		}

		::std::sort(borders.begin(), borders.end());

		for (unsigned i = 0; i < borders.size() - 1; ++i)
		{
			partitions[i].first	= borders[i];
			partitions[i].second	= borders[i + 1];
		}

/*::std::cout << "PASS: " << p << ::std::endl;
		for (t = 0; t < (1 << p); ++t)
::std::cout << t << ": " << ::std::distance(first, partitions[t].first)
		<< " - " << ::std::distance(first, partitions[t].second)
		<< ::std::endl;*/
	}

	for (unsigned i = 0; i < pivots.size(); ++i)
		if(!used[i])
			pvts[i] = pivots[i];

	#pragma omp parallel for // default(none) private(t, comp)
// 				shared(used, ptable, pvts, partitions)
	for (int t = 0; t < int(pivots.size()); ++t)
		if (!used[t])
			ptable[t] = _pivot_range(partitions[t].first,
						partitions[t].second,
						pvts[t], comp);


	for (unsigned i = 0; i < pivots.size(); ++i)
	{
		if (!used[i])
		{
// ::std::cout << "LAST ADD: " << ::std::distance(first, ptable[i])
// 	<< ::std::endl;
			borders.push_back(ptable[i]);
		}
	}

	::std::sort(borders.begin(), borders.end());

	assert(borders.size() - 1 == P);
	for (unsigned i = 0; i < P; ++i)
	{
		partitions[i].first	= borders[i];
		partitions[i].second	= borders[i + 1];
	}

// ::std::cout << "LAST: " << p << ::std::endl;
// 	for (t = 0; t < P; ++t)
// ::std::cout << t << ": " << ::std::distance(first, partitions[t].first)
// 	<< " - " << ::std::distance(first, partitions[t].second)
// 	<< ::std::endl;

}

template<typename Iterator>
void _partition_range_stable_by_pivots(Iterator first, Iterator last,
	const ::std::vector<typename
			::std::iterator_traits<Iterator>::value_type> &pivots,
	::std::vector< ::std::pair<Iterator, Iterator> > &partitions,
	std::less<typename ::std::iterator_traits<Iterator>::value_type> comp,
	const unsigned P)
{
	typedef ::std::pair<Iterator, Iterator> Partition;

	Iterator start = first;
	for (unsigned i = 0; i < P - 1; ++i)
	{
		Iterator low = start;

		while (low < last)
		{
			// Find a value not lower than the pivot.
			while( (*low < pivots[i]) && (low < last) )
				::std::advance(low, 1);

			// Entire range scanned ?
			if (low == last) break;

			// Find a value lower than the pivot, starting from
			// low, working our way up.
			Iterator high = low;
			::std::advance(high, 1);
			while( !(*high < pivots[i]) && (high < last) )
				::std::advance(high, 1);

			// Entire range scanned ?
			if (high == last) break;

			// Swap values
			assert( !(*low<pivots[i]) && (*high<pivots[i]) );
			::std::iter_swap(low, high);
		}

		partitions[i] = Partition(start, low);
		start = low;
	}
	partitions[P - 1] = Partition(start, last);
}

/*
 * The sample ratio is used to sample more data. This way, the pivots can be
 * chosen more wisely, which is our only guarantee we can generate partitions
 * of equal size.
 */
template<typename RandomAccessIterator>
void _find_pivots(RandomAccessIterator first, RandomAccessIterator last,
	::std::vector<typename
	::std::iterator_traits<RandomAccessIterator>::value_type> &pivots,
	const unsigned P, unsigned SAMPLE_RATIO = 10)
{
	assert(SAMPLE_RATIO > 0);
	const unsigned N = ::std::distance(first, last);
	assert(N >= 2u*P);

	// Adjust the constant. Erm.
	while (SAMPLE_RATIO * (P + 1) > N)
		SAMPLE_RATIO /= 2;

	pivots.clear();
	pivots.reserve(P - 1);

	typedef typename
	    ::std::iterator_traits<RandomAccessIterator>::value_type value_type;

	::std::vector<value_type> samples;
	const unsigned NSAMPLES = SAMPLE_RATIO * P + SAMPLE_RATIO;
	samples.reserve(NSAMPLES);

	for (unsigned i = 0; i < NSAMPLES; ++i)
	{
		const unsigned offset = i * (N-1) / (NSAMPLES - 1);
		assert(offset < N);
		samples.push_back(*(first + offset));
// std::cout << "offset: " << offset << " sample: " << samples[i] << std::endl;
	}
	assert(samples.size() == NSAMPLES);

	// Sort samples to create relative ordering in data
	::std::sort(samples.begin(), samples.end());

	// Take pivots from sampled data
	for (unsigned i = 1; i < P; ++i)
	{
		pivots.push_back(samples[i * samples.size() / P]);
/*std::cout << "pivot: " << i << " idx: " << (i * samples.size() / P)
	<< " " << pivots[i-1] << std::endl;*/
	}
	assert(pivots.size() == P - 1);
}

}  // namespace omptl

#endif /* OMPTL_TOOLS_H */
