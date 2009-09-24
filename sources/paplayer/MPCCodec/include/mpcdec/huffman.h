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

/// \file huffman.h
/// Data structures and functions for huffman coding.

#ifndef _mpcdec_huffman_h_
#define _mpcdec_huffman_h_

#ifndef WIN32
#include "mpcdec/config_types.h"
#else
#include "mpcdec/config_win32.h"
#endif

#include "decoder.h"

struct mpc_decoder_t; // forward declare to break circular dependencies

/// Huffman table entry.
typedef struct huffman_type_t {
    mpc_uint32_t  Code;
    mpc_uint32_t  Length;
    mpc_int32_t   Value;
} HuffmanTyp;

//! \brief Sorts huffman-tables by codeword.
//!
//! offset resulting value.
//! \param elements
//! \param Table table to sort
//! \param offset offset of resulting sort
void
mpc_decoder_resort_huff_tables(
    const mpc_uint32_t elements, HuffmanTyp *Table, const mpc_int32_t offset);

/// Initializes sv6 huffman decoding structures.
void mpc_decoder_init_huffman_sv6(struct mpc_decoder_t *d);

/// Initializes sv6 huffman decoding tables.
void mpc_decoder_init_huffman_sv6_tables(struct mpc_decoder_t *d);

/// Initializes sv7 huffman decoding structures.
void mpc_decoder_init_huffman_sv7(struct mpc_decoder_t *d);

/// Initializes sv7 huffman decoding tables.
void mpc_decoder_init_huffman_sv7_tables(struct mpc_decoder_t *d);

#endif // _mpcdec_huffman_h_
