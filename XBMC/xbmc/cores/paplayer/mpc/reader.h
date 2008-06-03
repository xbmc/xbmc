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

/// \file reader.h

#ifndef _mpcdec_reader_h_
#define _mpcdec_reader_h_

/// \brief Stream reader interface structure.
///
/// This is the structure you must supply to the musepack decoding library
/// to feed it with raw data.  Implement the five member functions to provide
/// a functional reader.
typedef struct mpc_reader_t {
    /// Reads size bytes of data into buffer at ptr.
	mpc_int32_t (*read)(void *t, void *ptr, mpc_int32_t size);

    /// Seeks to byte position offset.
	mpc_bool_t (*seek)(void *t, mpc_int32_t offset);

    /// Returns the current byte offset in the stream.
	mpc_int32_t (*tell)(void *t);

    /// Returns the total length of the source stream, in bytes.
	mpc_int32_t (*get_size)(void *t);

    /// True if the stream is a seekable stream.
	mpc_bool_t (*canseek)(void *t);

    /// Field that can be used to identify a particular instance of
    /// reader or carry along data associated with that reader.
    void *data;

} mpc_reader;

typedef struct mpc_reader_file_t {
	mpc_reader reader;

    FILE *file;
    long file_size;
    mpc_bool_t is_seekable;
} mpc_reader_file;

/// Initializes reader with default stdio file reader implementation.  Use
/// this if you're just reading from a plain file.
///
/// \param r reader struct to initalize
/// \param input input stream to attach to the reader
void mpc_reader_setup_file_reader(mpc_reader_file *r, FILE *input);

#endif // _mpcdec_reader_h_
