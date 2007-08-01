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

/// \file huffsv7.c
/// Implementations of sv7 huffman decoding functions.

#include <mpcdec/mpcdec.h>
#include <mpcdec/huffman.h>
#include <mpcdec/requant.h>

void
mpc_decoder_init_huffman_sv7(mpc_decoder *d) 
{
    mpc_decoder_init_huffman_sv7_tables(d);
    mpc_decoder_resort_huff_tables(10, &(d->HuffHdr[0])   , 5);
    mpc_decoder_resort_huff_tables( 4, &(d->HuffSCFI[0])  , 0);
    mpc_decoder_resort_huff_tables(16, &(d->HuffDSCF[0])  , 7);
    mpc_decoder_resort_huff_tables(27, &(d->HuffQ1[0][0]) , 0);
    mpc_decoder_resort_huff_tables(27, &(d->HuffQ1[1][0]) , 0);
    mpc_decoder_resort_huff_tables(25, &(d->HuffQ2[0][0]) , 0);
    mpc_decoder_resort_huff_tables(25, &(d->HuffQ2[1][0]) , 0);
    mpc_decoder_resort_huff_tables( 7, &(d->HuffQ3[0][0]) , Dc[3]);
    mpc_decoder_resort_huff_tables( 7, &(d->HuffQ3[1][0]) , Dc[3]);
    mpc_decoder_resort_huff_tables( 9, &(d->HuffQ4[0][0]) , Dc[4]);
    mpc_decoder_resort_huff_tables( 9, &(d->HuffQ4[1][0]) , Dc[4]);
    mpc_decoder_resort_huff_tables(15, &(d->HuffQ5[0][0]) , Dc[5]);
    mpc_decoder_resort_huff_tables(15, &(d->HuffQ5[1][0]) , Dc[5]);
    mpc_decoder_resort_huff_tables(31, &(d->HuffQ6[0][0]) , Dc[6]);
    mpc_decoder_resort_huff_tables(31, &(d->HuffQ6[1][0]) , Dc[6]);
    mpc_decoder_resort_huff_tables(63, &(d->HuffQ7[0][0]) , Dc[7]);
    mpc_decoder_resort_huff_tables(63, &(d->HuffQ7[1][0]) , Dc[7]);
}

void 
mpc_decoder_init_huffman_sv7_tables(mpc_decoder *d) 
{
    /***************************** SCFI *******************************/
    d->HuffSCFI[0].Code = 2; d->HuffSCFI[0].Length = 3;
    d->HuffSCFI[1].Code = 1; d->HuffSCFI[1].Length = 1;
    d->HuffSCFI[2].Code = 3; d->HuffSCFI[2].Length = 3;
    d->HuffSCFI[3].Code = 0; d->HuffSCFI[3].Length = 2;

    /***************************** DSCF *******************************/
    d->HuffDSCF[ 0].Code = 32; d->HuffDSCF[ 0].Length = 6;
    d->HuffDSCF[ 1].Code =  4; d->HuffDSCF[ 1].Length = 5;
    d->HuffDSCF[ 2].Code = 17; d->HuffDSCF[ 2].Length = 5;
    d->HuffDSCF[ 3].Code = 30; d->HuffDSCF[ 3].Length = 5;
    d->HuffDSCF[ 4].Code = 13; d->HuffDSCF[ 4].Length = 4;
    d->HuffDSCF[ 5].Code =  0; d->HuffDSCF[ 5].Length = 3;
    d->HuffDSCF[ 6].Code =  3; d->HuffDSCF[ 6].Length = 3;
    d->HuffDSCF[ 7].Code =  9; d->HuffDSCF[ 7].Length = 4;
    d->HuffDSCF[ 8].Code =  5; d->HuffDSCF[ 8].Length = 3;
    d->HuffDSCF[ 9].Code =  2; d->HuffDSCF[ 9].Length = 3;
    d->HuffDSCF[10].Code = 14; d->HuffDSCF[10].Length = 4;
    d->HuffDSCF[11].Code =  3; d->HuffDSCF[11].Length = 4;
    d->HuffDSCF[12].Code = 31; d->HuffDSCF[12].Length = 5;
    d->HuffDSCF[13].Code =  5; d->HuffDSCF[13].Length = 5;
    d->HuffDSCF[14].Code = 33; d->HuffDSCF[14].Length = 6;
    d->HuffDSCF[15].Code = 12; d->HuffDSCF[15].Length = 4;

    /************************* frame-header ***************************/
    /***************** differential quantizer indizes *****************/
    d->HuffHdr[0].Code =  92; d->HuffHdr[0].Length = 8;
    d->HuffHdr[1].Code =  47; d->HuffHdr[1].Length = 7;
    d->HuffHdr[2].Code =  10; d->HuffHdr[2].Length = 5;
    d->HuffHdr[3].Code =   4; d->HuffHdr[3].Length = 4;
    d->HuffHdr[4].Code =   0; d->HuffHdr[4].Length = 2;
    d->HuffHdr[5].Code =   1; d->HuffHdr[5].Length = 1;
    d->HuffHdr[6].Code =   3; d->HuffHdr[6].Length = 3;
    d->HuffHdr[7].Code =  22; d->HuffHdr[7].Length = 6;
    d->HuffHdr[8].Code = 187; d->HuffHdr[8].Length = 9;
    d->HuffHdr[9].Code = 186; d->HuffHdr[9].Length = 9;

    /********************** 3-step quantizer **************************/
    /********************* 3 bundled samples **************************/
    //less shaped, book 0
    d->HuffQ1[0][ 0].Code = 54; d->HuffQ1[0][ 0].Length = 6;
    d->HuffQ1[0][ 1].Code =  9; d->HuffQ1[0][ 1].Length = 5;
    d->HuffQ1[0][ 2].Code = 32; d->HuffQ1[0][ 2].Length = 6;
    d->HuffQ1[0][ 3].Code =  5; d->HuffQ1[0][ 3].Length = 5;
    d->HuffQ1[0][ 4].Code = 10; d->HuffQ1[0][ 4].Length = 4;
    d->HuffQ1[0][ 5].Code =  7; d->HuffQ1[0][ 5].Length = 5;
    d->HuffQ1[0][ 6].Code = 52; d->HuffQ1[0][ 6].Length = 6;
    d->HuffQ1[0][ 7].Code =  0; d->HuffQ1[0][ 7].Length = 5;
    d->HuffQ1[0][ 8].Code = 35; d->HuffQ1[0][ 8].Length = 6;
    d->HuffQ1[0][ 9].Code = 10; d->HuffQ1[0][ 9].Length = 5;
    d->HuffQ1[0][10].Code =  6; d->HuffQ1[0][10].Length = 4;
    d->HuffQ1[0][11].Code =  4; d->HuffQ1[0][11].Length = 5;
    d->HuffQ1[0][12].Code = 11; d->HuffQ1[0][12].Length = 4;
    d->HuffQ1[0][13].Code =  7; d->HuffQ1[0][13].Length = 3;
    d->HuffQ1[0][14].Code = 12; d->HuffQ1[0][14].Length = 4;
    d->HuffQ1[0][15].Code =  3; d->HuffQ1[0][15].Length = 5;
    d->HuffQ1[0][16].Code =  7; d->HuffQ1[0][16].Length = 4;
    d->HuffQ1[0][17].Code = 11; d->HuffQ1[0][17].Length = 5;
    d->HuffQ1[0][18].Code = 34; d->HuffQ1[0][18].Length = 6;
    d->HuffQ1[0][19].Code =  1; d->HuffQ1[0][19].Length = 5;
    d->HuffQ1[0][20].Code = 53; d->HuffQ1[0][20].Length = 6;
    d->HuffQ1[0][21].Code =  6; d->HuffQ1[0][21].Length = 5;
    d->HuffQ1[0][22].Code =  9; d->HuffQ1[0][22].Length = 4;
    d->HuffQ1[0][23].Code =  2; d->HuffQ1[0][23].Length = 5;
    d->HuffQ1[0][24].Code = 33; d->HuffQ1[0][24].Length = 6;
    d->HuffQ1[0][25].Code =  8; d->HuffQ1[0][25].Length = 5;
    d->HuffQ1[0][26].Code = 55; d->HuffQ1[0][26].Length = 6;

    //more shaped, book 1
    d->HuffQ1[1][ 0].Code = 103; d->HuffQ1[1][ 0].Length = 8;
    d->HuffQ1[1][ 1].Code =  62; d->HuffQ1[1][ 1].Length = 7;
    d->HuffQ1[1][ 2].Code = 225; d->HuffQ1[1][ 2].Length = 9;
    d->HuffQ1[1][ 3].Code =  55; d->HuffQ1[1][ 3].Length = 7;
    d->HuffQ1[1][ 4].Code =   3; d->HuffQ1[1][ 4].Length = 4;
    d->HuffQ1[1][ 5].Code =  52; d->HuffQ1[1][ 5].Length = 7;
    d->HuffQ1[1][ 6].Code = 101; d->HuffQ1[1][ 6].Length = 8;
    d->HuffQ1[1][ 7].Code =  60; d->HuffQ1[1][ 7].Length = 7;
    d->HuffQ1[1][ 8].Code = 227; d->HuffQ1[1][ 8].Length = 9;
    d->HuffQ1[1][ 9].Code =  24; d->HuffQ1[1][ 9].Length = 6;
    d->HuffQ1[1][10].Code =   0; d->HuffQ1[1][10].Length = 4;
    d->HuffQ1[1][11].Code =  61; d->HuffQ1[1][11].Length = 7;
    d->HuffQ1[1][12].Code =   4; d->HuffQ1[1][12].Length = 4;
    d->HuffQ1[1][13].Code =   1; d->HuffQ1[1][13].Length = 1;
    d->HuffQ1[1][14].Code =   5; d->HuffQ1[1][14].Length = 4;
    d->HuffQ1[1][15].Code =  63; d->HuffQ1[1][15].Length = 7;
    d->HuffQ1[1][16].Code =   1; d->HuffQ1[1][16].Length = 4;
    d->HuffQ1[1][17].Code =  59; d->HuffQ1[1][17].Length = 7;
    d->HuffQ1[1][18].Code = 226; d->HuffQ1[1][18].Length = 9;
    d->HuffQ1[1][19].Code =  57; d->HuffQ1[1][19].Length = 7;
    d->HuffQ1[1][20].Code = 100; d->HuffQ1[1][20].Length = 8;
    d->HuffQ1[1][21].Code =  53; d->HuffQ1[1][21].Length = 7;
    d->HuffQ1[1][22].Code =   2; d->HuffQ1[1][22].Length = 4;
    d->HuffQ1[1][23].Code =  54; d->HuffQ1[1][23].Length = 7;
    d->HuffQ1[1][24].Code = 224; d->HuffQ1[1][24].Length = 9;
    d->HuffQ1[1][25].Code =  58; d->HuffQ1[1][25].Length = 7;
    d->HuffQ1[1][26].Code = 102; d->HuffQ1[1][26].Length = 8;

    /********************** 5-step quantizer **************************/
    /********************* 2 bundled samples **************************/
    //less shaped, book 0
    d->HuffQ2[0][ 0].Code =  89; d->HuffQ2[0][ 0].Length = 7;
    d->HuffQ2[0][ 1].Code =  47; d->HuffQ2[0][ 1].Length = 6;
    d->HuffQ2[0][ 2].Code =  15; d->HuffQ2[0][ 2].Length = 5;
    d->HuffQ2[0][ 3].Code =   0; d->HuffQ2[0][ 3].Length = 5;
    d->HuffQ2[0][ 4].Code =  91; d->HuffQ2[0][ 4].Length = 7;
    d->HuffQ2[0][ 5].Code =   4; d->HuffQ2[0][ 5].Length = 5;
    d->HuffQ2[0][ 6].Code =   6; d->HuffQ2[0][ 6].Length = 4;
    d->HuffQ2[0][ 7].Code =  13; d->HuffQ2[0][ 7].Length = 4;
    d->HuffQ2[0][ 8].Code =   4; d->HuffQ2[0][ 8].Length = 4;
    d->HuffQ2[0][ 9].Code =   5; d->HuffQ2[0][ 9].Length = 5;
    d->HuffQ2[0][10].Code =  20; d->HuffQ2[0][10].Length = 5;
    d->HuffQ2[0][11].Code =  12; d->HuffQ2[0][11].Length = 4;
    d->HuffQ2[0][12].Code =   4; d->HuffQ2[0][12].Length = 3;
    d->HuffQ2[0][13].Code =  15; d->HuffQ2[0][13].Length = 4;
    d->HuffQ2[0][14].Code =  14; d->HuffQ2[0][14].Length = 5;
    d->HuffQ2[0][15].Code =   3; d->HuffQ2[0][15].Length = 5;
    d->HuffQ2[0][16].Code =   3; d->HuffQ2[0][16].Length = 4;
    d->HuffQ2[0][17].Code =  14; d->HuffQ2[0][17].Length = 4;
    d->HuffQ2[0][18].Code =   5; d->HuffQ2[0][18].Length = 4;
    d->HuffQ2[0][19].Code =   1; d->HuffQ2[0][19].Length = 5;
    d->HuffQ2[0][20].Code =  90; d->HuffQ2[0][20].Length = 7;
    d->HuffQ2[0][21].Code =   2; d->HuffQ2[0][21].Length = 5;
    d->HuffQ2[0][22].Code =  21; d->HuffQ2[0][22].Length = 5;
    d->HuffQ2[0][23].Code =  46; d->HuffQ2[0][23].Length = 6;
    d->HuffQ2[0][24].Code =  88; d->HuffQ2[0][24].Length = 7;

    //more shaped, book 1
    d->HuffQ2[1][ 0].Code =  921; d->HuffQ2[1][ 0].Length = 10;
    d->HuffQ2[1][ 1].Code =  113; d->HuffQ2[1][ 1].Length =  7;
    d->HuffQ2[1][ 2].Code =   51; d->HuffQ2[1][ 2].Length =  6;
    d->HuffQ2[1][ 3].Code =  231; d->HuffQ2[1][ 3].Length =  8;
    d->HuffQ2[1][ 4].Code =  922; d->HuffQ2[1][ 4].Length = 10;
    d->HuffQ2[1][ 5].Code =  104; d->HuffQ2[1][ 5].Length =  7;
    d->HuffQ2[1][ 6].Code =   30; d->HuffQ2[1][ 6].Length =  5;
    d->HuffQ2[1][ 7].Code =    0; d->HuffQ2[1][ 7].Length =  3;
    d->HuffQ2[1][ 8].Code =   29; d->HuffQ2[1][ 8].Length =  5;
    d->HuffQ2[1][ 9].Code =  105; d->HuffQ2[1][ 9].Length =  7;
    d->HuffQ2[1][10].Code =   50; d->HuffQ2[1][10].Length =  6;
    d->HuffQ2[1][11].Code =    1; d->HuffQ2[1][11].Length =  3;
    d->HuffQ2[1][12].Code =    2; d->HuffQ2[1][12].Length =  2;
    d->HuffQ2[1][13].Code =    3; d->HuffQ2[1][13].Length =  3;
    d->HuffQ2[1][14].Code =   49; d->HuffQ2[1][14].Length =  6;
    d->HuffQ2[1][15].Code =  107; d->HuffQ2[1][15].Length =  7;
    d->HuffQ2[1][16].Code =   27; d->HuffQ2[1][16].Length =  5;
    d->HuffQ2[1][17].Code =    2; d->HuffQ2[1][17].Length =  3;
    d->HuffQ2[1][18].Code =   31; d->HuffQ2[1][18].Length =  5;
    d->HuffQ2[1][19].Code =  112; d->HuffQ2[1][19].Length =  7;
    d->HuffQ2[1][20].Code =  920; d->HuffQ2[1][20].Length = 10;
    d->HuffQ2[1][21].Code =  106; d->HuffQ2[1][21].Length =  7;
    d->HuffQ2[1][22].Code =   48; d->HuffQ2[1][22].Length =  6;
    d->HuffQ2[1][23].Code =  114; d->HuffQ2[1][23].Length =  7;
    d->HuffQ2[1][24].Code =  923; d->HuffQ2[1][24].Length = 10;

    /********************** 7-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    d->HuffQ3[0][0].Code = 12; d->HuffQ3[0][0].Length = 4;
    d->HuffQ3[0][1].Code =  4; d->HuffQ3[0][1].Length = 3;
    d->HuffQ3[0][2].Code =  0; d->HuffQ3[0][2].Length = 2;
    d->HuffQ3[0][3].Code =  1; d->HuffQ3[0][3].Length = 2;
    d->HuffQ3[0][4].Code =  7; d->HuffQ3[0][4].Length = 3;
    d->HuffQ3[0][5].Code =  5; d->HuffQ3[0][5].Length = 3;
    d->HuffQ3[0][6].Code = 13; d->HuffQ3[0][6].Length = 4;

    //more shaped, book 1
    d->HuffQ3[1][0].Code = 4; d->HuffQ3[1][0].Length = 5;
    d->HuffQ3[1][1].Code = 3; d->HuffQ3[1][1].Length = 4;
    d->HuffQ3[1][2].Code = 2; d->HuffQ3[1][2].Length = 2;
    d->HuffQ3[1][3].Code = 3; d->HuffQ3[1][3].Length = 2;
    d->HuffQ3[1][4].Code = 1; d->HuffQ3[1][4].Length = 2;
    d->HuffQ3[1][5].Code = 0; d->HuffQ3[1][5].Length = 3;
    d->HuffQ3[1][6].Code = 5; d->HuffQ3[1][6].Length = 5;

    /********************** 9-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    d->HuffQ4[0][0].Code = 5; d->HuffQ4[0][0].Length = 4;
    d->HuffQ4[0][1].Code = 0; d->HuffQ4[0][1].Length = 3;
    d->HuffQ4[0][2].Code = 4; d->HuffQ4[0][2].Length = 3;
    d->HuffQ4[0][3].Code = 6; d->HuffQ4[0][3].Length = 3;
    d->HuffQ4[0][4].Code = 7; d->HuffQ4[0][4].Length = 3;
    d->HuffQ4[0][5].Code = 5; d->HuffQ4[0][5].Length = 3;
    d->HuffQ4[0][6].Code = 3; d->HuffQ4[0][6].Length = 3;
    d->HuffQ4[0][7].Code = 1; d->HuffQ4[0][7].Length = 3;
    d->HuffQ4[0][8].Code = 4; d->HuffQ4[0][8].Length = 4;

    //more shaped, book 1
    d->HuffQ4[1][0].Code =  9; d->HuffQ4[1][0].Length = 5;
    d->HuffQ4[1][1].Code = 12; d->HuffQ4[1][1].Length = 4;
    d->HuffQ4[1][2].Code =  3; d->HuffQ4[1][2].Length = 3;
    d->HuffQ4[1][3].Code =  0; d->HuffQ4[1][3].Length = 2;
    d->HuffQ4[1][4].Code =  2; d->HuffQ4[1][4].Length = 2;
    d->HuffQ4[1][5].Code =  7; d->HuffQ4[1][5].Length = 3;
    d->HuffQ4[1][6].Code = 13; d->HuffQ4[1][6].Length = 4;
    d->HuffQ4[1][7].Code =  5; d->HuffQ4[1][7].Length = 4;
    d->HuffQ4[1][8].Code =  8; d->HuffQ4[1][8].Length = 5;

    /********************* 15-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    d->HuffQ5[0][ 0].Code = 57; d->HuffQ5[0][ 0].Length = 6;
    d->HuffQ5[0][ 1].Code = 23; d->HuffQ5[0][ 1].Length = 5;
    d->HuffQ5[0][ 2].Code =  8; d->HuffQ5[0][ 2].Length = 4;
    d->HuffQ5[0][ 3].Code = 10; d->HuffQ5[0][ 3].Length = 4;
    d->HuffQ5[0][ 4].Code = 13; d->HuffQ5[0][ 4].Length = 4;
    d->HuffQ5[0][ 5].Code =  0; d->HuffQ5[0][ 5].Length = 3;
    d->HuffQ5[0][ 6].Code =  2; d->HuffQ5[0][ 6].Length = 3;
    d->HuffQ5[0][ 7].Code =  3; d->HuffQ5[0][ 7].Length = 3;
    d->HuffQ5[0][ 8].Code =  1; d->HuffQ5[0][ 8].Length = 3;
    d->HuffQ5[0][ 9].Code = 15; d->HuffQ5[0][ 9].Length = 4;
    d->HuffQ5[0][10].Code = 12; d->HuffQ5[0][10].Length = 4;
    d->HuffQ5[0][11].Code =  9; d->HuffQ5[0][11].Length = 4;
    d->HuffQ5[0][12].Code = 29; d->HuffQ5[0][12].Length = 5;
    d->HuffQ5[0][13].Code = 22; d->HuffQ5[0][13].Length = 5;
    d->HuffQ5[0][14].Code = 56; d->HuffQ5[0][14].Length = 6;

    //more shaped, book 1
    d->HuffQ5[1][ 0].Code = 229; d->HuffQ5[1][ 0].Length = 8;
    d->HuffQ5[1][ 1].Code =  56; d->HuffQ5[1][ 1].Length = 6;
    d->HuffQ5[1][ 2].Code =   7; d->HuffQ5[1][ 2].Length = 5;
    d->HuffQ5[1][ 3].Code =   2; d->HuffQ5[1][ 3].Length = 4;
    d->HuffQ5[1][ 4].Code =   0; d->HuffQ5[1][ 4].Length = 3;
    d->HuffQ5[1][ 5].Code =   3; d->HuffQ5[1][ 5].Length = 3;
    d->HuffQ5[1][ 6].Code =   5; d->HuffQ5[1][ 6].Length = 3;
    d->HuffQ5[1][ 7].Code =   6; d->HuffQ5[1][ 7].Length = 3;
    d->HuffQ5[1][ 8].Code =   4; d->HuffQ5[1][ 8].Length = 3;
    d->HuffQ5[1][ 9].Code =   2; d->HuffQ5[1][ 9].Length = 3;
    d->HuffQ5[1][10].Code =  15; d->HuffQ5[1][10].Length = 4;
    d->HuffQ5[1][11].Code =  29; d->HuffQ5[1][11].Length = 5;
    d->HuffQ5[1][12].Code =   6; d->HuffQ5[1][12].Length = 5;
    d->HuffQ5[1][13].Code = 115; d->HuffQ5[1][13].Length = 7;
    d->HuffQ5[1][14].Code = 228; d->HuffQ5[1][14].Length = 8;

    /********************* 31-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    d->HuffQ6[0][ 0].Code =  65; d->HuffQ6[0][ 0].Length = 7;
    d->HuffQ6[0][ 1].Code =   6; d->HuffQ6[0][ 1].Length = 6;
    d->HuffQ6[0][ 2].Code =  44; d->HuffQ6[0][ 2].Length = 6;
    d->HuffQ6[0][ 3].Code =  45; d->HuffQ6[0][ 3].Length = 6;
    d->HuffQ6[0][ 4].Code =  59; d->HuffQ6[0][ 4].Length = 6;
    d->HuffQ6[0][ 5].Code =  13; d->HuffQ6[0][ 5].Length = 5;
    d->HuffQ6[0][ 6].Code =  17; d->HuffQ6[0][ 6].Length = 5;
    d->HuffQ6[0][ 7].Code =  19; d->HuffQ6[0][ 7].Length = 5;
    d->HuffQ6[0][ 8].Code =  23; d->HuffQ6[0][ 8].Length = 5;
    d->HuffQ6[0][ 9].Code =  21; d->HuffQ6[0][ 9].Length = 5;
    d->HuffQ6[0][10].Code =  26; d->HuffQ6[0][10].Length = 5;
    d->HuffQ6[0][11].Code =  30; d->HuffQ6[0][11].Length = 5;
    d->HuffQ6[0][12].Code =   0; d->HuffQ6[0][12].Length = 4;
    d->HuffQ6[0][13].Code =   2; d->HuffQ6[0][13].Length = 4;
    d->HuffQ6[0][14].Code =   5; d->HuffQ6[0][14].Length = 4;
    d->HuffQ6[0][15].Code =   7; d->HuffQ6[0][15].Length = 4;
    d->HuffQ6[0][16].Code =   3; d->HuffQ6[0][16].Length = 4;
    d->HuffQ6[0][17].Code =   4; d->HuffQ6[0][17].Length = 4;
    d->HuffQ6[0][18].Code =  31; d->HuffQ6[0][18].Length = 5;
    d->HuffQ6[0][19].Code =  28; d->HuffQ6[0][19].Length = 5;
    d->HuffQ6[0][20].Code =  25; d->HuffQ6[0][20].Length = 5;
    d->HuffQ6[0][21].Code =  27; d->HuffQ6[0][21].Length = 5;
    d->HuffQ6[0][22].Code =  24; d->HuffQ6[0][22].Length = 5;
    d->HuffQ6[0][23].Code =  20; d->HuffQ6[0][23].Length = 5;
    d->HuffQ6[0][24].Code =  18; d->HuffQ6[0][24].Length = 5;
    d->HuffQ6[0][25].Code =  12; d->HuffQ6[0][25].Length = 5;
    d->HuffQ6[0][26].Code =   2; d->HuffQ6[0][26].Length = 5;
    d->HuffQ6[0][27].Code =  58; d->HuffQ6[0][27].Length = 6;
    d->HuffQ6[0][28].Code =  33; d->HuffQ6[0][28].Length = 6;
    d->HuffQ6[0][29].Code =   7; d->HuffQ6[0][29].Length = 6;
    d->HuffQ6[0][30].Code =  64; d->HuffQ6[0][30].Length = 7;

    //more shaped, book 1
    d->HuffQ6[1][ 0].Code = 6472; d->HuffQ6[1][ 0].Length = 13;
    d->HuffQ6[1][ 1].Code = 6474; d->HuffQ6[1][ 1].Length = 13;
    d->HuffQ6[1][ 2].Code =  808; d->HuffQ6[1][ 2].Length = 10;
    d->HuffQ6[1][ 3].Code =  405; d->HuffQ6[1][ 3].Length =  9;
    d->HuffQ6[1][ 4].Code =  203; d->HuffQ6[1][ 4].Length =  8;
    d->HuffQ6[1][ 5].Code =  102; d->HuffQ6[1][ 5].Length =  7;
    d->HuffQ6[1][ 6].Code =   49; d->HuffQ6[1][ 6].Length =  6;
    d->HuffQ6[1][ 7].Code =    9; d->HuffQ6[1][ 7].Length =  5;
    d->HuffQ6[1][ 8].Code =   15; d->HuffQ6[1][ 8].Length =  5;
    d->HuffQ6[1][ 9].Code =   31; d->HuffQ6[1][ 9].Length =  5;
    d->HuffQ6[1][10].Code =    2; d->HuffQ6[1][10].Length =  4;
    d->HuffQ6[1][11].Code =    6; d->HuffQ6[1][11].Length =  4;
    d->HuffQ6[1][12].Code =    8; d->HuffQ6[1][12].Length =  4;
    d->HuffQ6[1][13].Code =   11; d->HuffQ6[1][13].Length =  4;
    d->HuffQ6[1][14].Code =   13; d->HuffQ6[1][14].Length =  4;
    d->HuffQ6[1][15].Code =    0; d->HuffQ6[1][15].Length =  3;
    d->HuffQ6[1][16].Code =   14; d->HuffQ6[1][16].Length =  4;
    d->HuffQ6[1][17].Code =   10; d->HuffQ6[1][17].Length =  4;
    d->HuffQ6[1][18].Code =    9; d->HuffQ6[1][18].Length =  4;
    d->HuffQ6[1][19].Code =    5; d->HuffQ6[1][19].Length =  4;
    d->HuffQ6[1][20].Code =    3; d->HuffQ6[1][20].Length =  4;
    d->HuffQ6[1][21].Code =   30; d->HuffQ6[1][21].Length =  5;
    d->HuffQ6[1][22].Code =   14; d->HuffQ6[1][22].Length =  5;
    d->HuffQ6[1][23].Code =    8; d->HuffQ6[1][23].Length =  5;
    d->HuffQ6[1][24].Code =   48; d->HuffQ6[1][24].Length =  6;
    d->HuffQ6[1][25].Code =  103; d->HuffQ6[1][25].Length =  7;
    d->HuffQ6[1][26].Code =  201; d->HuffQ6[1][26].Length =  8;
    d->HuffQ6[1][27].Code =  200; d->HuffQ6[1][27].Length =  8;
    d->HuffQ6[1][28].Code = 1619; d->HuffQ6[1][28].Length = 11;
    d->HuffQ6[1][29].Code = 6473; d->HuffQ6[1][29].Length = 13;
    d->HuffQ6[1][30].Code = 6475; d->HuffQ6[1][30].Length = 13;

    /********************* 63-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    d->HuffQ7[0][ 0].Code = 103; d->HuffQ7[0][ 0].Length = 8;    /* 0.003338  -          01100111  */
    d->HuffQ7[0][ 1].Code = 153; d->HuffQ7[0][ 1].Length = 8;    /* 0.003766  -          10011001  */
    d->HuffQ7[0][ 2].Code = 181; d->HuffQ7[0][ 2].Length = 8;    /* 0.004715  -          10110101  */
    d->HuffQ7[0][ 3].Code = 233; d->HuffQ7[0][ 3].Length = 8;    /* 0.005528  -          11101001  */
    d->HuffQ7[0][ 4].Code =  64; d->HuffQ7[0][ 4].Length = 7;    /* 0.006677  -           1000000  */
    d->HuffQ7[0][ 5].Code =  65; d->HuffQ7[0][ 5].Length = 7;    /* 0.007041  -           1000001  */
    d->HuffQ7[0][ 6].Code =  77; d->HuffQ7[0][ 6].Length = 7;    /* 0.007733  -           1001101  */
    d->HuffQ7[0][ 7].Code =  81; d->HuffQ7[0][ 7].Length = 7;    /* 0.008296  -           1010001  */
    d->HuffQ7[0][ 8].Code =  91; d->HuffQ7[0][ 8].Length = 7;    /* 0.009295  -           1011011  */
    d->HuffQ7[0][ 9].Code = 113; d->HuffQ7[0][ 9].Length = 7;    /* 0.010814  -           1110001  */
    d->HuffQ7[0][10].Code = 112; d->HuffQ7[0][10].Length = 7;    /* 0.010807  -           1110000  */
    d->HuffQ7[0][11].Code =  24; d->HuffQ7[0][11].Length = 6;    /* 0.012748  -            011000  */
    d->HuffQ7[0][12].Code =  29; d->HuffQ7[0][12].Length = 6;    /* 0.013390  -            011101  */
    d->HuffQ7[0][13].Code =  35; d->HuffQ7[0][13].Length = 6;    /* 0.014224  -            100011  */
    d->HuffQ7[0][14].Code =  37; d->HuffQ7[0][14].Length = 6;    /* 0.015201  -            100101  */
    d->HuffQ7[0][15].Code =  41; d->HuffQ7[0][15].Length = 6;    /* 0.016642  -            101001  */
    d->HuffQ7[0][16].Code =  44; d->HuffQ7[0][16].Length = 6;    /* 0.017292  -            101100  */
    d->HuffQ7[0][17].Code =  46; d->HuffQ7[0][17].Length = 6;    /* 0.018647  -            101110  */
    d->HuffQ7[0][18].Code =  51; d->HuffQ7[0][18].Length = 6;    /* 0.020473  -            110011  */
    d->HuffQ7[0][19].Code =  49; d->HuffQ7[0][19].Length = 6;    /* 0.020152  -            110001  */
    d->HuffQ7[0][20].Code =  54; d->HuffQ7[0][20].Length = 6;    /* 0.021315  -            110110  */
    d->HuffQ7[0][21].Code =  55; d->HuffQ7[0][21].Length = 6;    /* 0.021358  -            110111  */
    d->HuffQ7[0][22].Code =  57; d->HuffQ7[0][22].Length = 6;    /* 0.021700  -            111001  */
    d->HuffQ7[0][23].Code =  60; d->HuffQ7[0][23].Length = 6;    /* 0.022449  -            111100  */
    d->HuffQ7[0][24].Code =   0; d->HuffQ7[0][24].Length = 5;    /* 0.023063  -             00000  */
    d->HuffQ7[0][25].Code =   2; d->HuffQ7[0][25].Length = 5;    /* 0.023854  -             00010  */
    d->HuffQ7[0][26].Code =  10; d->HuffQ7[0][26].Length = 5;    /* 0.025481  -             01010  */
    d->HuffQ7[0][27].Code =   5; d->HuffQ7[0][27].Length = 5;    /* 0.024867  -             00101  */
    d->HuffQ7[0][28].Code =   9; d->HuffQ7[0][28].Length = 5;    /* 0.025352  -             01001  */
    d->HuffQ7[0][29].Code =   6; d->HuffQ7[0][29].Length = 5;    /* 0.025074  -             00110  */
    d->HuffQ7[0][30].Code =  13; d->HuffQ7[0][30].Length = 5;    /* 0.025745  -             01101  */
    d->HuffQ7[0][31].Code =   7; d->HuffQ7[0][31].Length = 5;    /* 0.025195  -             00111  */
    d->HuffQ7[0][32].Code =  11; d->HuffQ7[0][32].Length = 5;    /* 0.025502  -             01011  */
    d->HuffQ7[0][33].Code =  15; d->HuffQ7[0][33].Length = 5;    /* 0.026251  -             01111  */
    d->HuffQ7[0][34].Code =   8; d->HuffQ7[0][34].Length = 5;    /* 0.025260  -             01000  */
    d->HuffQ7[0][35].Code =   4; d->HuffQ7[0][35].Length = 5;    /* 0.024418  -             00100  */
    d->HuffQ7[0][36].Code =   3; d->HuffQ7[0][36].Length = 5;    /* 0.023983  -             00011  */
    d->HuffQ7[0][37].Code =   1; d->HuffQ7[0][37].Length = 5;    /* 0.023697  -             00001  */
    d->HuffQ7[0][38].Code =  63; d->HuffQ7[0][38].Length = 6;    /* 0.023041  -            111111  */
    d->HuffQ7[0][39].Code =  62; d->HuffQ7[0][39].Length = 6;    /* 0.022656  -            111110  */
    d->HuffQ7[0][40].Code =  61; d->HuffQ7[0][40].Length = 6;    /* 0.022549  -            111101  */
    d->HuffQ7[0][41].Code =  53; d->HuffQ7[0][41].Length = 6;    /* 0.021151  -            110101  */
    d->HuffQ7[0][42].Code =  59; d->HuffQ7[0][42].Length = 6;    /* 0.022042  -            111011  */
    d->HuffQ7[0][43].Code =  52; d->HuffQ7[0][43].Length = 6;    /* 0.020837  -            110100  */
    d->HuffQ7[0][44].Code =  48; d->HuffQ7[0][44].Length = 6;    /* 0.019446  -            110000  */
    d->HuffQ7[0][45].Code =  47; d->HuffQ7[0][45].Length = 6;    /* 0.019189  -            101111  */
    d->HuffQ7[0][46].Code =  43; d->HuffQ7[0][46].Length = 6;    /* 0.017177  -            101011  */
    d->HuffQ7[0][47].Code =  42; d->HuffQ7[0][47].Length = 6;    /* 0.017035  -            101010  */
    d->HuffQ7[0][48].Code =  39; d->HuffQ7[0][48].Length = 6;    /* 0.015287  -            100111  */
    d->HuffQ7[0][49].Code =  36; d->HuffQ7[0][49].Length = 6;    /* 0.014559  -            100100  */
    d->HuffQ7[0][50].Code =  33; d->HuffQ7[0][50].Length = 6;    /* 0.014117  -            100001  */
    d->HuffQ7[0][51].Code =  28; d->HuffQ7[0][51].Length = 6;    /* 0.012776  -            011100  */
    d->HuffQ7[0][52].Code = 117; d->HuffQ7[0][52].Length = 7;    /* 0.011107  -           1110101  */
    d->HuffQ7[0][53].Code = 101; d->HuffQ7[0][53].Length = 7;    /* 0.010636  -           1100101  */
    d->HuffQ7[0][54].Code = 100; d->HuffQ7[0][54].Length = 7;    /* 0.009751  -           1100100  */
    d->HuffQ7[0][55].Code =  80; d->HuffQ7[0][55].Length = 7;    /* 0.008132  -           1010000  */
    d->HuffQ7[0][56].Code =  69; d->HuffQ7[0][56].Length = 7;    /* 0.007091  -           1000101  */
    d->HuffQ7[0][57].Code =  68; d->HuffQ7[0][57].Length = 7;    /* 0.007084  -           1000100  */
    d->HuffQ7[0][58].Code =  50; d->HuffQ7[0][58].Length = 7;    /* 0.006277  -           0110010  */
    d->HuffQ7[0][59].Code = 232; d->HuffQ7[0][59].Length = 8;    /* 0.005386  -          11101000  */
    d->HuffQ7[0][60].Code = 180; d->HuffQ7[0][60].Length = 8;    /* 0.004408  -          10110100  */
    d->HuffQ7[0][61].Code = 152; d->HuffQ7[0][61].Length = 8;    /* 0.003759  -          10011000  */
    d->HuffQ7[0][62].Code = 102; d->HuffQ7[0][62].Length = 8;    /* 0.003160  -          01100110  */

    //more shaped, book 1
    d->HuffQ7[1][ 0].Code = 14244; d->HuffQ7[1][ 0].Length = 14;    /* 0.000059  -        11011110100100  */
    d->HuffQ7[1][ 1].Code = 14253; d->HuffQ7[1][ 1].Length = 14;    /* 0.000098  -        11011110101101  */
    d->HuffQ7[1][ 2].Code = 14246; d->HuffQ7[1][ 2].Length = 14;    /* 0.000078  -        11011110100110  */
    d->HuffQ7[1][ 3].Code = 14254; d->HuffQ7[1][ 3].Length = 14;    /* 0.000111  -        11011110101110  */
    d->HuffQ7[1][ 4].Code =  3562; d->HuffQ7[1][ 4].Length = 12;    /* 0.000320  -          110111101010  */
    d->HuffQ7[1][ 5].Code =   752; d->HuffQ7[1][ 5].Length = 10;    /* 0.000920  -            1011110000  */
    d->HuffQ7[1][ 6].Code =   753; d->HuffQ7[1][ 6].Length = 10;    /* 0.001057  -            1011110001  */
    d->HuffQ7[1][ 7].Code =   160; d->HuffQ7[1][ 7].Length =  9;    /* 0.001403  -             010100000  */
    d->HuffQ7[1][ 8].Code =   162; d->HuffQ7[1][ 8].Length =  9;    /* 0.001579  -             010100010  */
    d->HuffQ7[1][ 9].Code =   444; d->HuffQ7[1][ 9].Length =  9;    /* 0.002486  -             110111100  */
    d->HuffQ7[1][10].Code =   122; d->HuffQ7[1][10].Length =  8;    /* 0.003772  -              01111010  */
    d->HuffQ7[1][11].Code =   223; d->HuffQ7[1][11].Length =  8;    /* 0.005710  -              11011111  */
    d->HuffQ7[1][12].Code =    60; d->HuffQ7[1][12].Length =  7;    /* 0.006858  -               0111100  */
    d->HuffQ7[1][13].Code =    73; d->HuffQ7[1][13].Length =  7;    /* 0.008033  -               1001001  */
    d->HuffQ7[1][14].Code =   110; d->HuffQ7[1][14].Length =  7;    /* 0.009827  -               1101110  */
    d->HuffQ7[1][15].Code =    14; d->HuffQ7[1][15].Length =  6;    /* 0.012601  -                001110  */
    d->HuffQ7[1][16].Code =    24; d->HuffQ7[1][16].Length =  6;    /* 0.013194  -                011000  */
    d->HuffQ7[1][17].Code =    25; d->HuffQ7[1][17].Length =  6;    /* 0.013938  -                011001  */
    d->HuffQ7[1][18].Code =    34; d->HuffQ7[1][18].Length =  6;    /* 0.015693  -                100010  */
    d->HuffQ7[1][19].Code =    37; d->HuffQ7[1][19].Length =  6;    /* 0.017846  -                100101  */
    d->HuffQ7[1][20].Code =    54; d->HuffQ7[1][20].Length =  6;    /* 0.020078  -                110110  */
    d->HuffQ7[1][21].Code =     3; d->HuffQ7[1][21].Length =  5;    /* 0.022975  -                 00011  */
    d->HuffQ7[1][22].Code =     9; d->HuffQ7[1][22].Length =  5;    /* 0.025631  -                 01001  */
    d->HuffQ7[1][23].Code =    11; d->HuffQ7[1][23].Length =  5;    /* 0.027021  -                 01011  */
    d->HuffQ7[1][24].Code =    16; d->HuffQ7[1][24].Length =  5;    /* 0.031465  -                 10000  */
    d->HuffQ7[1][25].Code =    19; d->HuffQ7[1][25].Length =  5;    /* 0.034244  -                 10011  */
    d->HuffQ7[1][26].Code =    21; d->HuffQ7[1][26].Length =  5;    /* 0.035921  -                 10101  */
    d->HuffQ7[1][27].Code =    24; d->HuffQ7[1][27].Length =  5;    /* 0.037938  -                 11000  */
    d->HuffQ7[1][28].Code =    26; d->HuffQ7[1][28].Length =  5;    /* 0.039595  -                 11010  */
    d->HuffQ7[1][29].Code =    29; d->HuffQ7[1][29].Length =  5;    /* 0.041546  -                 11101  */
    d->HuffQ7[1][30].Code =    31; d->HuffQ7[1][30].Length =  5;    /* 0.042623  -                 11111  */
    d->HuffQ7[1][31].Code =     2; d->HuffQ7[1][31].Length =  4;    /* 0.045180  -                  0010  */
    d->HuffQ7[1][32].Code =     0; d->HuffQ7[1][32].Length =  4;    /* 0.043151  -                  0000  */
    d->HuffQ7[1][33].Code =    30; d->HuffQ7[1][33].Length =  5;    /* 0.042538  -                 11110  */
    d->HuffQ7[1][34].Code =    28; d->HuffQ7[1][34].Length =  5;    /* 0.041422  -                 11100  */
    d->HuffQ7[1][35].Code =    25; d->HuffQ7[1][35].Length =  5;    /* 0.039145  -                 11001  */
    d->HuffQ7[1][36].Code =    22; d->HuffQ7[1][36].Length =  5;    /* 0.036691  -                 10110  */
    d->HuffQ7[1][37].Code =    20; d->HuffQ7[1][37].Length =  5;    /* 0.034955  -                 10100  */
    d->HuffQ7[1][38].Code =    14; d->HuffQ7[1][38].Length =  5;    /* 0.029155  -                 01110  */
    d->HuffQ7[1][39].Code =    13; d->HuffQ7[1][39].Length =  5;    /* 0.027921  -                 01101  */
    d->HuffQ7[1][40].Code =     8; d->HuffQ7[1][40].Length =  5;    /* 0.025553  -                 01000  */
    d->HuffQ7[1][41].Code =     6; d->HuffQ7[1][41].Length =  5;    /* 0.023093  -                 00110  */
    d->HuffQ7[1][42].Code =     2; d->HuffQ7[1][42].Length =  5;    /* 0.021200  -                 00010  */
    d->HuffQ7[1][43].Code =    46; d->HuffQ7[1][43].Length =  6;    /* 0.018134  -                101110  */
    d->HuffQ7[1][44].Code =    35; d->HuffQ7[1][44].Length =  6;    /* 0.015824  -                100011  */
    d->HuffQ7[1][45].Code =    31; d->HuffQ7[1][45].Length =  6;    /* 0.014701  -                011111  */
    d->HuffQ7[1][46].Code =    21; d->HuffQ7[1][46].Length =  6;    /* 0.013187  -                010101  */
    d->HuffQ7[1][47].Code =    15; d->HuffQ7[1][47].Length =  6;    /* 0.012776  -                001111  */
    d->HuffQ7[1][48].Code =    95; d->HuffQ7[1][48].Length =  7;    /* 0.009664  -               1011111  */
    d->HuffQ7[1][49].Code =    72; d->HuffQ7[1][49].Length =  7;    /* 0.007922  -               1001000  */
    d->HuffQ7[1][50].Code =    41; d->HuffQ7[1][50].Length =  7;    /* 0.006838  -               0101001  */
    d->HuffQ7[1][51].Code =   189; d->HuffQ7[1][51].Length =  8;    /* 0.005024  -              10111101  */
    d->HuffQ7[1][52].Code =   123; d->HuffQ7[1][52].Length =  8;    /* 0.003830  -              01111011  */
    d->HuffQ7[1][53].Code =   377; d->HuffQ7[1][53].Length =  9;    /* 0.002232  -             101111001  */
    d->HuffQ7[1][54].Code =   161; d->HuffQ7[1][54].Length =  9;    /* 0.001566  -             010100001  */
    d->HuffQ7[1][55].Code =   891; d->HuffQ7[1][55].Length = 10;    /* 0.001383  -            1101111011  */
    d->HuffQ7[1][56].Code =   327; d->HuffQ7[1][56].Length = 10;    /* 0.000900  -            0101000111  */
    d->HuffQ7[1][57].Code =   326; d->HuffQ7[1][57].Length = 10;    /* 0.000790  -            0101000110  */
    d->HuffQ7[1][58].Code =  3560; d->HuffQ7[1][58].Length = 12;    /* 0.000254  -          110111101000  */
    d->HuffQ7[1][59].Code = 14255; d->HuffQ7[1][59].Length = 14;    /* 0.000117  -        11011110101111  */
    d->HuffQ7[1][60].Code = 14247; d->HuffQ7[1][60].Length = 14;    /* 0.000085  -        11011110100111  */
    d->HuffQ7[1][61].Code = 14252; d->HuffQ7[1][61].Length = 14;    /* 0.000085  -        11011110101100  */
    d->HuffQ7[1][62].Code = 14245; d->HuffQ7[1][62].Length = 14;    /* 0.000065  -        11011110100101  */
}
