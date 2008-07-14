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

/// \file decoder.h

#ifndef _mpcdec_decoder_h_
#define _mpcdec_decoder_h_

#include "huffman.h"
#include "math.h"
#include "mpcdec.h"
#include "reader.h"
#include "streaminfo.h"

enum {
    MPC_V_MEM = 2304,
    MPC_DECODER_MEMSIZE = 16384,  // overall buffer size
};

typedef struct {
    mpc_int32_t  L [36];
    mpc_int32_t  R [36];
} QuantTyp;

typedef struct mpc_decoder_t {
    mpc_reader *r;

    /// @name internal state variables
    //@{

    mpc_uint32_t  dword; /// actually decoded 32bit-word
    mpc_uint32_t  pos;   /// bit-position within dword
    mpc_uint32_t  Speicher[MPC_DECODER_MEMSIZE]; /// read-buffer
    mpc_uint32_t  Zaehler; /// actual index within read-buffer

    mpc_uint32_t samples_to_skip;

    mpc_uint32_t  FwdJumpInfo;
    mpc_uint32_t  ActDecodePos;
    mpc_uint32_t  FrameWasValid;

    mpc_uint32_t  DecodedFrames;
    mpc_uint32_t  OverallFrames;
    mpc_int32_t   SampleRate;                 // Sample frequency
    mpc_uint32_t  StreamVersion;              // version of bitstream
    mpc_uint32_t  MS_used;                    // MS-coding used ?
    mpc_int32_t   Max_Band;
    mpc_uint32_t  MPCHeaderPos;               // AB: needed to support ID3v2
    mpc_uint32_t  LastValidSamples;
    mpc_uint32_t  TrueGaplessPresent;

    mpc_uint32_t  EQ_activated;

    mpc_uint32_t  WordsRead;                  // counts amount of decoded dwords
#ifdef USE_SEEK_TABLE
    mpc_uint16_t* SeekTable;
#endif
    // randomizer state variables
    mpc_uint32_t  __r1; 
    mpc_uint32_t  __r2; 

    mpc_uint32_t  Q_bit [32];     
    mpc_uint32_t  Q_res [32][16];

    // huffman table stuff
    HuffmanTyp    HuffHdr  [10];
    HuffmanTyp    HuffSCFI [ 4];
    HuffmanTyp    HuffDSCF [16];
    HuffmanTyp*   HuffQ [2] [8];

    HuffmanTyp    HuffQ1 [2] [3*3*3];
    HuffmanTyp    HuffQ2 [2] [5*5];
    HuffmanTyp    HuffQ3 [2] [ 7];
    HuffmanTyp    HuffQ4 [2] [ 9];
    HuffmanTyp    HuffQ5 [2] [15];
    HuffmanTyp    HuffQ6 [2] [31];
    HuffmanTyp    HuffQ7 [2] [63];
    const HuffmanTyp* SampleHuff [18];
    HuffmanTyp    SCFI_Bundle   [ 8];
    HuffmanTyp    DSCF_Entropie [13];
    HuffmanTyp    Region_A [16];
    HuffmanTyp    Region_B [ 8];
    HuffmanTyp    Region_C [ 4];

    HuffmanTyp    Entropie_1 [ 3];
    HuffmanTyp    Entropie_2 [ 5];
    HuffmanTyp    Entropie_3 [ 7];
    HuffmanTyp    Entropie_4 [ 9];
    HuffmanTyp    Entropie_5 [15];
    HuffmanTyp    Entropie_6 [31];
    HuffmanTyp    Entropie_7 [63];

    mpc_int32_t   SCF_Index_L [32] [3];
    mpc_int32_t   SCF_Index_R [32] [3];       // holds scalefactor-indices
    QuantTyp      Q [32];                     // holds quantized samples
    mpc_int32_t   Res_L [32];
    mpc_int32_t   Res_R [32];                 // holds the chosen quantizer for each subband
    mpc_int32_t   DSCF_Flag_L [32];
    mpc_int32_t   DSCF_Flag_R [32];           // differential SCF used?
    mpc_int32_t   SCFI_L [32];
    mpc_int32_t   SCFI_R [32];                // describes order of transmitted SCF
    mpc_int32_t   DSCF_Reference_L [32];
    mpc_int32_t   DSCF_Reference_R [32];      // holds last frames SCF
    mpc_int32_t   MS_Flag[32];                // MS used?
#ifdef MPC_FIXED_POINT
    unsigned char SCF_shift[256];
#endif

    MPC_SAMPLE_FORMAT V_L[MPC_V_MEM + 960];
    MPC_SAMPLE_FORMAT V_R[MPC_V_MEM + 960];
    MPC_SAMPLE_FORMAT Y_L[36][32];
    MPC_SAMPLE_FORMAT Y_R[36][32];
    MPC_SAMPLE_FORMAT SCF[256]; ///< holds adapted scalefactors (for clipping prevention)
    //@}

} mpc_decoder;

#endif // _mpc_decoder_h
