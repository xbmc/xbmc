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


#ifndef OMPTL_ALGORITHM
#define OMPTL_ALGORITHM 1

#include <algorithm>
#include "omptl"

namespace omptl
{

template <class ForwardIterator>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last,
			const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last,
                              BinaryPredicate binary_pred,
			      const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T, class StrictWeakOrdering>
bool binary_search(ForwardIterator first, ForwardIterator last, const T& value,
                   StrictWeakOrdering comp,
		   const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
bool binary_search(ForwardIterator first, ForwardIterator last, const T& value,
		   const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last,
		    OutputIterator result,
		    const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator1, class BidirectionalIterator2>
BidirectionalIterator2 copy_backward(BidirectionalIterator1 first,
                                     BidirectionalIterator1 last,
                                     BidirectionalIterator2 result,
			 	     const unsigned P = _Pfunc::Pfunc());

/*
 * Note: implementation assumes that
 * ::std::iterator_traits<InputIterator>::difference_type(0) will
 * return a difference_type representing zero.
 */
template <class InputIterator, class EqualityComparable>
typename ::std::iterator_traits<InputIterator>::difference_type
count(InputIterator first, InputIterator last, const EqualityComparable& value,
		   const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class EqualityComparable, class Size>
void count(InputIterator first, InputIterator last,
           const EqualityComparable& value,
           Size& n, const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class Predicate>
typename InputIterator::difference_type
count_if(InputIterator first, InputIterator last, Predicate pred,
	 const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class Predicate, class Size>
void count_if(InputIterator first, InputIterator last,
              Predicate pred, Size& n, const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2,
          class BinaryPredicate>
bool equal(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, BinaryPredicate binary_pred,
	   const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2>
bool equal(InputIterator1 first1, InputIterator1 last1,
           InputIterator2 first2, const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T, class StrictWeakOrdering>
::std::pair<ForwardIterator, ForwardIterator>
equal_range(ForwardIterator first, ForwardIterator last, const T& value,
            StrictWeakOrdering comp,
	    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
::std::pair<ForwardIterator, ForwardIterator>
equal_range(ForwardIterator first, ForwardIterator last, const T& value,
	    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
void fill(ForwardIterator first, ForwardIterator last, const T& value,
	  const unsigned P = _Pfunc::Pfunc());

template <class OutputIterator, class Size, class T>
OutputIterator fill_n(OutputIterator first, Size n, const T& value,
		      const unsigned P = _Pfunc::Pfunc());

/*
 * find suffers from a loss of efficiency when executed in parallel!
 */
template<class InputIterator, class EqualityComparable>
InputIterator find(InputIterator first, InputIterator last,
                   const EqualityComparable& value,
		   const unsigned P = _Pfunc::Pfunc());

/*
 * find_if suffers from a loss of efficiency when executed in parallel!
 */
template<class InputIterator, class Predicate>
InputIterator find_if(InputIterator first, InputIterator last,
                      Predicate pred, const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator1, class ForwardIterator2,
          class BinaryPredicate>
ForwardIterator1
find_end(ForwardIterator1 first1, ForwardIterator1 last1,
         ForwardIterator2 first2, ForwardIterator2 last2,
         BinaryPredicate comp, const unsigned P = _Pfunc::Pfunc() );

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 find_end(ForwardIterator1 first1, ForwardIterator1 last1,
         		  ForwardIterator2 first2, ForwardIterator2 last2,
	 		  const unsigned P = _Pfunc::Pfunc() );

/*
 * find_first_of suffers from a loss of efficiency when executed in parallel!
 */
template <class InputIterator, class ForwardIterator, class BinaryPredicate>
InputIterator find_first_of(InputIterator first1, InputIterator last1,
                            ForwardIterator first2, ForwardIterator last2,
                            BinaryPredicate comp,
			    const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class ForwardIterator>
InputIterator find_first_of(InputIterator first1, InputIterator last1,
                            ForwardIterator first2, ForwardIterator last2,
			    const unsigned P = _Pfunc::Pfunc());

// Calls to UnaryFunction f must be completely independent
template <class InputIterator, class UnaryFunction>
UnaryFunction for_each(InputIterator first, InputIterator last, UnaryFunction f,
			const unsigned P = _Pfunc::Pfunc());

// Not parallellized, Generator is explicitly allowed and expected to return
// different results on subsequent calls. Order is therefor respected unless
// the programmer explicitly accepts responsibility and calls par_generate.
template <class ForwardIterator, class Generator>
void generate(ForwardIterator first, ForwardIterator last, Generator gen);

template <class ForwardIterator, class Generator>
void par_generate(ForwardIterator first, ForwardIterator last, Generator gen,
	      const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class StrictWeakOrdering>
void push_heap(RandomAccessIterator first, RandomAccessIterator last,
               StrictWeakOrdering comp,
	       const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
void push_heap(RandomAccessIterator first, RandomAccessIterator last,
	       const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class StrictWeakOrdering>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last,
                     StrictWeakOrdering comp,
		     const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
inline void pop_heap(RandomAccessIterator first, RandomAccessIterator last,
		     const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class StrictWeakOrdering>
void make_heap(RandomAccessIterator first, RandomAccessIterator last,
               StrictWeakOrdering comp,
		const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
void make_heap(RandomAccessIterator first, RandomAccessIterator last,
		const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class StrictWeakOrdering>
void sort_heap(RandomAccessIterator first, RandomAccessIterator last,
               StrictWeakOrdering comp,
		 const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
void sort_heap(RandomAccessIterator first, RandomAccessIterator last,
		 const unsigned P = _Pfunc::Pfunc());

// Warning: includes loses some efficiency in parallel.
template <class InputIterator1, class InputIterator2, class StrictWeakOrdering>
bool includes(InputIterator1 first1, InputIterator1 last1,
              InputIterator2 first2, InputIterator2 last2,
              StrictWeakOrdering comp, const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2>
bool includes(InputIterator1 first1, InputIterator1 last1,
              InputIterator2 first2, InputIterator2 last2,
	      const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                             InputIterator2 first2, InputIterator2 last2,
                             BinaryPredicate comp,
			     const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1,
                             InputIterator2 first2, InputIterator2 last2,
			     const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T, class StrictWeakOrdering>
ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last,
                            const T& value, StrictWeakOrdering comp,
			    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
ForwardIterator
lower_bound(ForwardIterator first, ForwardIterator last,
	    const T& value, const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator merge(InputIterator1 first1, InputIterator1 last1,
                     InputIterator2 first2, InputIterator2 last2,
                     OutputIterator result,
		     StrictWeakOrdering comp,
		     const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator merge(InputIterator1 first1, InputIterator1 last1,
                     InputIterator2 first2, InputIterator2 last2,
                     OutputIterator result,
		     const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last,
                            BinaryPredicate comp,
			    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last,
			    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator max_element(ForwardIterator first, ForwardIterator last,
                            BinaryPredicate comp,
			    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator>
ForwardIterator max_element(ForwardIterator first, ForwardIterator last,
			    const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class BinaryPredicate>
::std::pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
         BinaryPredicate binary_pred, const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2>
::std::pair<InputIterator1, InputIterator2>
mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
	 const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class StrictWeakOrdering>
void nth_element(RandomAccessIterator first, RandomAccessIterator nth,
                 RandomAccessIterator last, StrictWeakOrdering comp,
		 const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
void nth_element(RandomAccessIterator first, RandomAccessIterator nth,
                 RandomAccessIterator last,
		 const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class StrictWeakOrdering>
void partial_sort(RandomAccessIterator first,
                  RandomAccessIterator middle,
                  RandomAccessIterator last,
                  StrictWeakOrdering comp,
		  const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
void partial_sort(RandomAccessIterator first,
                  RandomAccessIterator middle,
                  RandomAccessIterator last,
		  const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class RandomAccessIterator,
          class StrictWeakOrdering>
RandomAccessIterator
partial_sort_copy(InputIterator first, InputIterator last,
                  RandomAccessIterator result_first,
                  RandomAccessIterator result_last, StrictWeakOrdering comp,
		  const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class RandomAccessIterator>
RandomAccessIterator
partial_sort_copy(InputIterator first, InputIterator last,
                  RandomAccessIterator result_first,
                  RandomAccessIterator result_last,
		  const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class Predicate>
ForwardIterator partition(ForwardIterator first, ForwardIterator last,
			  Predicate pred,
			  const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator, class StrictWeakOrdering>
bool next_permutation(BidirectionalIterator first, BidirectionalIterator last,
                      StrictWeakOrdering comp,
		      const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator>
bool next_permutation(BidirectionalIterator first, BidirectionalIterator last,
		      const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator, class StrictWeakOrdering>
bool prev_permutation(BidirectionalIterator first, BidirectionalIterator last,
                      StrictWeakOrdering comp,
		      const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator>
bool prev_permutation(BidirectionalIterator first, BidirectionalIterator last,
		      const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class Predicate>
ForwardIterator
stable_partition(ForwardIterator first, ForwardIterator last,
		 Predicate pred, const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator>
void random_shuffle(RandomAccessIterator first, RandomAccessIterator last,
		    const unsigned P = _Pfunc::Pfunc());

template <class RandomAccessIterator, class RandomNumberGenerator>
void random_shuffle(RandomAccessIterator first, RandomAccessIterator last,
                    RandomNumberGenerator &rgen,
		    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
ForwardIterator remove(ForwardIterator first, ForwardIterator last,
                       const T& value, const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class Predicate>
ForwardIterator remove_if(ForwardIterator first, ForwardIterator last,
                          Predicate pred,
			  const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator, class T>
OutputIterator remove_copy(InputIterator first, InputIterator last,
                           OutputIterator result, const T& value,
			   const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator, class Predicate>
OutputIterator remove_copy_if(InputIterator first, InputIterator last,
                              OutputIterator result, Predicate pred,
			      const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
void replace(ForwardIterator first, ForwardIterator last, const T& old_value,
             const T& new_value, const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator, class T>
OutputIterator replace_copy(InputIterator first, InputIterator last,
                            OutputIterator result, const T& old_value,
                            const T& new_value,
			    const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator, class Predicate, class T>
OutputIterator replace_copy_if(InputIterator first, InputIterator last,
                               OutputIterator result, Predicate pred,
                               const T& new_value,
			       const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class Predicate, class T>
void replace_if(ForwardIterator first, ForwardIterator last, Predicate pred,
                const T& new_value,
		const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator>
void reverse(BidirectionalIterator first, BidirectionalIterator last,
	     const unsigned P = _Pfunc::Pfunc());

template <class BidirectionalIterator, class OutputIterator>
OutputIterator reverse_copy(BidirectionalIterator first,
			    BidirectionalIterator last,
			    OutputIterator result,
			    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator>
ForwardIterator rotate( ForwardIterator first, ForwardIterator middle,
			ForwardIterator last,
			const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class OutputIterator>
OutputIterator rotate_copy(ForwardIterator first, ForwardIterator middle,
                           ForwardIterator last, OutputIterator result,
			   const unsigned P = _Pfunc::Pfunc());

// search suffers from a loss of efficiency when executed in parallel!
template <class ForwardIterator1, class ForwardIterator2, class BinaryPredicate>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                        ForwardIterator2 first2, ForwardIterator2 last2,
                        BinaryPredicate binary_pred,
			const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1,
                        ForwardIterator2 first2, ForwardIterator2 last2,
			const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class Integer,
          class T, class BinaryPredicate>
ForwardIterator search_n(ForwardIterator first, ForwardIterator last,
                         Integer count, const T& value,
                         BinaryPredicate binary_pred,
			 const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class Integer, class T>
ForwardIterator search_n(ForwardIterator first, ForwardIterator last,
                         Integer count, const T& value,
			const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result, StrictWeakOrdering comp,
				const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result,
				const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result, StrictWeakOrdering comp,
			 	const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1,
				InputIterator2 first2, InputIterator2 last2,
				OutputIterator result,
				const unsigned P = _Pfunc::Pfunc());


template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator
set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result, StrictWeakOrdering comp,
			 const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator
set_symmetric_difference(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result,
			 const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class StrictWeakOrdering>
OutputIterator set_union(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result, StrictWeakOrdering comp,
			 const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator>
OutputIterator set_union(InputIterator1 first1, InputIterator1 last1,
			 InputIterator2 first2, InputIterator2 last2,
			 OutputIterator result,
			 const unsigned P = _Pfunc::Pfunc());


template<typename RandomAccessIterator>
void stable_sort(RandomAccessIterator first, RandomAccessIterator last,
			const unsigned P = _Pfunc::Pfunc());

template<typename RandomAccessIterator, class StrictWeakOrdering>
void stable_sort(RandomAccessIterator first, RandomAccessIterator last,
	StrictWeakOrdering comp, const unsigned P = _Pfunc::Pfunc());

template<typename RandomAccessIterator>
void sort(RandomAccessIterator first, RandomAccessIterator last,
			const unsigned P = _Pfunc::Pfunc());

template<typename RandomAccessIterator, class StrictWeakOrdering>
void sort(RandomAccessIterator first, RandomAccessIterator last,
	  StrictWeakOrdering comp, const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator2 swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1,
				ForwardIterator2 first2,
				const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator, class UnaryFunction>
OutputIterator transform(InputIterator first, InputIterator last,
                         OutputIterator result, UnaryFunction op,
			 const unsigned P = _Pfunc::Pfunc());

template <class InputIterator1, class InputIterator2, class OutputIterator,
          class BinaryFunction>
OutputIterator transform(InputIterator1 first1, InputIterator1 last1,
                         InputIterator2 first2, OutputIterator result,
                         BinaryFunction binary_op,
			 const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator unique(ForwardIterator first, ForwardIterator last,
                       BinaryPredicate binary_pred,
		       const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator>
ForwardIterator unique(ForwardIterator first, ForwardIterator last,
		       const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator, class BinaryPredicate>
OutputIterator unique_copy(InputIterator first, InputIterator last,
                           OutputIterator result, BinaryPredicate binary_pred,
		       const unsigned P = _Pfunc::Pfunc());

template <class InputIterator, class OutputIterator>
OutputIterator unique_copy(InputIterator first, InputIterator last,
                           OutputIterator result,
		       const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T, class StrictWeakOrdering>
ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last,
			    const T& value, StrictWeakOrdering comp,
			    const unsigned P = _Pfunc::Pfunc());

template <class ForwardIterator, class T>
ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last,
			    const T& value,
			    const unsigned P = _Pfunc::Pfunc());

} // namespace omptl

#ifdef _OPENMP
  #include "omptl_algorithm_par.h"
#else
  #include "omptl_algorithm_ser.h"
#endif

#endif /* OMPTL_ALGORITHM */
