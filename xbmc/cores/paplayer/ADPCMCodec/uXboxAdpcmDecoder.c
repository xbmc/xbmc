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

#include "uXboxAdpcmDecoder.h"

#define TXboxAdpcmDecoder_delimit(x,h,l)    \
                    if(x > h) {             \
                        x = h;              \
                    } else if(x < l) {      \
                        x = l;              \
                    }



typedef struct {
    int8_t  Index;
    int16_t StepSize;
    int16_t Predictor;
} TAdpcmState;



const static int16_t StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};



const static int8_t IndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};



int TXboxAdpcmDecoder_DecodeSample(int Code, TAdpcmState *State) {
    int     Delta,
            Result;

    Delta = State->StepSize >> 3;
    if(Code & 4) Delta += State->StepSize;
    if(Code & 2) Delta += State->StepSize >> 1;
    if(Code & 1) Delta += State->StepSize >> 2;
    if(Code & 8) Delta = -Delta;
    Result           = State->Predictor + Delta;
    TXboxAdpcmDecoder_delimit(Result, 32767, -32768);
    State->Index    += IndexTable[Code];
    TXboxAdpcmDecoder_delimit(State->Index, 88, 0);
    State->StepSize  = StepTable[State->Index];
    State->Predictor = Result;
    return(Result);
}



int TXboxAdpcmDecoder_Decode_Memory(uint8_t *in, int inlen, uint8_t *out, int FChannels) {
    TAdpcmState FAdpcmState[6];
    int16_t     Buffers[6][8];
    uint32_t    CodeBuf;
    int         i,
                j,
                c,
                outlen;

    inlen = (inlen / XBOX_ADPCM_SRCSIZE) / FChannels;

    for(outlen = 0; inlen--; outlen++) {
        for(c = 0; c < FChannels; c++) {
            *out++ = in[0];
            *out++ = in[1];
            FAdpcmState[c].Predictor = in[0] | (in[1] << 8);    in  += 2;
            FAdpcmState[c].Index     = in[0] | (in[1] << 8);    in  += 2;
            TXboxAdpcmDecoder_delimit(FAdpcmState[c].Index, 88, 0);
            FAdpcmState[c].StepSize  = StepTable[FAdpcmState[c].Index];
        }
        for(i = 0; i < 8; i++) {
            for(c = 0; c < FChannels; c++) {
                CodeBuf = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
                in += 4;
                for(j = 0; j < 8; j++) {
                    Buffers[c][j] = TXboxAdpcmDecoder_DecodeSample(CodeBuf & 15, &FAdpcmState[c]);
                    CodeBuf >>= 4;
                }
            }
            for(j = 0; j < 8; j++) {
                for(c = 0; c < FChannels; c++) {
                    *out++ = (Buffers[c][j]     ) & 0xff;
                    *out++ = (Buffers[c][j] >> 8) & 0xff;
                }
            }
        }
    }

    return(outlen * XBOX_ADPCM_DSTSIZE * FChannels);
}



int TXboxAdpcmDecoder_Decode(FILE *ASource, FILE *ADest, int SourcePos, int SourceSize, int FChannels) {
    int     DestSize;
    uint8_t in[XBOX_ADPCM_SRCSIZE  * 6],
            out[XBOX_ADPCM_DSTSIZE * 6];

    if(FChannels <= 0) return(0);

    if(SourcePos >= 0) {
        if(fseek(ASource, SourcePos, SEEK_SET) < 0) return(0);
    }
    if(SourceSize > 0) {
        SourceSize -= (SourceSize % XBOX_ADPCM_SRCSIZE);
        SourceSize  = (SourceSize / XBOX_ADPCM_SRCSIZE) / FChannels;
    }

    for(DestSize = 0; SourceSize; SourceSize--) {
        if(!fread(in,   sizeof(in),  1, ASource)) break;
        DestSize += TXboxAdpcmDecoder_Decode_Memory(in, sizeof(in), out, FChannels);
        if(!fwrite(out, sizeof(out), 1, ADest))   break;
    }

    return(DestSize);
}



int TXboxAdpcmDecoder_guess_output_size(int SourceSize) {
    return((SourceSize / XBOX_ADPCM_SRCSIZE) * XBOX_ADPCM_DSTSIZE);
}

