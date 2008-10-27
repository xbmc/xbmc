/* mySTL vector.hpp                                
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


/* mySTL vector implements simple vector, w/ swap
 *
 */

#ifndef mySTL_VECTOR_HPP
#define mySTL_VECTOR_HPP

#include "helpers.hpp"    // construct, destory, fill, etc.
#include "algorithm.hpp"  // swap
#include <assert.h>       // assert


namespace mySTL {


template <typename T>
struct vector_base {
    T* start_;
    T* finish_;
    T* end_of_storage_;

    vector_base() : start_(0), finish_(0), end_of_storage_(0) {}
    vector_base(size_t n)
    {
        start_ = GetArrayMemory<T>(n);
        finish_ = start_;
        end_of_storage_ = start_ + n;
    }

    ~vector_base() 
    { 
        FreeArrayMemory(start_);
    }

    void Swap(vector_base& that) 
    {
        swap(start_, that.start_);
        swap(finish_, that.finish_);
        swap(end_of_storage_, that.end_of_storage_);
    }
};



template <typename T>
class vector {
public:
    typedef T*       iterator;
    typedef const T* const_iterator;

    vector() {}
    explicit vector(size_t n) : vec_(n) 
    { 
        vec_.finish_ = uninit_fill_n(vec_.start_, n, T()); 
    }

    ~vector() { destroy(vec_.start_, vec_.finish_); }

    vector(const vector& other) : vec_(other.size())
    {
        vec_.finish_ = uninit_copy(other.vec_.start_, other.vec_.finish_,
                                   vec_.start_);   
    }

    size_t capacity() const { return vec_.end_of_storage_ - vec_.start_; }

    size_t size() const { return vec_.finish_ - vec_.start_; }

    T&       operator[](size_t idx)       { return *(vec_.start_ + idx); }
    const T& operator[](size_t idx) const { return *(vec_.start_ + idx); }

    const T* begin() const { return vec_.start_; }
    const T* end()   const { return vec_.finish_; }

    void push_back(const T& v)
    {
        if (vec_.finish_ != vec_.end_of_storage_) {
            construct(vec_.finish_, v);
            ++vec_.finish_;
        }
        else {
            vector tmp(size() * 2 + 1, *this);
            construct(tmp.vec_.finish_, v);
            ++tmp.vec_.finish_;
            Swap(tmp);
        }  
    }

    void resize(size_t n, const T& v)
    {
        if (n == size()) return;

        if (n < size()) {
            T* first = vec_.start_ + n;
            destroy(first, vec_.finish_);
            vec_.finish_ -= vec_.finish_ - first;
        }
        else {
            vector tmp(n, *this);
            tmp.vec_.finish_ = uninit_fill_n(tmp.vec_.finish_, n - size(), v);
            Swap(tmp);
        }
    }

    void reserve(size_t n)
    {
        if (capacity() < n) {
            vector tmp(n, *this);
            Swap(tmp);
        }
    }

    void Swap(vector& that)
    {
        vec_.Swap(that.vec_);
    }
private:
    vector_base<T> vec_;

    vector& operator=(const vector&);   // hide assign

    // for growing, n must be bigger than other size
    vector(size_t n, const vector& other) : vec_(n)
    {
        assert(n > other.size());
        vec_.finish_ = uninit_copy(other.vec_.start_, other.vec_.finish_,
                                   vec_.start_);   
    }
};



} // namespace mySTL

#endif // mySTL_VECTOR_HPP
