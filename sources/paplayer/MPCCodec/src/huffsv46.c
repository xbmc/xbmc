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

/// \file huffsv46.c
/// Implementations of huffman decoding for streamversions < 7.

#include <mpcdec/mpcdec.h>
#include <mpcdec/requant.h>
#include <mpcdec/huffman.h>

void
mpc_decoder_init_huffman_sv6(mpc_decoder *d) 
{
    mpc_decoder_init_huffman_sv6_tables(d);
    mpc_decoder_resort_huff_tables(16, d->Region_A      , 0);
    mpc_decoder_resort_huff_tables( 8, d->Region_B      , 0);
    mpc_decoder_resort_huff_tables( 4, d->Region_C      , 0);
    mpc_decoder_resort_huff_tables( 8, d->SCFI_Bundle   , 0);
    mpc_decoder_resort_huff_tables(13, d->DSCF_Entropie , 6);
    mpc_decoder_resort_huff_tables( 3, d->Entropie_1    , Dc[1]);
    mpc_decoder_resort_huff_tables( 5, d->Entropie_2    , Dc[2]);
    mpc_decoder_resort_huff_tables( 7, d->Entropie_3    , Dc[3]);
    mpc_decoder_resort_huff_tables( 9, d->Entropie_4    , Dc[4]);
    mpc_decoder_resort_huff_tables(15, d->Entropie_5    , Dc[5]);
    mpc_decoder_resort_huff_tables(31, d->Entropie_6    , Dc[6]);
    mpc_decoder_resort_huff_tables(63, d->Entropie_7    , Dc[7]);
}

void 
mpc_decoder_init_huffman_sv6_tables(mpc_decoder *d) 
{
    // SCFI-bundle
    d->SCFI_Bundle[7].Code=  1; d->SCFI_Bundle[7].Length= 1;
    d->SCFI_Bundle[3].Code=  1; d->SCFI_Bundle[3].Length= 2;
    d->SCFI_Bundle[5].Code=  0; d->SCFI_Bundle[5].Length= 3;
    d->SCFI_Bundle[1].Code=  7; d->SCFI_Bundle[1].Length= 5;
    d->SCFI_Bundle[2].Code=  6; d->SCFI_Bundle[2].Length= 5;
    d->SCFI_Bundle[4].Code=  4; d->SCFI_Bundle[4].Length= 5;
    d->SCFI_Bundle[0].Code= 11; d->SCFI_Bundle[0].Length= 6;
    d->SCFI_Bundle[6].Code= 10; d->SCFI_Bundle[6].Length= 6;

    // region A (subbands  0..10)
    d->Region_A[ 1].Code=    1; d->Region_A[ 1].Length=  1;
    d->Region_A[ 2].Code=    0; d->Region_A[ 2].Length=  2;
    d->Region_A[ 0].Code=    2; d->Region_A[ 0].Length=  3;
    d->Region_A[ 3].Code=   15; d->Region_A[ 3].Length=  5;
    d->Region_A[ 5].Code=   13; d->Region_A[ 5].Length=  5;
    d->Region_A[ 6].Code=   12; d->Region_A[ 6].Length=  5;
    d->Region_A[ 4].Code=   29; d->Region_A[ 4].Length=  6;
    d->Region_A[ 7].Code=   57; d->Region_A[ 7].Length=  7;
    d->Region_A[ 8].Code=  113; d->Region_A[ 8].Length=  8;
    d->Region_A[ 9].Code=  225; d->Region_A[ 9].Length=  9;
    d->Region_A[10].Code=  449; d->Region_A[10].Length= 10;
    d->Region_A[11].Code=  897; d->Region_A[11].Length= 11;
    d->Region_A[12].Code= 1793; d->Region_A[12].Length= 12;
    d->Region_A[13].Code= 3585; d->Region_A[13].Length= 13;
    d->Region_A[14].Code= 7169; d->Region_A[14].Length= 14;
    d->Region_A[15].Code= 7168; d->Region_A[15].Length= 14;

    // region B (subbands 11..22)
    d->Region_B[1].Code= 1; d->Region_B[1].Length= 1;
    d->Region_B[0].Code= 1; d->Region_B[0].Length= 2;
    d->Region_B[2].Code= 1; d->Region_B[2].Length= 3;
    d->Region_B[3].Code= 1; d->Region_B[3].Length= 4;
    d->Region_B[4].Code= 1; d->Region_B[4].Length= 5;
    d->Region_B[5].Code= 1; d->Region_B[5].Length= 6;
    d->Region_B[6].Code= 1; d->Region_B[6].Length= 7;
    d->Region_B[7].Code= 0; d->Region_B[7].Length= 7;

    // region C (subbands 23..31)
    d->Region_C[0].Code= 1; d->Region_C[0].Length= 1;
    d->Region_C[1].Code= 1; d->Region_C[1].Length= 2;
    d->Region_C[2].Code= 1; d->Region_C[2].Length= 3;
    d->Region_C[3].Code= 0; d->Region_C[3].Length= 3;

    // DSCF
    d->DSCF_Entropie[ 6].Code=  0; d->DSCF_Entropie[ 6].Length= 2;
    d->DSCF_Entropie[ 7].Code=  7; d->DSCF_Entropie[ 7].Length= 3;
    d->DSCF_Entropie[ 5].Code=  4; d->DSCF_Entropie[ 5].Length= 3;
    d->DSCF_Entropie[ 8].Code=  3; d->DSCF_Entropie[ 8].Length= 3;
    d->DSCF_Entropie[ 9].Code= 13; d->DSCF_Entropie[ 9].Length= 4;
    d->DSCF_Entropie[ 4].Code= 11; d->DSCF_Entropie[ 4].Length= 4;
    d->DSCF_Entropie[10].Code= 10; d->DSCF_Entropie[10].Length= 4;
    d->DSCF_Entropie[ 2].Code=  4; d->DSCF_Entropie[ 2].Length= 4;
    d->DSCF_Entropie[11].Code= 25; d->DSCF_Entropie[11].Length= 5;
    d->DSCF_Entropie[ 3].Code= 24; d->DSCF_Entropie[ 3].Length= 5;
    d->DSCF_Entropie[ 1].Code= 11; d->DSCF_Entropie[ 1].Length= 5;
    d->DSCF_Entropie[12].Code= 21; d->DSCF_Entropie[12].Length= 6;
    d->DSCF_Entropie[ 0].Code= 20; d->DSCF_Entropie[ 0].Length= 6;

    // first quantizer
    d->Entropie_1[1].Code= 1; d->Entropie_1[1].Length= 1;
    d->Entropie_1[0].Code= 1; d->Entropie_1[0].Length= 2;
    d->Entropie_1[2].Code= 0; d->Entropie_1[2].Length= 2;

    // second quantizer
    d->Entropie_2[2].Code=  3; d->Entropie_2[2].Length= 2;
    d->Entropie_2[3].Code=  1; d->Entropie_2[3].Length= 2;
    d->Entropie_2[1].Code=  0; d->Entropie_2[1].Length= 2;
    d->Entropie_2[4].Code=  5; d->Entropie_2[4].Length= 3;
    d->Entropie_2[0].Code=  4; d->Entropie_2[0].Length= 3;

    // third quantizer
    d->Entropie_3[3].Code=  3; d->Entropie_3[3].Length= 2;
    d->Entropie_3[2].Code=  1; d->Entropie_3[2].Length= 2;
    d->Entropie_3[4].Code=  0; d->Entropie_3[4].Length= 2;
    d->Entropie_3[1].Code=  5; d->Entropie_3[1].Length= 3;
    d->Entropie_3[5].Code=  9; d->Entropie_3[5].Length= 4;
    d->Entropie_3[0].Code= 17; d->Entropie_3[0].Length= 5;
    d->Entropie_3[6].Code= 16; d->Entropie_3[6].Length= 5;

    // forth quantizer
    d->Entropie_4[4].Code=  0; d->Entropie_4[4].Length= 2;
    d->Entropie_4[5].Code=  6; d->Entropie_4[5].Length= 3;
    d->Entropie_4[3].Code=  5; d->Entropie_4[3].Length= 3;
    d->Entropie_4[6].Code=  4; d->Entropie_4[6].Length= 3;
    d->Entropie_4[2].Code=  3; d->Entropie_4[2].Length= 3;
    d->Entropie_4[7].Code= 15; d->Entropie_4[7].Length= 4;
    d->Entropie_4[1].Code= 14; d->Entropie_4[1].Length= 4;
    d->Entropie_4[0].Code=  5; d->Entropie_4[0].Length= 4;
    d->Entropie_4[8].Code=  4; d->Entropie_4[8].Length= 4;

    // fifth quantizer
    d->Entropie_5[7 ].Code=  4; d->Entropie_5[7 ].Length= 3;
    d->Entropie_5[8 ].Code=  3; d->Entropie_5[8 ].Length= 3;
    d->Entropie_5[6 ].Code=  2; d->Entropie_5[6 ].Length= 3;
    d->Entropie_5[9 ].Code=  0; d->Entropie_5[9 ].Length= 3;
    d->Entropie_5[5 ].Code= 15; d->Entropie_5[5 ].Length= 4;
    d->Entropie_5[4 ].Code= 13; d->Entropie_5[4 ].Length= 4;
    d->Entropie_5[10].Code= 12; d->Entropie_5[10].Length= 4;
    d->Entropie_5[11].Code= 10; d->Entropie_5[11].Length= 4;
    d->Entropie_5[3 ].Code=  3; d->Entropie_5[3 ].Length= 4;
    d->Entropie_5[12].Code=  2; d->Entropie_5[12].Length= 4;
    d->Entropie_5[2 ].Code= 29; d->Entropie_5[2 ].Length= 5;
    d->Entropie_5[1 ].Code= 23; d->Entropie_5[1 ].Length= 5;
    d->Entropie_5[13].Code= 22; d->Entropie_5[13].Length= 5;
    d->Entropie_5[0 ].Code= 57; d->Entropie_5[0 ].Length= 6;
    d->Entropie_5[14].Code= 56; d->Entropie_5[14].Length= 6;

    // sixth quantizer
    d->Entropie_6[15].Code=  9; d->Entropie_6[15].Length= 4;
    d->Entropie_6[16].Code=  8; d->Entropie_6[16].Length= 4;
    d->Entropie_6[14].Code=  7; d->Entropie_6[14].Length= 4;
    d->Entropie_6[18].Code=  6; d->Entropie_6[18].Length= 4;
    d->Entropie_6[17].Code=  5; d->Entropie_6[17].Length= 4;
    d->Entropie_6[12].Code=  3; d->Entropie_6[12].Length= 4;
    d->Entropie_6[13].Code=  2; d->Entropie_6[13].Length= 4;
    d->Entropie_6[19].Code=  0; d->Entropie_6[19].Length= 4;
    d->Entropie_6[11].Code= 31; d->Entropie_6[11].Length= 5;
    d->Entropie_6[20].Code= 30; d->Entropie_6[20].Length= 5;
    d->Entropie_6[10].Code= 29; d->Entropie_6[10].Length= 5;
    d->Entropie_6[9 ].Code= 27; d->Entropie_6[9 ].Length= 5;
    d->Entropie_6[21].Code= 26; d->Entropie_6[21].Length= 5;
    d->Entropie_6[22].Code= 25; d->Entropie_6[22].Length= 5;
    d->Entropie_6[8 ].Code= 24; d->Entropie_6[8 ].Length= 5;
    d->Entropie_6[7 ].Code= 23; d->Entropie_6[7 ].Length= 5;
    d->Entropie_6[23].Code= 21; d->Entropie_6[23].Length= 5;
    d->Entropie_6[6 ].Code=  9; d->Entropie_6[6 ].Length= 5;
    d->Entropie_6[24].Code=  3; d->Entropie_6[24].Length= 5;
    d->Entropie_6[25].Code= 57; d->Entropie_6[25].Length= 6;
    d->Entropie_6[5 ].Code= 56; d->Entropie_6[5 ].Length= 6;
    d->Entropie_6[4 ].Code= 45; d->Entropie_6[4 ].Length= 6;
    d->Entropie_6[26].Code= 41; d->Entropie_6[26].Length= 6;
    d->Entropie_6[2 ].Code= 40; d->Entropie_6[2 ].Length= 6;
    d->Entropie_6[27].Code= 17; d->Entropie_6[27].Length= 6;
    d->Entropie_6[28].Code= 16; d->Entropie_6[28].Length= 6;
    d->Entropie_6[3 ].Code=  5; d->Entropie_6[3 ].Length= 6;
    d->Entropie_6[29].Code= 89; d->Entropie_6[29].Length= 7;
    d->Entropie_6[1 ].Code= 88; d->Entropie_6[1 ].Length= 7;
    d->Entropie_6[30].Code=  9; d->Entropie_6[30].Length= 7;
    d->Entropie_6[0 ].Code=  8; d->Entropie_6[0 ].Length= 7;

    // seventh quantizer
    d->Entropie_7[25].Code=   0; d->Entropie_7[25].Length= 5;
    d->Entropie_7[37].Code=   1; d->Entropie_7[37].Length= 5;
    d->Entropie_7[62].Code=  16; d->Entropie_7[62].Length= 8;
    d->Entropie_7[ 0].Code=  17; d->Entropie_7[ 0].Length= 8;
    d->Entropie_7[ 3].Code=   9; d->Entropie_7[ 3].Length= 7;
    d->Entropie_7[ 5].Code=  10; d->Entropie_7[ 5].Length= 7;
    d->Entropie_7[ 6].Code=  11; d->Entropie_7[ 6].Length= 7;
    d->Entropie_7[38].Code=   3; d->Entropie_7[38].Length= 5;
    d->Entropie_7[35].Code=   4; d->Entropie_7[35].Length= 5;
    d->Entropie_7[33].Code=   5; d->Entropie_7[33].Length= 5;
    d->Entropie_7[24].Code=   6; d->Entropie_7[24].Length= 5;
    d->Entropie_7[27].Code=   7; d->Entropie_7[27].Length= 5;
    d->Entropie_7[26].Code=   8; d->Entropie_7[26].Length= 5;
    d->Entropie_7[12].Code=  18; d->Entropie_7[12].Length= 6;
    d->Entropie_7[50].Code=  19; d->Entropie_7[50].Length= 6;
    d->Entropie_7[29].Code=  10; d->Entropie_7[29].Length= 5;
    d->Entropie_7[31].Code=  11; d->Entropie_7[31].Length= 5;
    d->Entropie_7[36].Code=  12; d->Entropie_7[36].Length= 5;
    d->Entropie_7[34].Code=  13; d->Entropie_7[34].Length= 5;
    d->Entropie_7[28].Code=  14; d->Entropie_7[28].Length= 5;
    d->Entropie_7[49].Code=  30; d->Entropie_7[49].Length= 6;
    d->Entropie_7[56].Code=  62; d->Entropie_7[56].Length= 7;
    d->Entropie_7[ 7].Code=  63; d->Entropie_7[ 7].Length= 7;
    d->Entropie_7[32].Code=  16; d->Entropie_7[32].Length= 5;
    d->Entropie_7[30].Code=  17; d->Entropie_7[30].Length= 5;
    d->Entropie_7[13].Code=  36; d->Entropie_7[13].Length= 6;
    d->Entropie_7[55].Code=  74; d->Entropie_7[55].Length= 7;
    d->Entropie_7[61].Code= 150; d->Entropie_7[61].Length= 8;
    d->Entropie_7[ 1].Code= 151; d->Entropie_7[ 1].Length= 8;
    d->Entropie_7[14].Code=  38; d->Entropie_7[14].Length= 6;
    d->Entropie_7[48].Code=  39; d->Entropie_7[48].Length= 6;
    d->Entropie_7[ 4].Code=  80; d->Entropie_7[ 4].Length= 7;
    d->Entropie_7[58].Code=  81; d->Entropie_7[58].Length= 7;
    d->Entropie_7[47].Code=  41; d->Entropie_7[47].Length= 6;
    d->Entropie_7[15].Code=  42; d->Entropie_7[15].Length= 6;
    d->Entropie_7[16].Code=  43; d->Entropie_7[16].Length= 6;
    d->Entropie_7[54].Code=  88; d->Entropie_7[54].Length= 7;
    d->Entropie_7[ 8].Code=  89; d->Entropie_7[ 8].Length= 7;
    d->Entropie_7[17].Code=  45; d->Entropie_7[17].Length= 6;
    d->Entropie_7[46].Code=  46; d->Entropie_7[46].Length= 6;
    d->Entropie_7[45].Code=  47; d->Entropie_7[45].Length= 6;
    d->Entropie_7[53].Code=  96; d->Entropie_7[53].Length= 7;
    d->Entropie_7[ 9].Code=  97; d->Entropie_7[ 9].Length= 7;
    d->Entropie_7[43].Code=  49; d->Entropie_7[43].Length= 6;
    d->Entropie_7[19].Code=  50; d->Entropie_7[19].Length= 6;
    d->Entropie_7[18].Code=  51; d->Entropie_7[18].Length= 6;
    d->Entropie_7[44].Code=  52; d->Entropie_7[44].Length= 6;
    d->Entropie_7[ 2].Code= 212; d->Entropie_7[ 2].Length= 8;
    d->Entropie_7[60].Code= 213; d->Entropie_7[60].Length= 8;
    d->Entropie_7[10].Code= 107; d->Entropie_7[10].Length= 7;
    d->Entropie_7[42].Code=  54; d->Entropie_7[42].Length= 6;
    d->Entropie_7[41].Code=  55; d->Entropie_7[41].Length= 6;
    d->Entropie_7[20].Code=  56; d->Entropie_7[20].Length= 6;
    d->Entropie_7[21].Code=  57; d->Entropie_7[21].Length= 6;
    d->Entropie_7[52].Code= 116; d->Entropie_7[52].Length= 7;
    d->Entropie_7[51].Code= 117; d->Entropie_7[51].Length= 7;
    d->Entropie_7[40].Code=  59; d->Entropie_7[40].Length= 6;
    d->Entropie_7[22].Code=  60; d->Entropie_7[22].Length= 6;
    d->Entropie_7[23].Code=  61; d->Entropie_7[23].Length= 6;
    d->Entropie_7[39].Code=  62; d->Entropie_7[39].Length= 6;
    d->Entropie_7[11].Code= 126; d->Entropie_7[11].Length= 7;
    d->Entropie_7[57].Code= 254; d->Entropie_7[57].Length= 8;
    d->Entropie_7[59].Code= 255; d->Entropie_7[59].Length= 8;
}
