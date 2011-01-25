/*
  TXboxAdpcmDecoder class
  (c) 2005 Benjamin Haisch

  Revision 2 with stereo support

###
  C conversion 0.1.3
  by Luigi Auriemma
  e-mail: aluigi@autistici.org
  web:    aluigi.org

    Copyright 2005,2006 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt
*/

#include <stdio.h>

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;

#define XBOX_ADPCM_SRCSIZE  36
#define XBOX_ADPCM_DSTSIZE  130

int TXboxAdpcmDecoder_Decode_Memory(
    uint8_t *in,        // input buffer
    int     inlen,      // input size MUST BE a multiplier of XBOX_ADPCM_SRCSIZE
    uint8_t *out,       // output buffer
    int     FChannels   // channels
);

int TXboxAdpcmDecoder_Decode(
    FILE    *ASource,   // input file
    FILE    *ADest,     // output file
    int     SourcePos,  // fseek offset, use -1 for none
    int     SourceSize, // size of the input audio block, use -1 for all
    int     AChannels   // number of channels, usually 2
);


int TXboxAdpcmDecoder_guess_output_size(
    int     SourceSize  // size of the input audio block
);


