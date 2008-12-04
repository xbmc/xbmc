/* file.hpp                                
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

/* file.hpp provies File Sources and Sinks
*/


#ifndef TAO_CRYPT_FILE_HPP
#define TAO_CRYPT_FILE_HPP

#include "misc.hpp"
#include "block.hpp"
#include "error.hpp"
#include <stdio.h>

namespace TaoCrypt {


class Source {
    ByteBlock buffer_;
    word32    current_;
    Error     error_;
public:
    explicit Source(word32 sz = 0) : buffer_(sz), current_(0) {}
    Source(const byte* b, word32 sz) : buffer_(b, sz), current_(0) {}

    word32 size() const        { return buffer_.size(); }
    void   grow(word32 sz)     { buffer_.CleanGrow(sz); }
   
    const byte*  get_buffer()  const { return buffer_.get_buffer(); }
    const byte*  get_current() const { return &buffer_[current_]; }
    word32       get_index()   const { return current_; }
    void         set_index(word32 i) { current_ = i; }

    byte operator[] (word32 i) { current_ = i; return next(); }
    byte next() { return buffer_[current_++]; }
    byte prev() { return buffer_[--current_]; }

    void add(const byte* data, word32 len)
    {
        memcpy(buffer_.get_buffer() + current_, data, len);
        current_ += len;
    }

    void advance(word32 i) { current_ += i; }
    void reset(ByteBlock&);

    Error  GetError()              { return error_; }
    void   SetError(ErrorNumber w) { error_.SetError(w); }

    friend class FileSource;  // for get()

    Source(const Source& that)
        : buffer_(that.buffer_), current_(that.current_) {}

    Source& operator=(const Source& that)
    {
        Source tmp(that);
        Swap(tmp);
        return *this;
    }

    void Swap(Source& other) 
    {
        buffer_.Swap(other.buffer_);
        STL::swap(current_, other.current_);
    }

};


// File Source
class FileSource {
    FILE* file_;
public:
    FileSource(const char* fname, Source& source);
    ~FileSource();
   
    word32   size(bool use_current = false);
private:
    word32   get(Source&);
    word32   size_left();                     

    FileSource(const FileSource&);            // hide
    FileSource& operator=(const FileSource&); // hide
};


// File Sink
class FileSink {
    FILE* file_;
public:
    FileSink(const char* fname, Source& source);
    ~FileSink();

    word32 size(bool use_current = false);
private:
    void put(Source&);

    FileSink(const FileSink&);            // hide
    FileSink& operator=(const FileSink&); // hide
};



} // namespace

#endif // TAO_CRYPT_FILE_HPP
