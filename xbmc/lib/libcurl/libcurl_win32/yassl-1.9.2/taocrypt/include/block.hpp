/* block.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
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


/* block.hpp provides word and byte blocks with configurable allocators
*/


#ifndef TAO_CRYPT_BLOCK_HPP
#define TAO_CRYPT_BLOCK_HPP

#include "misc.hpp"
#include <string.h>         // memcpy
#include <stddef.h>         // ptrdiff_t

#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif


namespace STL = STL_NAMESPACE;


namespace TaoCrypt {


// a Base class for Allocators
template<class T>
class AllocatorBase
{
public:
    typedef T         value_type;
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef T*        pointer;
    typedef const T*  const_pointer;
    typedef T&        reference;
    typedef const T&  const_reference;

    pointer       address(reference r) const {return (&r);}
    const_pointer address(const_reference r) const {return (&r); }
    void          construct(pointer p, const T& val) {new (p) T(val);}
    void          destroy(pointer p) {p->~T();}
    size_type     max_size() const {return ~size_type(0)/sizeof(T);}
protected:
    static void CheckSize(size_t n)
    {
        assert(n <= ~size_t(0) / sizeof(T));
    }
};


// General purpose realloc
template<typename T, class A>
typename A::pointer StdReallocate(A& a, T* p, typename A::size_type oldSize,
                                  typename A::size_type newSize, bool preserve)
{
    if (oldSize == newSize)
        return p;

    if (preserve) {
        A b = A();
        typename A::pointer newPointer = b.allocate(newSize, 0);
        memcpy(newPointer, p, sizeof(T) * min(oldSize, newSize));
        a.deallocate(p, oldSize);
        STL::swap(a, b);
        return newPointer;
    }
    else {
        a.deallocate(p, oldSize);
        return a.allocate(newSize, 0);
    }
}


// Allocator that zeros out memory on deletion
template <class T>
class AllocatorWithCleanup : public AllocatorBase<T>
{
public:
    typedef typename AllocatorBase<T>::pointer   pointer;
    typedef typename AllocatorBase<T>::size_type size_type;

    pointer allocate(size_type n, const void* = 0)
    {
        this->CheckSize(n);
        if (n == 0)
            return 0;
        return NEW_TC T[n];
    }

    void deallocate(void* p, size_type n)
    {
        memset(p, 0, n * sizeof(T));
        tcArrayDelete((T*)p);
    }

    pointer reallocate(T* p, size_type oldSize, size_type newSize,
                       bool preserve)
    {
        return StdReallocate(*this, p, oldSize, newSize, preserve);
    }

    // VS.NET STL enforces the policy of "All STL-compliant allocators have to
    // provide a template class member called rebind".
    template <class U> struct rebind { typedef AllocatorWithCleanup<U> other;};
};


// Block class template
template<typename T, class A = AllocatorWithCleanup<T> >
class Block {
public:
    explicit Block(word32 s = 0) : sz_(s), buffer_(allocator_.allocate(sz_)) 
                    { CleanNew(sz_); }

    Block(const T* buff, word32 s) : sz_(s), buffer_(allocator_.allocate(sz_))
        { memcpy(buffer_, buff, sz_ * sizeof(T)); }

    Block(const Block& that) : sz_(that.sz_), buffer_(allocator_.allocate(sz_))
        { memcpy(buffer_, that.buffer_, sz_ * sizeof(T)); }

    Block& operator=(const Block& that) {
        Block tmp(that);
        Swap(tmp);
        return *this;
    }

    T& operator[] (word32 i) { assert(i < sz_); return buffer_[i]; }
    const T& operator[] (word32 i) const 
        { assert(i < sz_); return buffer_[i]; }

    T* operator+ (word32 i) { return buffer_ + i; }
    const T* operator+ (word32 i) const { return buffer_ + i; }

    word32 size() const { return sz_; }

    T* get_buffer() const { return buffer_; }
    T* begin()      const { return get_buffer(); }

    void CleanGrow(word32 newSize)
    {
        if (newSize > sz_) {
            buffer_ = allocator_.reallocate(buffer_, sz_, newSize, true);
            memset(buffer_ + sz_, 0, (newSize - sz_) * sizeof(T));
            sz_ = newSize;
        }
    }

    void CleanNew(word32 newSize)
    {
        New(newSize);
        memset(buffer_, 0, sz_ * sizeof(T));
    }

    void New(word32 newSize)
    {
        buffer_ = allocator_.reallocate(buffer_, sz_, newSize, false);
        sz_ = newSize;
    }

    void resize(word32 newSize)
    {
        buffer_ = allocator_.reallocate(buffer_, sz_, newSize, true);
        sz_ = newSize;
    }

    void Swap(Block& other) {
        STL::swap(sz_, other.sz_);
        STL::swap(buffer_, other.buffer_);
        STL::swap(allocator_, other.allocator_);
    }

    ~Block() { allocator_.deallocate(buffer_, sz_); }
private:
    word32 sz_;     // size in Ts
    T*     buffer_;
    A      allocator_;
};


typedef Block<byte>   ByteBlock;
typedef Block<word>   WordBlock;
typedef Block<word32> Word32Block;


} // namespace

#endif // TAO_CRYPT_BLOCK_HPP
