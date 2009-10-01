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

/// \file internal.h
/// Definitions and structures used only internally by the libmpcdec.

#ifndef _mpcdec_internal_h
#define _mpcdec_internal_h


enum {
    MPC_DECODER_SYNTH_DELAY = 481
};

/// Big/little endian 32 bit byte swapping routine.
static __inline
mpc_uint32_t mpc_swap32(mpc_uint32_t val) {
    return (((val & 0xff000000) >> 24) | ((val & 0x00ff0000) >> 8) |
            ((val & 0x0000ff00) <<  8) | ((val & 0x000000ff) << 24));
}

/// Searches for a ID3v2-tag and reads the length (in bytes) of it.
/// \param reader supplying raw stream data
/// \return size of tag, in bytes
/// \return -1 on errors of any kind
mpc_int32_t JumpID3v2(mpc_reader* fp);

/// helper functions used by multiple files
mpc_uint32_t mpc_random_int(mpc_decoder *d); // in synth_filter.c
void mpc_decoder_initialisiere_quantisierungstabellen(mpc_decoder *d, double scale_factor);
void mpc_decoder_synthese_filter_float(mpc_decoder *d, MPC_SAMPLE_FORMAT* OutData);

#endif // _mpcdec_internal_h

