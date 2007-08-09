#ifndef    __FTVector__
#define    __FTVector__

#include "FTGL.h"

/**
 * Provides a non-STL alternative to the STL vector
 */
template <typename FT_VECTOR_ITEM_TYPE>
class FTGL_EXPORT FTVector
{
    public:
        typedef FT_VECTOR_ITEM_TYPE value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* iterator;
        typedef const value_type* const_iterator;
        typedef size_t size_type;
        
        FTVector()
        {
            Capacity = Size = 0;
            Items = 0;
        }

        
        virtual ~FTVector()
        {
            clear();
        }
        
        FTVector& operator =(const FTVector& v)
        {
            reserve(v.capacity());
            
            iterator ptr = begin();
            const_iterator vbegin = v.begin();
            const_iterator vend = v.end();
            
            while( vbegin != vend)
            {
                *ptr++ = *vbegin++;
            }
            
            Size = v.size();
            return *this;
        }
        
        size_type size() const
        {
            return Size;
        }
        
        size_type capacity() const
        {
            return Capacity;
        }
        
        iterator begin()
        {
            return Items;
        }
        
        const_iterator begin() const
        {
            return Items;
        }
        
        iterator end()
        {
            return begin() + size(); 
        }
        
        const_iterator end() const
        {
            return begin() + size(); 
        }
        
        bool empty() const 
        { 
            return size() == 0; 
        }

        reference operator [](size_type pos) 
        { 
            return( *(begin() + pos)); 
        }

        const_reference operator []( size_type pos) const 
        { 
            return( *(begin() + pos)); 
        }
        
        void clear()
        {
            if( Capacity)
            {
                delete [] Items;
                Capacity = Size = 0;
                Items = 0;
            }
        }

        void reserve( size_type n)
        {
            if( capacity() < n)
            {
                expand(n);
            }
        }

        void push_back(const value_type& x)
        {
            if( size() == capacity())
            {
                expand();
            }
           
           ( *this)[size()] = x;
            ++Size;
        }

        void resize(size_type n, value_type x)
        {
            if( n == size())
            {
                return;
            }
            
            reserve(n);
            iterator begin, end;
            
            if( n >= Size)
            {
                begin = this->end();
                end = this->begin() + n;
            }
            else
            {
                begin = this->begin() + n;
                end = this->end();
            }
        
            while( begin != end)
            {
                *begin++ = x;
            }
        
            Size = n;
        }

        
    private:
        void expand(size_type capacity_hint = 0)
        {
            size_type new_capacity =( capacity() == 0) ? 256 : capacity()* 2;
            if( capacity_hint)
            {
                while( new_capacity < capacity_hint)
                {
                    new_capacity *= 2;
                }
            }
            
            value_type *new_items = new value_type[new_capacity];
            
            iterator begin = this->begin();
            iterator end = this->end();
            value_type *ptr = new_items;
            
            while( begin != end)
            {
                *ptr++ = *begin++;
            }
            
            if( Capacity)
            {
                delete [] Items;
            }
        
            Items = new_items;
            Capacity = new_capacity;
        }

        size_type Capacity;
        size_type Size;
        value_type* Items;
};

#endif  //  __FTVector__
