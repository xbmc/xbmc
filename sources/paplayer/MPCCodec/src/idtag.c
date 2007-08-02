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

/// \file idtag.c
/// Rudimentary id3tag handling routines, just enough to skip id3v2 tags,
/// if present.

#include <mpcdec/mpcdec.h>
#include <mpcdec/internal.h>

mpc_int32_t
JumpID3v2 (mpc_reader* r) {
    unsigned char  tmp [10];
    mpc_uint32_t   Unsynchronisation;   // ID3v2.4-flag
    mpc_uint32_t   ExtHeaderPresent;    // ID3v2.4-flag
    mpc_uint32_t   ExperimentalFlag;    // ID3v2.4-flag
    mpc_uint32_t   FooterPresent;       // ID3v2.4-flag
    mpc_int32_t    ret;

    // seek to first byte of mpc data
    if (!r->seek (r->data, 0)) {
        return 0;  
    }
    
    r->read(r->data, tmp, sizeof(tmp));

    // check id3-tag
    if ( 0 != memcmp ( tmp, "ID3", 3) )
        return 0;

    // read flags
    Unsynchronisation = tmp[5] & 0x80;
    ExtHeaderPresent  = tmp[5] & 0x40;
    ExperimentalFlag  = tmp[5] & 0x20;
    FooterPresent     = tmp[5] & 0x10;

    if ( tmp[5] & 0x0F )
        return -1;              // not (yet???) allowed
    if ( (tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80 )
        return -1;              // not allowed

    // read HeaderSize (syncsave: 4 * $0xxxxxxx = 28 significant bits)
    ret  = tmp[6] << 21;
    ret += tmp[7] << 14;
    ret += tmp[8] <<  7;
    ret += tmp[9]      ;
    ret += 10;
    if ( FooterPresent )
        ret += 10;

    return ret;
}
