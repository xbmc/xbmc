/* mySTL algorithm.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL.
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


/* mySTL algorithm implements max, min, for_each, swap, find_if, copy,
 * copy_backward, fill
 */

#ifndef mySTL_ALGORITHM_HPP
#define mySTL_ALGORITHM_HPP


namespace mySTL {


template<typename T>
inline const T& max(const T& a, const T&b)
{
    return a < b ? b : a;
}


template<typename T>
inline const T& min(const T& a, const T&b)
{
    return b < a ? b : a;
}


template<typename InIter, typename Func>
Func for_each(InIter first, InIter last, Func op)
{
    while (first != last) {
        op(*first);
        ++first;
    }
    return op;
}


template<typename T>
inline void swap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}


template<typename InIter, typename Pred>
InIter find_if(InIter first, InIter last, Pred pred)
{
    while (first != last && !pred(*first))
        ++first;
    return first;
}


template<typename InputIter, typename OutputIter>
inline OutputIter copy(InputIter first, InputIter last, OutputIter place)
{
    while (first != last) {
        *place = *first;
        ++first;
        ++place;
    }
    return place;
}


template<typename InputIter, typename OutputIter>
inline OutputIter 
copy_backward(InputIter first, InputIter last, OutputIter place)
{
    while (first != last)
        *--place = *--last;
    return place;
}


template<typename InputIter, typename T>
void fill(InputIter first, InputIter last, const T& v)
{
    while (first != last) {
        *first = v;
        ++first;
    }
}


}  // namespace mySTL

#endif // mySTL_ALGORITHM_HPP
