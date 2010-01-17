/* mySTL helpers.hpp                                
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


/* mySTL helpers implements misc constructs for vector and list
 *
 */

#ifndef mySTL_HELPERS_HPP
#define mySTL_HELPERS_HPP

#include <stdlib.h>
#ifdef _MSC_VER
    #include <new>
#endif

/*
      Workaround for the lack of operator new(size_t, void*)
      in IBM VA C++ 6.0
      Also used as a workaround to avoid including <new>
*/
    struct Dummy {};

    inline void* operator new(size_t size, Dummy* d) 
    { 
        return static_cast<void*>(d);
    }

    // for compilers that want matching delete
    inline void operator delete(void* ptr, Dummy* d) 
    { 
    }

    typedef Dummy* yassl_pointer;

namespace mySTL {


template <typename T, typename T2>
inline void construct(T* p, const T2& value)
{
    new (reinterpret_cast<yassl_pointer>(p)) T(value);
}


template <typename T>
inline void construct(T* p)
{
    new (reinterpret_cast<yassl_pointer>(p)) T();
}


template <typename T>
inline void destroy(T* p)
{
    p->~T();
}


template <typename Iter>
void destroy(Iter first, Iter last)
{
    while (first != last) {
        destroy(&*first);
        ++first;
    }
}


template <typename Iter, typename PlaceIter>
PlaceIter uninit_copy(Iter first, Iter last, PlaceIter place)
{
    while (first != last) {
        construct(&*place, *first);
        ++first;
        ++place;
    }
    return place;
}


template <typename PlaceIter, typename Size, typename T>
PlaceIter uninit_fill_n(PlaceIter place, Size n, const T& value)
{
    while (n) {
        construct(&*place, value);
        --n;
        ++place;
    }
    return place;
}


template <typename T>
T* GetArrayMemory(size_t items)
{
    unsigned char* ret;

    #ifdef YASSL_LIB
        ret = NEW_YS unsigned char[sizeof(T) * items];
    #else
        ret = NEW_TC unsigned char[sizeof(T) * items];
    #endif

    return reinterpret_cast<T*>(ret);
}


template <typename T>
void FreeArrayMemory(T* ptr)
{
    unsigned char* p = reinterpret_cast<unsigned char*>(ptr);

    #ifdef YASSL_LIB
        yaSSL::ysArrayDelete(p);
    #else
        TaoCrypt::tcArrayDelete(p);
    #endif
}



inline void* GetMemory(size_t bytes)
{
    return GetArrayMemory<unsigned char>(bytes);
}


inline void FreeMemory(void* ptr)
{
    FreeArrayMemory(ptr);
}



} // namespace mySTL

#endif // mySTL_HELPERS_HPP
