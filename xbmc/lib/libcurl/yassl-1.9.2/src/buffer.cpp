/* buffer.cpp                               
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


/* yaSSL buffer header implements input/output buffers to simulate streaming
 * with SSL types and sockets
 */


#include <string.h>             // memcpy
#include "runtime.hpp"
#include "buffer.hpp"
#include "yassl_types.hpp"

namespace yaSSL {



// Checking Policy should implement a check function that tests whether the
// index is within the size limit of the array

void Check::check(uint i, uint limit) 
{ 
    assert(i < limit);
}


void NoCheck::check(uint, uint) 
{
}


/* input_buffer operates like a smart c style array with a checking option, 
 * meant to be read from through [] with AUTO index or read().
 * Should only write to at/near construction with assign() or raw (e.g., recv)
 * followed by add_size with the number of elements added by raw write.
 *
 * Not using vector because need checked []access, offset, and the ability to
 * write to the buffer bulk wise and have the correct size
 */


input_buffer::input_buffer() 
    : size_(0), current_(0), buffer_(0), end_(0) 
{}


input_buffer::input_buffer(uint s) 
    : size_(0), current_(0), buffer_(NEW_YS byte[s]), end_(buffer_ + s)
{}


// with assign
input_buffer::input_buffer(uint s, const byte* t, uint len) 
    : size_(0), current_(0), buffer_(NEW_YS byte[s]), end_(buffer_ + s) 
{ 
    assign(t, len); 
}


input_buffer::~input_buffer() 
{ 
    ysArrayDelete(buffer_); 
}


// users can pass defualt zero length buffer and then allocate
void input_buffer::allocate(uint s) 
{ 
    assert(!buffer_);       // find realloc error
    buffer_ = NEW_YS byte[s];
    end_ = buffer_ + s; 
}


// for passing to raw writing functions at beginning, then use add_size
byte* input_buffer::get_buffer() const 
{ 
    return buffer_; 
}


// after a raw write user can set NEW_YS size
// if you know the size before the write use assign()
void input_buffer::add_size(uint i) 
{ 
    check(size_ + i-1, get_capacity()); 
    size_ += i; 
}


uint input_buffer::get_capacity()  const 
{ 
    return end_ - buffer_; 
}


uint input_buffer::get_current()   const 
{ 
    return current_; 
}


uint input_buffer::get_size()      const 
{ 
    return size_; 
}


uint input_buffer::get_remaining() const 
{ 
    return size_ - current_; 
}


void input_buffer::set_current(uint i) 
{
    if (i)
        check(i - 1, size_); 
    current_ = i; 
}


// read only access through [], advance current
// user passes in AUTO index for ease of use
const byte& input_buffer::operator[](uint i) 
{
    assert (i == AUTO);
    check(current_, size_);
    return buffer_[current_++];
}


// end of input test
bool input_buffer::eof() 
{ 
    return current_ >= size_; 
}


// peek ahead
byte input_buffer::peek() const
{
    return buffer_[current_];
}


// write function, should use at/near construction
void input_buffer::assign(const byte* t, uint s)
{
    check(current_, get_capacity());
    add_size(s);
    memcpy(&buffer_[current_], t, s);
}


// use read to query input, adjusts current
void input_buffer::read(byte* dst, uint length)
{
    check(current_ + length - 1, size_);
    memcpy(dst, &buffer_[current_], length);
    current_ += length;
}



/* output_buffer operates like a smart c style array with a checking option.
 * Meant to be written to through [] with AUTO index or write().
 * Size (current) counter increases when written to. Can be constructed with 
 * zero length buffer but be sure to allocate before first use. 
 * Don't use add write for a couple bytes, use [] instead, way less overhead.
 * 
 * Not using vector because need checked []access and the ability to
 * write to the buffer bulk wise and retain correct size
 */


output_buffer::output_buffer() 
    : current_(0), buffer_(0), end_(0) 
{}


// with allocate
output_buffer::output_buffer(uint s) 
    : current_(0), buffer_(NEW_YS byte[s]), end_(buffer_ + s) 
{}


// with assign
output_buffer::output_buffer(uint s, const byte* t, uint len) 
    : current_(0), buffer_(NEW_YS byte[s]), end_(buffer_+ s) 
{ 
    write(t, len); 
}


output_buffer::~output_buffer() 
{ 
    ysArrayDelete(buffer_); 
}


uint output_buffer::get_size() const 
{ 
    return current_; 
}


uint output_buffer::get_capacity() const 
{ 
    return end_ - buffer_; 
}


void output_buffer::set_current(uint c) 
{ 
    check(c, get_capacity()); 
    current_ = c; 
}


// users can pass defualt zero length buffer and then allocate
void output_buffer::allocate(uint s) 
{ 
    assert(!buffer_);   // find realloc error
    buffer_ = NEW_YS byte[s]; end_ = buffer_ + s; 
}


// for passing to reading functions when finished
const byte* output_buffer::get_buffer() const 
{ 
    return buffer_; 
}


// allow write access through [], update current
// user passes in AUTO as index for ease of use
byte& output_buffer::operator[](uint i) 
{
    assert(i == AUTO);
    check(current_, get_capacity());
    return buffer_[current_++];
}


// end of output test
bool output_buffer::eof() 
{ 
    return current_ >= get_capacity(); 
}


void output_buffer::write(const byte* t, uint s)
{
    check(current_ + s - 1, get_capacity()); 
    memcpy(&buffer_[current_], t, s);
    current_ += s;
}



} // naemspace

