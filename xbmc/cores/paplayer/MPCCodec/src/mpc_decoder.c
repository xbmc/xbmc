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

/// \file mpc_decoder.c
/// Core decoding routines and logic.

#include <mpcdec/mpcdec.h>
#include <mpcdec/internal.h>
#include <mpcdec/requant.h>
#include <mpcdec/huffman.h>

//------------------------------------------------------------------------------
// types
//------------------------------------------------------------------------------
enum
    {
        EQ_TAP = 13,                        // length of FIR filter for EQ
        DELAY = ((EQ_TAP + 1) / 2),         // delay of FIR
        FIR_BANDS = 4,                      // number of subbands to be FIR filtered
        MEMSIZE = MPC_DECODER_MEMSIZE,      // overall buffer size
        MEMSIZE2 = (MEMSIZE/2),             // size of one buffer
        MEMMASK = (MEMSIZE-1)
    };

//------------------------------------------------------------------------------
// forward declarations
//------------------------------------------------------------------------------
void mpc_decoder_init_huffman_sv6(mpc_decoder *d);
void mpc_decoder_init_huffman_sv7(mpc_decoder *d);
void mpc_decoder_read_bitstream_sv6(mpc_decoder *d);
void mpc_decoder_read_bitstream_sv7(mpc_decoder *d);
void mpc_decoder_update_buffer(mpc_decoder *d, mpc_uint32_t RING);
mpc_bool_t mpc_decoder_seek_sample(mpc_decoder *d, mpc_int64_t destsample);
void mpc_decoder_requantisierung(mpc_decoder *d, const mpc_int32_t Last_Band);

//------------------------------------------------------------------------------
// utility functions
//------------------------------------------------------------------------------
static mpc_int32_t f_read(mpc_decoder *d, void *ptr, size_t size) 
{ 
    return d->r->read(d->r->data, ptr, size); 
};

static mpc_bool_t f_seek(mpc_decoder *d, mpc_int32_t offset) 
{ 
    return d->r->seek(d->r->data, offset); 
};

static mpc_int32_t f_read_dword(mpc_decoder *d, mpc_uint32_t * ptr, mpc_uint32_t count) 
{
    count = f_read(d, ptr, count << 2) >> 2;
#ifndef MPC_LITTLE_ENDIAN
    mpc_uint32_t n;
    for(n = 0; n< count; n++) {
        ptr[n] = mpc_swap32(ptr[n]);
    }
#endif
    return count;
}

//------------------------------------------------------------------------------
// huffman & bitstream functions
//------------------------------------------------------------------------------
static const mpc_uint32_t mask [33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
    0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
    0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
    0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
    0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
    0xFFFFFFFF
};

/* F U N C T I O N S */

// resets bitstream decoding
static void
mpc_decoder_reset_bitstream_decode(mpc_decoder *d) 
{
    d->dword = 0;
    d->pos = 0;
    d->Zaehler = 0;
    d->WordsRead = 0;
}

// reports the number of read bits
static mpc_uint32_t
mpc_decoder_bits_read(mpc_decoder *d) 
{
    return 32 * d->WordsRead + d->pos;
}

// read desired number of bits out of the bitstream
static mpc_uint32_t
mpc_decoder_bitstream_read(mpc_decoder *d, const mpc_uint32_t bits) 
{
    mpc_uint32_t out = d->dword;

    d->pos += bits;

    if (d->pos < 32) {
        out >>= (32 - d->pos);
    }
    else {
        d->dword = d->Speicher[d->Zaehler = (d->Zaehler + 1) & MEMMASK];
        d->pos -= 32;
        if (d->pos) {
            out <<= d->pos;
            out |= d->dword >> (32 - d->pos);
        }
        ++(d->WordsRead);
    }

    return out & mask[bits];
}

// decode SCFI-bundle (sv4,5,6)
static void
mpc_decoder_scfi_bundle_read(
    mpc_decoder *d,
    HuffmanTyp* Table, mpc_int32_t* SCFI, mpc_int32_t* DSCF) 
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;
    if (d->pos > 26) {
        code |= d->Speicher[(d->Zaehler + 1) & MEMMASK] >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        d->pos -= 32;
        d->dword = d->Speicher[d->Zaehler = (d->Zaehler+1) & MEMMASK];
        ++(d->WordsRead);
    }

    *SCFI = Table->Value >> 1;
    *DSCF = Table->Value &  1;
}

static int
mpc_decoder_huffman_typ_cmpfn(const void* p1, const void* p2)
{
    if (((HuffmanTyp*) p1)->Code < ((HuffmanTyp*) p2)->Code ) return +1;
    if (((HuffmanTyp*) p1)->Code > ((HuffmanTyp*) p2)->Code ) return -1;
    return 0;
}

// sort huffman-tables by codeword
// offset resulting value
void
mpc_decoder_resort_huff_tables(
    const mpc_uint32_t elements, HuffmanTyp* Table, const mpc_int32_t offset ) 
{
    mpc_uint32_t  i;

    for ( i = 0; i < elements; i++ ) {
        Table[i].Code <<= 32 - Table[i].Length;
        Table[i].Value  =  i - offset;
    }
    qsort(Table, elements, sizeof(*Table), mpc_decoder_huffman_typ_cmpfn);
}

// basic huffman decoding routine
// works with maximum lengths up to 14
static mpc_int32_t
mpc_decoder_huffman_decode(mpc_decoder *d, const HuffmanTyp *Table) 
{
    // load preview and decode
    mpc_uint32_t code = d->dword << d->pos;
    if (d->pos > 18) {
        code |= d->Speicher[(d->Zaehler + 1) & MEMMASK] >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        d->pos -= 32;
        d->dword = d->Speicher[d->Zaehler = (d->Zaehler + 1) & MEMMASK];
        ++(d->WordsRead);
    }

    return Table->Value;
}

// faster huffman through previewing less bits
// works with maximum lengths up to 10
static mpc_int32_t
mpc_decoder_huffman_decode_fast(mpc_decoder *d, const HuffmanTyp* Table)
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;
    if (d->pos > 22) {
        code |= d->Speicher[(d->Zaehler + 1) & MEMMASK] >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        d->pos -= 32;
        d->dword = d->Speicher[d->Zaehler = (d->Zaehler + 1) & MEMMASK];
        ++(d->WordsRead);
    }

    return Table->Value;
}

// even faster huffman through previewing even less bits
// works with maximum lengths up to 5
static mpc_int32_t
mpc_decoder_huffman_decode_faster(mpc_decoder *d, const HuffmanTyp* Table)
{
    // load preview and decode
    mpc_uint32_t code  = d->dword << d->pos;
    if (d->pos > 27) {
        code |= d->Speicher[(d->Zaehler + 1) & MEMMASK] >> (32 - d->pos);
    }
    while (code < Table->Code) {
        Table++;
    }

    // set the new position within bitstream without performing a dummy-read
    if ((d->pos += Table->Length) >= 32) {
        d->pos -= 32;
        d->dword = d->Speicher[d->Zaehler = (d->Zaehler + 1) & MEMMASK];
        ++(d->WordsRead);
    }

    return Table->Value;
}

static void
mpc_decoder_reset_v(mpc_decoder *d) 
{
    memset(d->V_L, 0, sizeof d->V_L);
    memset(d->V_R, 0, sizeof d->V_R);
}

static void
mpc_decoder_reset_synthesis(mpc_decoder *d) 
{
    mpc_decoder_reset_v(d);
}

static void
mpc_decoder_reset_y(mpc_decoder *d) 
{
    memset(d->Y_L, 0, sizeof d->Y_L);
    memset(d->Y_R, 0, sizeof d->Y_R);
}

static void
mpc_decoder_reset_globals(mpc_decoder *d) 
{
    mpc_decoder_reset_bitstream_decode(d);

    d->DecodedFrames  = 0;
    d->StreamVersion  = 0;
    d->MS_used        = 0;

    memset(d->Y_L          , 0, sizeof d->Y_L           );
    memset(d->Y_R          , 0, sizeof d->Y_R           );
    memset(d->SCF_Index_L     , 0, sizeof d->SCF_Index_L      );
    memset(d->SCF_Index_R     , 0, sizeof d->SCF_Index_R      );
    memset(d->Res_L           , 0, sizeof d->Res_L            );
    memset(d->Res_R           , 0, sizeof d->Res_R            );
    memset(d->SCFI_L          , 0, sizeof d->SCFI_L           );
    memset(d->SCFI_R          , 0, sizeof d->SCFI_R           );
    memset(d->DSCF_Flag_L     , 0, sizeof d->DSCF_Flag_L      );
    memset(d->DSCF_Flag_R     , 0, sizeof d->DSCF_Flag_R      );
    memset(d->DSCF_Reference_L, 0, sizeof d->DSCF_Reference_L );
    memset(d->DSCF_Reference_R, 0, sizeof d->DSCF_Reference_R );
    memset(d->Q               , 0, sizeof d->Q                );
    memset(d->MS_Flag         , 0, sizeof d->MS_Flag          );
}

mpc_uint32_t
mpc_decoder_decode_frame(mpc_decoder *d, mpc_uint32_t *in_buffer,
                         mpc_uint32_t in_len, MPC_SAMPLE_FORMAT *out_buffer)
{
  unsigned int i;
  mpc_decoder_reset_bitstream_decode(d);
  if (in_len > sizeof(d->Speicher)) in_len = sizeof(d->Speicher);
  memcpy(d->Speicher, in_buffer, in_len);
#ifdef MPC_LITTLE_ENDIAN
  for (i = 0; i < (in_len + 3) / 4; i++)
    d->Speicher[i] = mpc_swap32(d->Speicher[i]);
#endif
  d->dword = d->Speicher[0];
  switch (d->StreamVersion) {
    case 0x04:
    case 0x05:
    case 0x06:
        mpc_decoder_read_bitstream_sv6(d);
        break;
    case 0x07:
    case 0x17:
        mpc_decoder_read_bitstream_sv7(d);
        break;
    default:
        return (mpc_uint32_t)(-1);
  }
  mpc_decoder_requantisierung(d, d->Max_Band);
  mpc_decoder_synthese_filter_float(d, out_buffer);
  return mpc_decoder_bits_read(d);
}

static mpc_uint32_t
mpc_decoder_decode_internal(mpc_decoder *d, MPC_SAMPLE_FORMAT *buffer) 
{
    mpc_uint32_t output_frame_length = MPC_FRAME_LENGTH;

    mpc_uint32_t  FrameBitCnt = 0;

    if (d->DecodedFrames >= d->OverallFrames) {
        return (mpc_uint32_t)(-1);                           // end of file -> abort decoding
    }

    // read jump-info for validity check of frame
    d->FwdJumpInfo  = mpc_decoder_bitstream_read(d, 20);
#ifdef USE_SEEK_TABLE
	  d->SeekTable [d->DecodedFrames] = 20 + d->FwdJumpInfo;
#endif

    d->ActDecodePos = (d->Zaehler << 5) + d->pos;

    // decode data and check for validity of frame
    FrameBitCnt = mpc_decoder_bits_read(d);
    switch (d->StreamVersion) {
    case 0x04:
    case 0x05:
    case 0x06:
        mpc_decoder_read_bitstream_sv6(d);
        break;
    case 0x07:
    case 0x17:
        mpc_decoder_read_bitstream_sv7(d);
        break;
    default:
        return (mpc_uint32_t)(-1);
    }
    d->FrameWasValid = mpc_decoder_bits_read(d) - FrameBitCnt == d->FwdJumpInfo;

    // synthesize signal
    mpc_decoder_requantisierung(d, d->Max_Band);

    //if ( d->EQ_activated && PluginSettings.EQbyMPC )
    //    perform_EQ ();

    mpc_decoder_synthese_filter_float(d, buffer);

    d->DecodedFrames++;

    // cut off first MPC_DECODER_SYNTH_DELAY zero-samples
    if (d->DecodedFrames == d->OverallFrames  && d->StreamVersion >= 6) {        
        // reconstruct exact filelength
        mpc_int32_t  mod_block   = mpc_decoder_bitstream_read(d,  11);
        mpc_int32_t  FilterDecay;

        if (mod_block == 0) {
            // Encoder bugfix
            mod_block = 1152;                    
        }
        FilterDecay = (mod_block + MPC_DECODER_SYNTH_DELAY) % MPC_FRAME_LENGTH;

        // additional FilterDecay samples are needed for decay of synthesis filter
        if (MPC_DECODER_SYNTH_DELAY + mod_block >= MPC_FRAME_LENGTH) {

            // **********************************************************************
            // Rhoades 4/16/2002
            // Commented out are blocks of code which cause gapless playback to fail.
            // Temporary fix...
            // **********************************************************************

            if (!d->TrueGaplessPresent) {
                mpc_decoder_reset_y(d);
            } 
            else {
                //if ( MPC_FRAME_LENGTH != d->LastValidSamples ) {
                mpc_decoder_bitstream_read(d, 20);
                mpc_decoder_read_bitstream_sv7(d);
                mpc_decoder_requantisierung(d, d->Max_Band);
                //FilterDecay = d->LastValidSamples;
                //}
                //else {
                //FilterDecay = 0;
                //}
            }

            mpc_decoder_synthese_filter_float(d, buffer + 2304);

            output_frame_length = MPC_FRAME_LENGTH + FilterDecay;
        }
        else {                              // there are only FilterDecay samples needed for this frame
            output_frame_length = FilterDecay;
        }
    }

    if (d->samples_to_skip) {
        if (output_frame_length < d->samples_to_skip) {
            d->samples_to_skip -= output_frame_length;
            output_frame_length = 0;
        }
        else {
            output_frame_length -= d->samples_to_skip;
            memmove(
                buffer, 
                buffer + d->samples_to_skip * 2, 
                output_frame_length * 2 * sizeof (MPC_SAMPLE_FORMAT));
            d->samples_to_skip = 0;
        }
    }

    return output_frame_length;
}

mpc_uint32_t mpc_decoder_decode(
    mpc_decoder *d,
    MPC_SAMPLE_FORMAT *buffer, 
    mpc_uint32_t *vbr_update_acc, 
    mpc_uint32_t *vbr_update_bits)
{
    for(;;)
    {
        //const mpc_int32_t MaxBrokenFrames = 5; // PluginSettings.MaxBrokenFrames

        mpc_uint32_t RING = d->Zaehler;
        mpc_int32_t vbr_ring = (RING << 5) + d->pos;

        mpc_uint32_t valid_samples = mpc_decoder_decode_internal(d, buffer);

        if (valid_samples == (mpc_uint32_t)(-1) ) {
            return 0;
        }

        /**************** ERROR CONCEALMENT *****************/
        if (d->FrameWasValid == 0 ) {
            return (mpc_uint32_t)(-1);
        } 
        else {
            if (vbr_update_acc && vbr_update_bits) {
                (*vbr_update_acc) ++;
                vbr_ring = (d->Zaehler << 5) + d->pos - vbr_ring;
                if (vbr_ring < 0) {
                    vbr_ring += 524288;
                }
                (*vbr_update_bits) += vbr_ring;
            }

        }
        mpc_decoder_update_buffer(d, RING);

        if (valid_samples > 0) {
            return valid_samples;
        }
    }
}

void
mpc_decoder_requantisierung(mpc_decoder *d, const mpc_int32_t Last_Band) 
{
    mpc_int32_t     Band;
    mpc_int32_t     n;
    MPC_SAMPLE_FORMAT facL;
    MPC_SAMPLE_FORMAT facR;
    MPC_SAMPLE_FORMAT templ;
    MPC_SAMPLE_FORMAT tempr;
    MPC_SAMPLE_FORMAT* YL;
    MPC_SAMPLE_FORMAT* YR;
    mpc_int32_t*    L;
    mpc_int32_t*    R;

#ifdef MPC_FIXED_POINT
#if MPC_FIXED_POINT_FRACTPART == 14
#define MPC_MULTIPLY_SCF(CcVal, SCF_idx) \
    MPC_MULTIPLY_EX(CcVal, d->SCF[SCF_idx], d->SCF_shift[SCF_idx])
#else

#error FIXME, Cc table is in 18.14 format

#endif
#else
#define MPC_MULTIPLY_SCF(CcVal, SCF_idx) \
    MPC_MULTIPLY(CcVal, d->SCF[SCF_idx])
#endif
    // requantization and scaling of subband-samples
    for ( Band = 0; Band <= Last_Band; Band++ ) {   // setting pointers
        YL = d->Y_L[0] + Band;
        YR = d->Y_R[0] + Band;
        L  = d->Q[Band].L;
        R  = d->Q[Band].R;
        /************************** MS-coded **************************/
        if ( d->MS_Flag [Band] ) {
            if ( d->Res_L [Band] ) {
                if ( d->Res_R [Band] ) {    // M!=0, S!=0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = MPC_MULTIPLY_FLOAT_INT(facL,*L++))+(tempr = MPC_MULTIPLY_FLOAT_INT(facR,*R++));
                        *YR   = templ - tempr;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = MPC_MULTIPLY_FLOAT_INT(facL,*L++))+(tempr = MPC_MULTIPLY_FLOAT_INT(facR,*R++));
                        *YR   = templ - tempr;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = MPC_MULTIPLY_FLOAT_INT(facL,*L++))+(tempr = MPC_MULTIPLY_FLOAT_INT(facR,*R++));
                        *YR   = templ - tempr;
                    }
                } else {    // M!=0, S==0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                    }
                }
            } else {
                if (d->Res_R[Band])    // M==0, S!=0
                {
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = MPC_MULTIPLY_FLOAT_INT(facR,*(R++)));
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = MPC_MULTIPLY_FLOAT_INT(facR,*(R++)));
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = MPC_MULTIPLY_FLOAT_INT(facR,*(R++)));
                    }
                } else {    // M==0, S==0
                    for ( n = 0; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = 0;
                    }
                }
            }
        }
        /************************** LR-coded **************************/
        else {
            if ( d->Res_L [Band] ) {
                if ( d->Res_R [Band] ) {    // L!=0, R!=0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for (n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for (; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for (; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                } else {     // L!=0, R==0
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = 0;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = 0;
                    }
                    facL = MPC_MULTIPLY_SCF( Cc[d->Res_L[Band]] , (unsigned char)d->SCF_Index_L[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = MPC_MULTIPLY_FLOAT_INT(facL,*L++);
                        *YR = 0;
                    }
                }
            }
            else {
                if ( d->Res_R [Band] ) {    // L==0, R!=0
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][0]);
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = 0;
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][1]);
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = 0;
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                    facR = MPC_MULTIPLY_SCF( Cc[d->Res_R[Band]] , (unsigned char)d->SCF_Index_R[Band][2]);
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = 0;
                        *YR = MPC_MULTIPLY_FLOAT_INT(facR,*R++);
                    }
                } else {    // L==0, R==0
                    for ( n = 0; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = 0;
                    }
                }
            }
        }
    }
}

/****************************************** SV 6 ******************************************/
void
mpc_decoder_read_bitstream_sv6(mpc_decoder *d) 
{
    mpc_int32_t n,k;
    mpc_int32_t Max_used_Band=0;
    HuffmanTyp *Table;
    const HuffmanTyp *x1;
    const HuffmanTyp *x2;
    mpc_int32_t *L;
    mpc_int32_t *R;
    mpc_int32_t *ResL = d->Res_L;
    mpc_int32_t *ResR = d->Res_R;

    /************************ HEADER **************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    for (n=0; n <= d->Max_Band; ++n, ++ResL, ++ResR)
    {
        if      (n<11)           Table = d->Region_A;
        else if (n>=11 && n<=22) Table = d->Region_B;
        else /*if (n>=23)*/      Table = d->Region_C;

        *ResL = d->Q_res[n][mpc_decoder_huffman_decode(d, Table)];
        if (d->MS_used) {
            d->MS_Flag[n] = mpc_decoder_bitstream_read(d,  1);
        }
        *ResR = d->Q_res[n][mpc_decoder_huffman_decode(d, Table)];

        // only perform the following procedure up to the maximum non-zero subband
        if (*ResL || *ResR) Max_used_Band = n;
    }

    /************************* SCFI-Bundle *****************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR) {
        if (*ResL) mpc_decoder_scfi_bundle_read(d, d->SCFI_Bundle, &(d->SCFI_L[n]), &(d->DSCF_Flag_L[n]));
        if (*ResR) mpc_decoder_scfi_bundle_read(d, d->SCFI_Bundle, &(d->SCFI_R[n]), &(d->DSCF_Flag_R[n]));
    }

    /***************************** SCFI ********************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    L    = d->SCF_Index_L[0];
    R    = d->SCF_Index_R[0];
    for (n=0; n <= Max_used_Band; ++n, ++ResL, ++ResR, L+=3, R+=3)
    {
        if (*ResL)
        {
            /*********** DSCF ************/
            if (d->DSCF_Flag_L[n]==1)
            {
                L[2] = d->DSCF_Reference_L[n];
                switch (d->SCFI_L[n])
                {
                case 3:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 1:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    L[1] = L[0] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    L[2] = L[1];
                    break;
                case 2:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    L[1] = L[0];
                    L[2] = L[1] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    break;
                case 0:
                    L[0] = L[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    L[1] = L[0] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    L[2] = L[1] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    break;
                default:
                    return;
                    break;
                }
            }
            /************ SCF ************/
            else
            {
                switch (d->SCFI_L[n])
                {
                case 3:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 1:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = mpc_decoder_bitstream_read(d,  6);
                    L[2] = L[1];
                    break;
                case 2:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = L[0];
                    L[2] = mpc_decoder_bitstream_read(d,  6);
                    break;
                case 0:
                    L[0] = mpc_decoder_bitstream_read(d,  6);
                    L[1] = mpc_decoder_bitstream_read(d,  6);
                    L[2] = mpc_decoder_bitstream_read(d,  6);
                    break;
                default:
                    return;
                    break;
                }
            }
            // update Reference for DSCF
            d->DSCF_Reference_L[n] = L[2];
        }
        if (*ResR)
        {
            R[2] = d->DSCF_Reference_R[n];
            /*********** DSCF ************/
            if (d->DSCF_Flag_R[n]==1)
            {
                switch (d->SCFI_R[n])
                {
                case 3:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 1:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    R[1] = R[0] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    R[2] = R[1];
                    break;
                case 2:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    R[1] = R[0];
                    R[2] = R[1] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    break;
                case 0:
                    R[0] = R[2] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    R[1] = R[0] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    R[2] = R[1] + mpc_decoder_huffman_decode_fast(d,  d->DSCF_Entropie);
                    break;
                default:
                    return;
                    break;
                }
            }
            /************ SCF ************/
            else
            {
                switch (d->SCFI_R[n])
                {
                case 3:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 1:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = mpc_decoder_bitstream_read(d, 6);
                    R[2] = R[1];
                    break;
                case 2:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = R[0];
                    R[2] = mpc_decoder_bitstream_read(d, 6);
                    break;
                case 0:
                    R[0] = mpc_decoder_bitstream_read(d, 6);
                    R[1] = mpc_decoder_bitstream_read(d, 6);
                    R[2] = mpc_decoder_bitstream_read(d, 6);
                    break;
                default:
                    return;
                    break;
                }
            }
            // update Reference for DSCF
            d->DSCF_Reference_R[n] = R[2];
        }
    }

    /**************************** Samples ****************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    for (n=0; n <= Max_used_Band; ++n, ++ResL, ++ResR)
    {
        // setting pointers
        x1 = d->SampleHuff[*ResL];
        x2 = d->SampleHuff[*ResR];
        L = d->Q[n].L;
        R = d->Q[n].R;

        if (x1!=NULL || x2!=NULL)
            for (k=0; k<36; ++k)
            {
                if (x1 != NULL) *L++ = mpc_decoder_huffman_decode_fast(d,  x1);
                if (x2 != NULL) *R++ = mpc_decoder_huffman_decode_fast(d,  x2);
            }

        if (*ResL>7 || *ResR>7)
            for (k=0; k<36; ++k)
            {
                if (*ResL>7) *L++ = (mpc_int32_t)mpc_decoder_bitstream_read(d,  Res_bit[*ResL]) - Dc[*ResL];
                if (*ResR>7) *R++ = (mpc_int32_t)mpc_decoder_bitstream_read(d,  Res_bit[*ResR]) - Dc[*ResR];
            }
    }
}

/****************************************** SV 7 ******************************************/
void
mpc_decoder_read_bitstream_sv7(mpc_decoder *d) 
{
    // these arrays hold decoding results for bundled quantizers (3- and 5-step)
    /*static*/ mpc_int32_t idx30[] = { -1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1};
    /*static*/ mpc_int32_t idx31[] = { -1,-1,-1, 0, 0, 0, 1, 1, 1,-1,-1,-1, 0, 0, 0, 1, 1, 1,-1,-1,-1, 0, 0, 0, 1, 1, 1};
    /*static*/ mpc_int32_t idx32[] = { -1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    /*static*/ mpc_int32_t idx50[] = { -2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2};
    /*static*/ mpc_int32_t idx51[] = { -2,-2,-2,-2,-2,-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2};

    mpc_int32_t n,k;
    mpc_int32_t Max_used_Band=0;
    const HuffmanTyp *Table;
    mpc_int32_t idx;
    mpc_int32_t *L   ,*R;
    mpc_int32_t *ResL,*ResR;
    mpc_uint32_t tmp;

    /***************************** Header *****************************/
    ResL  = d->Res_L;
    ResR  = d->Res_R;

    // first subband
    *ResL = mpc_decoder_bitstream_read(d, 4);
    *ResR = mpc_decoder_bitstream_read(d, 4);
    if (d->MS_used && !(*ResL==0 && *ResR==0)) {
        d->MS_Flag[0] = mpc_decoder_bitstream_read(d, 1);
    }

    // consecutive subbands
    ++ResL; ++ResR; // increase pointers
    for (n=1; n <= d->Max_Band; ++n, ++ResL, ++ResR)
    {
        idx   = mpc_decoder_huffman_decode_fast(d, d->HuffHdr);
        *ResL = (idx!=4) ? *(ResL-1) + idx : (int) mpc_decoder_bitstream_read(d, 4);

        idx   = mpc_decoder_huffman_decode_fast(d, d->HuffHdr);
        *ResR = (idx!=4) ? *(ResR-1) + idx : (int) mpc_decoder_bitstream_read(d, 4);

        if (d->MS_used && !(*ResL==0 && *ResR==0)) {
            d->MS_Flag[n] = mpc_decoder_bitstream_read(d, 1);
        }

        // only perform following procedures up to the maximum non-zero subband
        if (*ResL!=0 || *ResR!=0) {
            Max_used_Band = n;
        }
    }
    /****************************** SCFI ******************************/
    L     = d->SCFI_L;
    R     = d->SCFI_R;
    ResL  = d->Res_L;
    ResR  = d->Res_R;
    for (n=0; n <= Max_used_Band; ++n, ++L, ++R, ++ResL, ++ResR) {
        if (*ResL) *L = mpc_decoder_huffman_decode_faster(d, d->HuffSCFI);
        if (*ResR) *R = mpc_decoder_huffman_decode_faster(d, d->HuffSCFI);
    }

    /**************************** SCF/DSCF ****************************/
    ResL  = d->Res_L;
    ResR  = d->Res_R;
    L     = d->SCF_Index_L[0];
    R     = d->SCF_Index_R[0];
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR, L+=3, R+=3) {
        if (*ResL)
        {
            L[2] = d->DSCF_Reference_L[n];
            switch (d->SCFI_L[n])
            {
            case 1:
                idx  = mpc_decoder_huffman_decode_fast(d, d->HuffDSCF);
                L[0] = (idx!=8) ? L[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                idx  = mpc_decoder_huffman_decode_fast(d, d->HuffDSCF);
                L[1] = (idx!=8) ? L[0] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                L[2] = L[1];
                break;
            case 3:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                L[0] = (idx!=8) ? L[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                L[1] = L[0];
                L[2] = L[1];
                break;
            case 2:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                L[0] = (idx!=8) ? L[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                L[1] = L[0];
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                L[2] = (idx!=8) ? L[1] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            case 0:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                L[0] = (idx!=8) ? L[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                L[1] = (idx!=8) ? L[0] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                L[2] = (idx!=8) ? L[1] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            default:
                return;
                break;
            }
            // update Reference for DSCF
            d->DSCF_Reference_L[n] = L[2];
        }
        if (*ResR)
        {
            R[2] = d->DSCF_Reference_R[n];
            switch (d->SCFI_R[n])
            {
            case 1:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[0] = (idx!=8) ? R[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[1] = (idx!=8) ? R[0] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                R[2] = R[1];
                break;
            case 3:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[0] = (idx!=8) ? R[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                R[1] = R[0];
                R[2] = R[1];
                break;
            case 2:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[0] = (idx!=8) ? R[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                R[1] = R[0];
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[2] = (idx!=8) ? R[1] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            case 0:
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[0] = (idx!=8) ? R[2] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[1] = (idx!=8) ? R[0] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                idx  = mpc_decoder_huffman_decode_fast(d,  d->HuffDSCF);
                R[2] = (idx!=8) ? R[1] + idx : (int) mpc_decoder_bitstream_read(d, 6);
                break;
            default:
                return;
                break;
            }
            // update Reference for DSCF
            d->DSCF_Reference_R[n] = R[2];
        }
    }
    /***************************** Samples ****************************/
    ResL = d->Res_L;
    ResR = d->Res_R;
    L    = d->Q[0].L;
    R    = d->Q[0].R;
    for (n=0; n <= Max_used_Band; ++n, ++ResL, ++ResR, L+=36, R+=36)
    {
        /************** links **************/
        switch (*ResL)
        {
        case  -2: case  -3: case  -4: case  -5: case  -6: case  -7: case  -8: case  -9:
        case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17:
            L += 36;
            break;
        case -1:
            for (k=0; k<36; k++ ) {
                tmp  = mpc_random_int(d);
                *L++ = ((tmp >> 24) & 0xFF) + ((tmp >> 16) & 0xFF) + ((tmp >>  8) & 0xFF) + ((tmp >>  0) & 0xFF) - 510;
            }
            break;
        case 0:
            L += 36;// increase pointer
            break;
        case 1:
            Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][1];
            for (k=0; k<12; ++k)
            {
                idx = mpc_decoder_huffman_decode_fast(d,  Table);
                *L++ = idx30[idx];
                *L++ = idx31[idx];
                *L++ = idx32[idx];
            }
            break;
        case 2:
            Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][2];
            for (k=0; k<18; ++k)
            {
                idx = mpc_decoder_huffman_decode_fast(d,  Table);
                *L++ = idx50[idx];
                *L++ = idx51[idx];
            }
            break;
        case 3:
        case 4:
            Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][*ResL];
            for (k=0; k<36; ++k)
                *L++ = mpc_decoder_huffman_decode_faster(d, Table);
            break;
        case 5:
            Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][*ResL];
            for (k=0; k<36; ++k)
                *L++ = mpc_decoder_huffman_decode_fast(d, Table);
            break;
        case 6:
        case 7:
            Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][*ResL];
            for (k=0; k<36; ++k)
                *L++ = mpc_decoder_huffman_decode(d, Table);
            break;
        case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
            tmp = Dc[*ResL];
            for (k=0; k<36; ++k)
                *L++ = (mpc_int32_t)mpc_decoder_bitstream_read(d, Res_bit[*ResL]) - tmp;
            break;
        default:
            return;
        }
        /************** rechts **************/
        switch (*ResR)
        {
        case  -2: case  -3: case  -4: case  -5: case  -6: case  -7: case  -8: case  -9:
        case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17:
            R += 36;
            break;
        case -1:
                for (k=0; k<36; k++ ) {
                    tmp  = mpc_random_int(d);
                    *R++ = ((tmp >> 24) & 0xFF) + ((tmp >> 16) & 0xFF) + ((tmp >>  8) & 0xFF) + ((tmp >>  0) & 0xFF) - 510;
                }
                break;
            case 0:
                R += 36;// increase pointer
                break;
            case 1:
                Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][1];
                for (k=0; k<12; ++k)
                {
                    idx = mpc_decoder_huffman_decode_fast(d, Table);
                    *R++ = idx30[idx];
                    *R++ = idx31[idx];
                    *R++ = idx32[idx];
                }
                break;
            case 2:
                Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][2];
                for (k=0; k<18; ++k)
                {
                    idx = mpc_decoder_huffman_decode_fast(d, Table);
                    *R++ = idx50[idx];
                    *R++ = idx51[idx];
                }
                break;
            case 3:
            case 4:
                Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][*ResR];
                for (k=0; k<36; ++k)
                    *R++ = mpc_decoder_huffman_decode_faster(d, Table);
                break;
            case 5:
                Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][*ResR];
                for (k=0; k<36; ++k)
                    *R++ = mpc_decoder_huffman_decode_fast(d, Table);
                break;
            case 6:
            case 7:
                Table = d->HuffQ[mpc_decoder_bitstream_read(d, 1)][*ResR];
                for (k=0; k<36; ++k)
                    *R++ = mpc_decoder_huffman_decode(d, Table);
                break;
            case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
                tmp = Dc[*ResR];
                for (k=0; k<36; ++k)
                    *R++ = (mpc_int32_t)mpc_decoder_bitstream_read(d, Res_bit[*ResR]) - tmp;
                break;
            default:
                return;
        }
    }
}

#ifdef USE_SEEK_TABLE
void mpc_decoder_free(mpc_decoder *d)
{
  if (d->SeekTable) free(d->SeekTable);
}
#endif

void mpc_decoder_setup(mpc_decoder *d, mpc_reader *r)
{
  d->r = r;

  d->HuffQ[0][0] = 0;
  d->HuffQ[1][0] = 0;
  d->HuffQ[0][1] = d->HuffQ1[0];
  d->HuffQ[1][1] = d->HuffQ1[1];
  d->HuffQ[0][2] = d->HuffQ2[0];
  d->HuffQ[1][2] = d->HuffQ2[1];
  d->HuffQ[0][3] = d->HuffQ3[0];
  d->HuffQ[1][3] = d->HuffQ3[1];
  d->HuffQ[0][4] = d->HuffQ4[0];
  d->HuffQ[1][4] = d->HuffQ4[1];
  d->HuffQ[0][5] = d->HuffQ5[0];
  d->HuffQ[1][5] = d->HuffQ5[1];
  d->HuffQ[0][6] = d->HuffQ6[0];
  d->HuffQ[1][6] = d->HuffQ6[1];
  d->HuffQ[0][7] = d->HuffQ7[0];
  d->HuffQ[1][7] = d->HuffQ7[1];

  d->SampleHuff[0] = NULL;
  d->SampleHuff[1] = d->Entropie_1;
  d->SampleHuff[2] = d->Entropie_2;
  d->SampleHuff[3] = d->Entropie_3;
  d->SampleHuff[4] = d->Entropie_4;
  d->SampleHuff[5] = d->Entropie_5;
  d->SampleHuff[6] = d->Entropie_6;
  d->SampleHuff[7] = d->Entropie_7;
  d->SampleHuff[8] = NULL;
  d->SampleHuff[9] = NULL;
  d->SampleHuff[10] = NULL;
  d->SampleHuff[11] = NULL;
  d->SampleHuff[12] = NULL;
  d->SampleHuff[13] = NULL;
  d->SampleHuff[14] = NULL;
  d->SampleHuff[15] = NULL;
  d->SampleHuff[16] = NULL;
  d->SampleHuff[17] = NULL;

#ifdef USE_SEEK_TABLE
  d->SeekTable = NULL;
#endif
  d->EQ_activated = 0;
  d->MPCHeaderPos = 0;
  d->StreamVersion = 0;
  d->MS_used = 0;
  d->FwdJumpInfo = 0;
  d->ActDecodePos = 0;
  d->FrameWasValid = 0;
  d->OverallFrames = 0;
  d->DecodedFrames = 0;
  d->LastValidSamples = 0;
  d->TrueGaplessPresent = 0;
  d->WordsRead = 0;
  d->Max_Band = 0;
  d->SampleRate = 0;
//  clips = 0;
  d->__r1 = 1;
  d->__r2 = 1;

  d->dword = 0;
  d->pos = 0;
  d->Zaehler = 0;
  d->WordsRead = 0;
  d->Max_Band = 0;

  mpc_decoder_initialisiere_quantisierungstabellen(d, 1.0f);
  mpc_decoder_init_huffman_sv6(d);
  mpc_decoder_init_huffman_sv7(d);
}

void mpc_decoder_set_streaminfo(mpc_decoder *d, mpc_streaminfo *si)
{
    mpc_decoder_reset_synthesis(d);
    mpc_decoder_reset_globals(d);

    d->StreamVersion      = si->stream_version;
    d->MS_used            = si->ms;
    d->Max_Band           = si->max_band;
    d->OverallFrames      = si->frames;
    d->MPCHeaderPos       = si->header_position;
    d->LastValidSamples   = si->last_frame_samples;
    d->TrueGaplessPresent = si->is_true_gapless;
    d->SampleRate         = (mpc_int32_t)si->sample_freq;

    d->samples_to_skip = MPC_DECODER_SYNTH_DELAY;

#ifdef USE_SEEK_TABLE
    if (NULL != d->SeekTable) free(d->SeekTable);
    d->SeekTable = (mpc_uint16_t *)calloc ( sizeof(mpc_uint16_t), d->OverallFrames + 64 );
#endif
}

mpc_bool_t mpc_decoder_initialize(mpc_decoder *d, mpc_streaminfo *si) 
{
    mpc_decoder_set_streaminfo(d, si);

    // AB: setting position to the beginning of the data-bitstream
    switch (d->StreamVersion) {
    case 0x04: f_seek(d, 4 + d->MPCHeaderPos); d->pos = 16; break;  // Geht auch über eine der Helperfunktionen
    case 0x05:
    case 0x06: f_seek(d, 8 + d->MPCHeaderPos); d->pos =  0; break;
    case 0x07:
    case 0x17: /*f_seek ( 24 + d->MPCHeaderPos );*/ d->pos =  8; break;
    default: return FALSE;
    }

    // AB: fill buffer and initialize decoder
    f_read_dword(d, d->Speicher, MEMSIZE );
    d->dword = d->Speicher[d->Zaehler = 0];

    return TRUE;
}

//---------------------------------------------------------------
// will seek from the beginning of the file to the desired
// position in ms (given by seek_needed)
//---------------------------------------------------------------
#if 0
static void
helper1(mpc_decoder *d, mpc_uint32_t bitpos) 
{
    f_seek(d, (bitpos >> 5) * 4 + d->MPCHeaderPos);
    f_read_dword(d, d->Speicher, 2);
    d->dword = d->Speicher[d->Zaehler = 0];
    d->pos = bitpos & 31;
}
#endif

static void
helper2(mpc_decoder *d, mpc_uint32_t bitpos) 
{
    f_seek(d, (bitpos>>5) * 4 + d->MPCHeaderPos);
    f_read_dword(d, d->Speicher, MEMSIZE);
    d->dword = d->Speicher[d->Zaehler = 0];
    d->pos = bitpos & 31;
}

#ifdef USE_SEEK_TABLE
static void
helper3(mpc_decoder *d, mpc_uint32_t bitpos, mpc_uint32_t* buffoffs) 
{
    d->pos = bitpos & 31;
    bitpos >>= 5;
    if ((mpc_uint32_t)(bitpos - *buffoffs) >= MEMSIZE - 2) {
        *buffoffs = bitpos;
        f_seek(d, bitpos * 4L + d->MPCHeaderPos);
        f_read_dword(d, d->Speicher, MEMSIZE );
    }
    d->dword = d->Speicher[d->Zaehler = bitpos - *buffoffs ];
}
#endif

static mpc_uint32_t get_initial_fpos(mpc_decoder *d, mpc_uint32_t StreamVersion)
{
    mpc_uint32_t fpos = 0;
    (void) StreamVersion;
    switch ( d->StreamVersion ) {                                                  // setting position to the beginning of the data-bitstream
    case  0x04: fpos =  48; break;
    case  0x05:
    case  0x06: fpos =  64; break;
    case  0x07:
    case  0x17: fpos = 200; break;
    }
    return fpos;
}

mpc_bool_t mpc_decoder_seek_seconds(mpc_decoder *d, double seconds) 
{
    return mpc_decoder_seek_sample(d, (mpc_int64_t)(seconds * (double)d->SampleRate + 0.5));
}

mpc_bool_t mpc_decoder_seek_sample(mpc_decoder *d, mpc_int64_t destsample) 
{
    mpc_uint32_t fpos;
    mpc_uint32_t fwd;

    fwd = (mpc_uint32_t) (destsample / MPC_FRAME_LENGTH);
    d->samples_to_skip = MPC_DECODER_SYNTH_DELAY + (mpc_uint32_t)(destsample % MPC_FRAME_LENGTH);

    memset(d->Y_L          , 0, sizeof d->Y_L           );
    memset(d->Y_R          , 0, sizeof d->Y_R           );
    memset(d->SCF_Index_L     , 0, sizeof d->SCF_Index_L      );
    memset(d->SCF_Index_R     , 0, sizeof d->SCF_Index_R      );
    memset(d->Res_L           , 0, sizeof d->Res_L            );
    memset(d->Res_R           , 0, sizeof d->Res_R            );
    memset(d->SCFI_L          , 0, sizeof d->SCFI_L           );
    memset(d->SCFI_R          , 0, sizeof d->SCFI_R           );
    memset(d->DSCF_Flag_L     , 0, sizeof d->DSCF_Flag_L      );
    memset(d->DSCF_Flag_R     , 0, sizeof d->DSCF_Flag_R      );
    memset(d->DSCF_Reference_L, 0, sizeof d->DSCF_Reference_L );
    memset(d->DSCF_Reference_R, 0, sizeof d->DSCF_Reference_R );
    memset(d->Q               , 0, sizeof d->Q                );
    memset(d->MS_Flag         , 0, sizeof d->MS_Flag          );

    // resetting synthesis filter to avoid "clicks"
    mpc_decoder_reset_synthesis(d);

    // prevent from desired position out of allowed range
    fwd = fwd < d->OverallFrames  ?  fwd  :  d->OverallFrames;

    // reset number of decoded frames
    d->DecodedFrames = 0;

    fpos = get_initial_fpos(d, d->StreamVersion);
    if (fpos == 0) {
        return FALSE;
    }

#ifdef USE_SEEK_TABLE
	{
		    mpc_uint32_t buffoffs = 0x80000000;
        for ( ; d->DecodedFrames + 1024 < fwd;  d->DecodedFrames++ ) {                   // proceed until 32 frames before (!!) desired position (allows to scan the scalefactors)
            if (  d->SeekTable [ d->DecodedFrames] == 0 ) {
                helper3( d, fpos, &buffoffs );
                fpos += d->SeekTable [d->DecodedFrames] = 20 + mpc_decoder_bitstream_read(d, 20);   // calculate desired pos (in Bits)
            } else {
                fpos += d->SeekTable [d->DecodedFrames];
            }
        }
    }
#endif
    helper2(d, fpos);

    // read the last 32 frames before the desired position to scan the scalefactors (artifactless jumping)
    for ( ; d->DecodedFrames < fwd; d->DecodedFrames++ ) {
        mpc_uint32_t   FrameBitCnt;
        mpc_uint32_t   RING;
        RING         = d->Zaehler;
        d->FwdJumpInfo  = mpc_decoder_bitstream_read(d, 20);    // read jump-info
#ifdef USE_SEEK_TABLE
        d->SeekTable[d->DecodedFrames] = 20 + d->FwdJumpInfo;
#endif
        d->ActDecodePos = (d->Zaehler << 5) + d->pos;
        FrameBitCnt  = mpc_decoder_bits_read(d);  // scanning the scalefactors and check for validity of frame
        if (d->StreamVersion >= 7)  {
            mpc_decoder_read_bitstream_sv7(d);
        }
        else {
            mpc_decoder_read_bitstream_sv6(d);
        }
        if (mpc_decoder_bits_read(d) - FrameBitCnt != d->FwdJumpInfo ) {
            return FALSE;
        }
        // update buffer
        if ((RING ^ d->Zaehler) & MEMSIZE2) {
            f_read_dword(d, d->Speicher + (RING & MEMSIZE2),  MEMSIZE2);
        }
    }

    // LastBitsRead = BitsRead ();
    // LastFrame = d->DecodedFrames;

    return TRUE;
}

void mpc_decoder_update_buffer(mpc_decoder *d, mpc_uint32_t RING) 
{
    if ((RING ^ d->Zaehler) & MEMSIZE2 ) {
        // update buffer
        f_read_dword(d, d->Speicher + (RING & MEMSIZE2), MEMSIZE2);
    }
}


