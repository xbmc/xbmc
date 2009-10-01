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

/// \file requant.c
/// Requantization function implementations.
/// \todo document me

#include <mpcdec/mpcdec.h>
#include <mpcdec/internal.h>

/* C O N S T A N T S */
// bits per sample for chosen quantizer
const mpc_uint32_t  Res_bit [18] = {
    0,  0,  0,  0,  0,  0,  0,  0,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16
};

// coefficients for requantization
// 65536/step bzw. 65536/(2*D+1)

#define _(X) MAKE_MPC_SAMPLE_EX(X,14)

const MPC_SAMPLE_FORMAT  __Cc [1 + 18] = {
      _(111.285962475327f),                                        // 32768/2/255*sqrt(3)
    _(65536.000000000000f), _(21845.333333333332f), _(13107.200000000001f), _(9362.285714285713f),
    _(7281.777777777777f),  _(4369.066666666666f),  _(2114.064516129032f), _(1040.253968253968f),
     _(516.031496062992f),  _(257.003921568627f),   _(128.250489236790f),   _(64.062561094819f),
       _(32.015632633121f),    _(16.003907203907f),     _(8.000976681723f),    _(4.000244155527f),
        _(2.000061037018f),     _(1.000015259021f)
};

#undef _

// offset for requantization
// 2*D+1 = steps of quantizer
const mpc_int32_t  __Dc [1 + 18] = {
      2,
      0,     1,     2,     3,     4,     7,    15,    31,    63,
    127,   255,   511,  1023,  2047,  4095,  8191, 16383, 32767
};

#ifdef MPC_FIXED_POINT
static mpc_uint32_t find_shift(double fval)
{
	mpc_int64_t val = (mpc_int64_t)fval;
	if (val<0) val = -val;
	mpc_uint32_t ptr = 0;
	while(val) {val>>=1;ptr++;}

	return ptr > 31 ? 0 : 31 - ptr;
}
#endif

/* F U N C T I O N S */

#define SET_SCF(N,X) d->SCF[N] = MAKE_MPC_SAMPLE_EX(X,d->SCF_shift[N] = (unsigned char)find_shift(X));

void
mpc_decoder_scale_output(mpc_decoder *d, double factor) 
{
    mpc_int32_t     n;
    double  f1;
    double  f2;
#ifndef MPC_FIXED_POINT
	factor *= 1.0 / (double)(1<<(MPC_FIXED_POINT_SHIFT-1));
#else
	factor *= 1.0 / (double)(1<<(16 - MPC_FIXED_POINT_SHIFT));
#endif
    f1 = f2 = factor;

    // handles +1.58...-98.41 dB, where's scf[n] / scf[n-1] = 1.20050805774840750476
	
	SET_SCF(1,factor);

	f1 *=   0.83298066476582673961;
	f2 *= 1/0.83298066476582673961;

    for ( n = 1; n <= 128; n++ ) {
		SET_SCF((unsigned char)(1+n),f1);
		SET_SCF((unsigned char)(1-n),f2);
        f1 *=   0.83298066476582673961;
        f2 *= 1/0.83298066476582673961;
    }
}

static void
mpc_decoder_quantisierungsmodes(mpc_decoder *d) // conversion: index -> quantizer (bitstream reading)
{                                               // conversion: quantizer -> index (bitstream writing)
    mpc_int32_t  Band = 0;
    mpc_int32_t  i;

    do {
        d->Q_bit [Band] = 4;
        for ( i = 0; i < 16-1; i++ )
            d->Q_res [Band] [i] = i;
        d->Q_res [Band][i] = 17;
        Band++;
    } while ( Band < 11 );

    do {
        d->Q_bit [Band] = 3;
        for ( i = 0; i < 8-1; i++ )
            d->Q_res [Band] [i] = i;
        d->Q_res [Band] [i] = 17;
        Band++;
    } while ( Band < 23 );

    do {
        d->Q_bit [Band] = 2;
        for ( i = 0; i < 4-1; i++ )
            d->Q_res [Band] [i] = i;
        d->Q_res [Band] [i] = 17;
        Band++;
    } while ( Band < 32 );
}

void
mpc_decoder_initialisiere_quantisierungstabellen(mpc_decoder *d, double scale_factor) 
{
    mpc_decoder_quantisierungsmodes(d);
    mpc_decoder_scale_output(d, scale_factor);
}
