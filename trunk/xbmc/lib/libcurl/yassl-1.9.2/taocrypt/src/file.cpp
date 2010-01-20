/* file.cpp                                
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

/* file.cpp implements File Sources and Sinks
*/

#include "runtime.hpp"
#include "file.hpp"


namespace TaoCrypt {


FileSource::FileSource(const char* fname, Source& source)
{
    file_ = fopen(fname, "rb");
    if (file_) get(source);
}


FileSource::~FileSource()
{
    if (file_)
        fclose(file_);
}



// return size of source from beginning or current position
word32 FileSource::size(bool use_current)
{
    long current = ftell(file_);
    long begin   = current;

    if (!use_current) {
        fseek(file_, 0, SEEK_SET);
        begin = ftell(file_);
    }

    fseek(file_, 0, SEEK_END);
    long end = ftell(file_);

    fseek(file_, current, SEEK_SET);

    return end - begin;
}


word32 FileSource::size_left()
{
    return size(true);
}


// fill file source from source
word32 FileSource::get(Source& source)
{
    word32 sz(size());
    if (source.size() < sz)
        source.grow(sz);

    size_t bytes = fread(source.buffer_.get_buffer(), 1, sz, file_);

    if (bytes == 1)
        return sz;
    else
        return 0;
}


FileSink::FileSink(const char* fname, Source& source)
{
    file_ = fopen(fname, "wb");
    if (file_) put(source);
}


FileSink::~FileSink()
{
    if (file_)
        fclose(file_);
}


// fill source from file sink
void FileSink::put(Source& source)
{
    fwrite(source.get_buffer(), 1, source.size(), file_);
}


// swap with other and reset to beginning
void Source::reset(ByteBlock& otherBlock)
{
    buffer_.Swap(otherBlock);   
    current_ = 0;
}


}  // namespace
