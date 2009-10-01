/* buffer.hpp                                
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


/* yaSSL buffer header defines input and output buffers to simulate streaming
 * with SSL types and sockets
 */

#ifndef yaSSL_BUFFER_HPP
#define yaSSL_BUFFER_HPP

#include <assert.h>             // assert
#include "yassl_types.hpp"      // ysDelete
#include "memory.hpp"           // mySTL::auto_ptr
#include STL_ALGORITHM_FILE


namespace STL = STL_NAMESPACE;


#ifdef _MSC_VER
    // disable truncated debug symbols
    #pragma warning(disable:4786)
#endif


namespace yaSSL {

typedef unsigned char byte;
typedef unsigned int  uint;
const uint AUTO = 0xFEEDBEEF;


// Checking Policy should implement a check function that tests whether the
// index is within the size limit of the array
struct Check {
    void check(uint i, uint limit);
};


struct NoCheck {
    void check(uint, uint);
};

/* input_buffer operates like a smart c style array with a checking option, 
 * meant to be read from through [] with AUTO index or read().
 * Should only write to at/near construction with assign() or raw (e.g., recv)
 * followed by add_size with the number of elements added by raw write.
 *
 * Not using vector because need checked []access, offset, and the ability to
 * write to the buffer bulk wise and have the correct size
 */

class input_buffer : public Check {
    uint   size_;                // number of elements in buffer
    uint   current_;             // current offset position in buffer
    byte*  buffer_;              // storage for buffer
    byte*  end_;                 // end of storage marker
public:
    input_buffer();

    explicit input_buffer(uint s);
                          
    // with assign
    input_buffer(uint s, const byte* t, uint len);
    
    ~input_buffer();

    // users can pass defualt zero length buffer and then allocate
    void allocate(uint s);

    // for passing to raw writing functions at beginning, then use add_size
    byte* get_buffer() const;

    // after a raw write user can set new size
    // if you know the size before the write use assign()
    void add_size(uint i);

    uint get_capacity()  const;

    uint get_current()   const;

    uint get_size()      const;

    uint get_remaining() const;

    void set_current(uint i);

    // read only access through [], advance current
    // user passes in AUTO index for ease of use
    const byte& operator[](uint i);
    
    // end of input test
    bool eof();

    // peek ahead
    byte peek() const;

    // write function, should use at/near construction
    void assign(const byte* t, uint s);
    
    // use read to query input, adjusts current
    void read(byte* dst, uint length);

private:
    input_buffer(const input_buffer&);              // hide copy
    input_buffer& operator=(const input_buffer&);   // and assign
};


/* output_buffer operates like a smart c style array with a checking option.
 * Meant to be written to through [] with AUTO index or write().
 * Size (current) counter increases when written to. Can be constructed with 
 * zero length buffer but be sure to allocate before first use. 
 * Don't use add write for a couple bytes, use [] instead, way less overhead.
 * 
 * Not using vector because need checked []access and the ability to
 * write to the buffer bulk wise and retain correct size
 */
class output_buffer : public Check {
    uint    current_;                // current offset and elements in buffer
    byte*   buffer_;                 // storage for buffer
    byte*   end_;                    // end of storage marker
public:
    // default
    output_buffer();

    // with allocate
    explicit output_buffer(uint s);

    // with assign
    output_buffer(uint s, const byte* t, uint len);

    ~output_buffer();

    uint get_size() const;

    uint get_capacity() const;

    void set_current(uint c);

    // users can pass defualt zero length buffer and then allocate
    void allocate(uint s);

    // for passing to reading functions when finished
    const byte* get_buffer() const;

    // allow write access through [], update current
    // user passes in AUTO as index for ease of use
    byte& operator[](uint i);
    
    // end of output test
    bool eof();

    void write(const byte* t, uint s);

private:
    output_buffer(const output_buffer&);              // hide copy
    output_buffer& operator=(const output_buffer&);   // and assign
};




// turn delete an incomplete type into comipler error instead of warning
template <typename T>
inline void checked_delete(T* p)
{
    typedef char complete_type[sizeof(T) ? 1 : -1];
    (void)sizeof(complete_type);
    ysDelete(p);
}


// checked delete functor increases effeciency, no indirection on function call
// sets pointer to zero so safe for std conatiners
struct del_ptr_zero
{
    template <typename T>
    void operator()(T*& p) const
    {
        T* tmp = 0;
        STL::swap(tmp, p);
        checked_delete(tmp); 
    }
};



} // naemspace

#endif // yaSSL_BUUFER_HPP
