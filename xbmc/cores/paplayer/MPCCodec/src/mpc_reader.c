/*
  Copyright (c) 2005, The Musepack Development Team
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.

  * Neither the name of the The Musepack Development Team nor the
  names of its contributors may be used to endorse or promote
  products derived from this software without specific prior
  written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/// \file mpc_reader.c
/// Contains implementations for simple file-based mpc_reader

#include <mpcdec/mpcdec.h>

/// mpc_reader callback implementations
static mpc_int32_t
read_impl(void *data, void *ptr, mpc_int32_t size)
{
    mpc_reader_file *d = (mpc_reader_file *) data;

    return fread(ptr, 1, size, d->file);
}

static mpc_bool_t
seek_impl(void *data, mpc_int32_t offset)
{
    mpc_reader_file *d = (mpc_reader_file *) data;

    return d->is_seekable ? !fseek(d->file, offset, SEEK_SET) : FALSE;
}

static mpc_int32_t
tell_impl(void *data)
{
    mpc_reader_file *d = (mpc_reader_file *) data;

    return ftell(d->file);
}

static mpc_int32_t
get_size_impl(void *data)
{
    mpc_reader_file *d = (mpc_reader_file *) data;

    return d->file_size;
}

static mpc_bool_t
canseek_impl(void *data)
{
    mpc_reader_file *d = (mpc_reader_file *) data;

    return d->is_seekable;
}

void
mpc_reader_setup_file_reader(mpc_reader_file *p_reader, FILE *input)
{
    p_reader->reader.seek = seek_impl;
    p_reader->reader.read = read_impl;
    p_reader->reader.tell = tell_impl;
    p_reader->reader.get_size = get_size_impl;
    p_reader->reader.canseek = canseek_impl;
    p_reader->reader.data = p_reader; // point back to ourselves

    p_reader->file = input;
    p_reader->is_seekable = TRUE;
    fseek(input, 0, SEEK_END);
    p_reader->file_size = ftell(input);
    fseek(input, 0, SEEK_SET);
}
