/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// MP3 internal implementation details
// Implementation

#include "MP3InternalsHuffman.hh"

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// This is crufty old code that needs to be cleaned up #####

static unsigned live_tabsel[2][3][16] = {
   { {32,32,64,96,128,160,192,224,256,288,320,352,384,416,448,448},
     {32,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,384},
     {32,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,320} },

   { {32,32,48,56,64,80,96,112,128,144,160,176,192,224,256,256},
     {8,8,16,24,32,40,48,56,64,80,96,112,128,144,160,160},
     {8,8,16,24,32,40,48,56,64,80,96,112,128,144,160,160} }
};
/* Note: live_tabsel[*][*][0 or 15] shouldn't occur; use dummy values there */

static long live_freqs[]
= { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000, 0 };

struct bandInfoStruct {
  int longIdx[23];
  int longDiff[22];
  int shortIdx[14];
  int shortDiff[13];
};

static struct bandInfoStruct const bandInfo[7] = {
/* MPEG 1.0 */
 { {0,4,8,12,16,20,24,30,36,44,52,62,74, 90,110,134,162,196,238,288,342,418,576},
   {4,4,4,4,4,4,6,6,8, 8,10,12,16,20,24,28,34,42,50,54, 76,158},
   {0,4*3,8*3,12*3,16*3,22*3,30*3,40*3,52*3,66*3, 84*3,106*3,136*3,192*3},
   {4,4,4,4,6,8,10,12,14,18,22,30,56} } ,

 { {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,384,576},
   {4,4,4,4,4,4,6,6,6, 8,10,12,16,18,22,28,34,40,46,54, 54,192},
   {0,4*3,8*3,12*3,16*3,22*3,28*3,38*3,50*3,64*3, 80*3,100*3,126*3,192*3},
   {4,4,4,4,6,6,10,12,14,16,20,26,66} } ,

 { {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576} ,
   {4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102, 26} ,
   {0,4*3,8*3,12*3,16*3,22*3,30*3,42*3,58*3,78*3,104*3,138*3,180*3,192*3} ,
   {4,4,4,4,6,8,12,16,20,26,34,42,12} }  ,

/* MPEG 2.0 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 } ,
   {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3} ,
   {4,4,4,6,6,8,10,14,18,26,32,42,18 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,330,394,464,540,576},
   {6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,52,64,70,76,36 } ,
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,136*3,180*3,192*3} ,
   {4,4,4,6,8,10,12,14,18,24,32,44,12 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,134*3,174*3,192*3},
   {4,4,4,6,8,10,12,14,18,24,30,40,18 } } ,

/* MPEG 2.5, wrong! table (it's just a copy of MPEG 2.0/44.1kHz) */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 } ,
   {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3} ,
   {4,4,4,6,6,8,10,14,18,26,32,42,18 } } ,
};

unsigned int n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
unsigned int i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

#define MPG_MD_MONO 3


////////// MP3FrameParams //////////

MP3FrameParams::MP3FrameParams()
  : bv(frameBytes, 0, sizeof frameBytes) /* by default */ {
  oldHdr = firstHdr = 0;

  static Boolean doneInit = False;
  if (doneInit) return;
  doneInit = True;

  int i,j,k,l;

  for (i=0;i<5;i++) {
    for (j=0;j<6;j++) {
      for (k=0;k<6;k++) {
        int n = k + j * 6 + i * 36;
        i_slen2[n] = i|(j<<3)|(k<<6)|(3<<12);
      }
    }
  }
  for (i=0;i<4;i++) {
    for (j=0;j<4;j++) {
      for (k=0;k<4;k++) {
        int n = k + j * 4 + i * 16;
        i_slen2[n+180] = i|(j<<3)|(k<<6)|(4<<12);
      }
    }
  }
  for (i=0;i<4;i++) {
    for (j=0;j<3;j++) {
      int n = j + i * 3;
      i_slen2[n+244] = i|(j<<3) | (5<<12);
      n_slen2[n+500] = i|(j<<3) | (2<<12) | (1<<15);
    }
  }

  for (i=0;i<5;i++) {
    for (j=0;j<5;j++) {
      for (k=0;k<4;k++) {
        for (l=0;l<4;l++) {
          int n = l + k * 4 + j * 16 + i * 80;
          n_slen2[n] = i|(j<<3)|(k<<6)|(l<<9)|(0<<12);
        }
      }
    }
  }
  for (i=0;i<5;i++) {
    for (j=0;j<5;j++) {
      for (k=0;k<4;k++) {
        int n = k + j * 4 + i * 20;
        n_slen2[n+400] = i|(j<<3)|(k<<6)|(1<<12);
      }
    }
  }
}

MP3FrameParams::~MP3FrameParams() {
}

void MP3FrameParams::setParamsFromHeader() {
  if (hdr & (1<<20)) {
    isMPEG2 = (hdr & (1<<19)) ? 0x0 : 0x1;
    isMPEG2_5 = 0;
  }
  else {
    isMPEG2 = 1;
    isMPEG2_5 = 1;
  }

  layer = 4-((hdr>>17)&3);
  if (layer == 4) layer = 3; // layer==4 is not allowed
  bitrateIndex = ((hdr>>12)&0xf);

  if (isMPEG2_5) {
    samplingFreqIndex = ((hdr>>10)&0x3) + 6;
  } else {
    samplingFreqIndex = ((hdr>>10)&0x3) + (isMPEG2*3);
  }

  hasCRC = ((hdr>>16)&0x1)^0x1;

  padding   = ((hdr>>9)&0x1);
  extension = ((hdr>>8)&0x1);
  mode      = ((hdr>>6)&0x3);
  mode_ext  = ((hdr>>4)&0x3);
  copyright = ((hdr>>3)&0x1);
  original  = ((hdr>>2)&0x1);
  emphasis  = hdr & 0x3;

  stereo    = (mode == MPG_MD_MONO) ? 1 : 2;

  if (((hdr>>10)&0x3) == 0x3) {
#ifdef DEBUG_ERRORS
    fprintf(stderr,"Stream error - hdr: 0x%08x\n", hdr);
#endif
  }

  bitrate = live_tabsel[isMPEG2][layer-1][bitrateIndex];
  samplingFreq = live_freqs[samplingFreqIndex];
  isStereo = (stereo > 1);
  isFreeFormat = (bitrateIndex == 0);
  frameSize
    = ComputeFrameSize(bitrate, samplingFreq, padding, isMPEG2, layer);
  sideInfoSize = computeSideInfoSize();
 }

unsigned MP3FrameParams::computeSideInfoSize() {
  unsigned size;

  if (isMPEG2) {
    size = isStereo ? 17 : 9;
  } else {
    size = isStereo ? 32 : 17;
  }

  if (hasCRC) {
    size += 2;
  }

  return size;
}

unsigned ComputeFrameSize(unsigned bitrate, unsigned samplingFreq,
			  Boolean usePadding, Boolean isMPEG2,
			  unsigned char layer) {
  if (samplingFreq == 0) return 0;
  unsigned const bitrateMultiplier = (layer == 1) ? 12000*4 : 144000;
  unsigned framesize;

  framesize = bitrate*bitrateMultiplier;
  framesize /= samplingFreq<<isMPEG2;
  framesize = framesize + usePadding - 4;

  return framesize;
}

#define TRUNC_FAIRLY
static unsigned updateSideInfoSizes(MP3SideInfo& sideInfo, Boolean isMPEG2,
				    unsigned char const* mainDataPtr,
				    unsigned allowedNumBits,
				    unsigned& part23Length0a,
				    unsigned& part23Length0aTruncation,
				    unsigned& part23Length0b,
				    unsigned& part23Length0bTruncation,
				    unsigned& part23Length1a,
				    unsigned& part23Length1aTruncation,
				    unsigned& part23Length1b,
				    unsigned& part23Length1bTruncation) {
  unsigned p23L0, p23L1 = 0, p23L0Trunc = 0, p23L1Trunc = 0;

  p23L0 = sideInfo.ch[0].gr[0].part2_3_length;
  p23L1 = isMPEG2 ? 0 : sideInfo.ch[0].gr[1].part2_3_length;
#ifdef TRUNC_ONLY0
  if (p23L0 < allowedNumBits)
    allowedNumBits = p23L0;
#endif
#ifdef TRUNC_ONLY1
  if (p23L1 < allowedNumBits)
    allowedNumBits = p23L1;
#endif
  if (p23L0 + p23L1 > allowedNumBits) {
    /* We need to shorten one or both fields */
    unsigned truncation = p23L0 + p23L1 - allowedNumBits;
#ifdef TRUNC_FAIRLY
    p23L0Trunc = (truncation*p23L0)/(p23L0 + p23L1);
    p23L1Trunc = truncation - p23L0Trunc;
#endif
#if defined(TRUNC_FAVOR0) || defined(TRUNC_ONLY0)
    p23L1Trunc = (truncation>p23L1) ? p23L1 : truncation;
    p23L0Trunc = truncation - p23L1Trunc;
#endif
#if defined(TRUNC_FAVOR1) || defined(TRUNC_ONLY1)
    p23L0Trunc = (truncation>p23L0) ? p23L0 : truncation;
    p23L1Trunc = truncation - p23L0Trunc;
#endif
  }

  /* ASSERT: (p23L0Trunc <= p23L0) && (p23l1Trunc <= p23L1) */
  p23L0 -= p23L0Trunc; p23L1 -= p23L1Trunc;
#ifdef DEBUG
  fprintf(stderr, "updateSideInfoSizes (allowed: %d): %d->%d, %d->%d\n", allowedNumBits, p23L0+p23L0Trunc, p23L0, p23L1+p23L1Trunc, p23L1);
#endif

  // The truncations computed above are still estimates.  We need to
  // adjust them so that the new fields will continue to end on
  // Huffman-encoded sample boundaries:
  updateSideInfoForHuffman(sideInfo, isMPEG2, mainDataPtr,
			   p23L0, p23L1,
			   part23Length0a, part23Length0aTruncation,
			   part23Length0b, part23Length0bTruncation,
			   part23Length1a, part23Length1aTruncation,
			   part23Length1b, part23Length1bTruncation);
  p23L0 = part23Length0a + part23Length0b;
  p23L1 = part23Length1a + part23Length1b;

  sideInfo.ch[0].gr[0].part2_3_length = p23L0;
  sideInfo.ch[0].gr[1].part2_3_length = p23L1;
  part23Length0bTruncation
    += sideInfo.ch[1].gr[0].part2_3_length; /* allow for stereo */
  sideInfo.ch[1].gr[0].part2_3_length = 0; /* output mono */
  sideInfo.ch[1].gr[1].part2_3_length = 0; /* output mono */

  return p23L0 + p23L1;
}


Boolean GetADUInfoFromMP3Frame(unsigned char const* framePtr,
			       unsigned totFrameSize,
			       unsigned& hdr, unsigned& frameSize,
			       MP3SideInfo& sideInfo, unsigned& sideInfoSize,
			       unsigned& backpointer, unsigned& aduSize) {
  if (totFrameSize < 4) return False; // there's not enough data

  MP3FrameParams fr;
  fr.hdr =   ((unsigned)framePtr[0] << 24) | ((unsigned)framePtr[1] << 16)
           | ((unsigned)framePtr[2] << 8) | (unsigned)framePtr[3];
  fr.setParamsFromHeader();
  fr.setBytePointer(framePtr + 4, totFrameSize - 4); // skip hdr

  frameSize = 4 + fr.frameSize;

  if (fr.layer != 3) {
    // Special case for non-layer III frames
    backpointer = 0;
    sideInfoSize = 0;
    aduSize = fr.frameSize;
    return True;
  }

  sideInfoSize = fr.sideInfoSize;
  if (totFrameSize < 4 + sideInfoSize) return False; // not enough data

  fr.getSideInfo(sideInfo);

  hdr = fr.hdr;
  backpointer = sideInfo.main_data_begin;
  unsigned numBits = sideInfo.ch[0].gr[0].part2_3_length;
  numBits += sideInfo.ch[0].gr[1].part2_3_length;
  numBits += sideInfo.ch[1].gr[0].part2_3_length;
  numBits += sideInfo.ch[1].gr[1].part2_3_length;
  aduSize = (numBits+7)/8;
#ifdef DEBUG
  fprintf(stderr, "mp3GetADUInfoFromFrame: hdr: %08x, frameSize: %d, part2_3_lengths: %d,%d,%d,%d, aduSize: %d, backpointer: %d\n", hdr, frameSize, sideInfo.ch[0].gr[0].part2_3_length, sideInfo.ch[0].gr[1].part2_3_length, sideInfo.ch[1].gr[0].part2_3_length, sideInfo.ch[1].gr[1].part2_3_length, aduSize, backpointer);
#endif

  return True;
}


static void getSideInfo1(MP3FrameParams& fr, MP3SideInfo& si,
			 int stereo, int ms_stereo, long sfreq,
			 int /*single*/) {
   int ch, gr;
#if 0
   int powdiff = (single == 3) ? 4 : 0;
#endif

   /* initialize all four "part2_3_length" fields to zero: */
   si.ch[0].gr[0].part2_3_length = 0; si.ch[1].gr[0].part2_3_length = 0;
   si.ch[0].gr[1].part2_3_length = 0; si.ch[1].gr[1].part2_3_length = 0;

   si.main_data_begin = fr.getBits(9);
   if (stereo == 1)
     si.private_bits = fr.getBits(5);
   else
     si.private_bits = fr.getBits(3);

   for (ch=0; ch<stereo; ch++) {
       si.ch[ch].gr[0].scfsi = -1;
       si.ch[ch].gr[1].scfsi = fr.getBits(4);
   }

   for (gr=0; gr<2; gr++) {
     for (ch=0; ch<stereo; ch++) {
       MP3SideInfo::gr_info_s_t& gr_info = si.ch[ch].gr[gr];

       gr_info.part2_3_length = fr.getBits(12);
       gr_info.big_values = fr.getBits(9);
       gr_info.global_gain = fr.getBits(8);
#if 0
       gr_info.pow2gain = gainpow2+256 - gr_info.global_gain + powdiff;
       if (ms_stereo) gr_info.pow2gain += 2;
#endif
       gr_info.scalefac_compress = fr.getBits(4);
/* window-switching flag == 1 for block_Type != 0 .. and block-type == 0 -> win-sw-flag = 0 */
       gr_info.window_switching_flag = fr.get1Bit();
       if (gr_info.window_switching_flag) {
         int i;
         gr_info.block_type = fr.getBits(2);
         gr_info.mixed_block_flag = fr.get1Bit();
         gr_info.table_select[0] = fr.getBits(5);
         gr_info.table_select[1] = fr.getBits(5);
         /*
          * table_select[2] not needed, because there is no region2,
          * but to satisfy some verifications tools we set it either.
          */
         gr_info.table_select[2] = 0;
         for (i=0;i<3;i++) {
	   gr_info.subblock_gain[i] = fr.getBits(3);
           gr_info.full_gain[i]
	     = gr_info.pow2gain + ((gr_info.subblock_gain[i])<<3);
	 }

#ifdef DEBUG_ERRORS
         if (gr_info.block_type == 0) {
           fprintf(stderr,"Blocktype == 0 and window-switching == 1 not allowed.\n");
         }
#endif
         /* region_count/start parameters are implicit in this case. */
         gr_info.region1start = 36>>1;
         gr_info.region2start = 576>>1;
       }
       else
       {
         int i,r0c,r1c;
         for (i=0; i<3; i++) {
	   gr_info.table_select[i] = fr.getBits(5);
	 }
         r0c = gr_info.region0_count = fr.getBits(4);
         r1c = gr_info.region1_count = fr.getBits(3);
         gr_info.region1start = bandInfo[sfreq].longIdx[r0c+1] >> 1 ;
         gr_info.region2start = bandInfo[sfreq].longIdx[r0c+1+r1c+1] >> 1;
         gr_info.block_type = 0;
         gr_info.mixed_block_flag = 0;
       }
       gr_info.preflag = fr.get1Bit();
       gr_info.scalefac_scale = fr.get1Bit();
       gr_info.count1table_select = fr.get1Bit();
     }
   }
}

static void getSideInfo2(MP3FrameParams& fr, MP3SideInfo& si,
			 int stereo, int ms_stereo, long sfreq,
			 int /*single*/) {
   int ch;
#if 0
   int powdiff = (single == 3) ? 4 : 0;
#endif

   /* initialize all four "part2_3_length" fields to zero: */
   si.ch[0].gr[0].part2_3_length = 0; si.ch[1].gr[0].part2_3_length = 0;
   si.ch[0].gr[1].part2_3_length = 0; si.ch[1].gr[1].part2_3_length = 0;

   si.main_data_begin = fr.getBits(8);
   if (stereo == 1)
     si.private_bits = fr.get1Bit();
   else
     si.private_bits = fr.getBits(2);

   for (ch=0; ch<stereo; ch++) {
       MP3SideInfo::gr_info_s_t& gr_info = si.ch[ch].gr[0];

       gr_info.part2_3_length = fr.getBits(12);
       si.ch[ch].gr[1].part2_3_length = 0; /* to ensure granule 1 unused */

       gr_info.big_values = fr.getBits(9);
       gr_info.global_gain = fr.getBits(8);
#if 0
       gr_info.pow2gain = gainpow2+256 - gr_info.global_gain + powdiff;
       if (ms_stereo) gr_info.pow2gain += 2;
#endif
       gr_info.scalefac_compress = fr.getBits(9);
/* window-switching flag == 1 for block_Type != 0 .. and block-type == 0 -> win-sw-flag = 0 */
       gr_info.window_switching_flag = fr.get1Bit();
       if (gr_info.window_switching_flag) {
         int i;
         gr_info.block_type = fr.getBits(2);
         gr_info.mixed_block_flag = fr.get1Bit();
         gr_info.table_select[0] = fr.getBits(5);
         gr_info.table_select[1] = fr.getBits(5);
         /*
          * table_select[2] not needed, because there is no region2,
          * but to satisfy some verifications tools we set it either.
          */
         gr_info.table_select[2] = 0;
         for (i=0;i<3;i++) {
	   gr_info.subblock_gain[i] = fr.getBits(3);
           gr_info.full_gain[i]
	     = gr_info.pow2gain + ((gr_info.subblock_gain[i])<<3);
	 }

#ifdef DEBUG_ERRORS
         if (gr_info.block_type == 0) {
           fprintf(stderr,"Blocktype == 0 and window-switching == 1 not allowed.\n");
         }
#endif
         /* region_count/start parameters are implicit in this case. */
/* check this again! */
         if (gr_info.block_type == 2)
           gr_info.region1start = 36>>1;
         else {
           gr_info.region1start = 54>>1;
         }
         gr_info.region2start = 576>>1;
       }
       else
       {
         int i,r0c,r1c;
         for (i=0; i<3; i++) {
           gr_info.table_select[i] = fr.getBits(5);
	 }
         r0c = gr_info.region0_count = fr.getBits(4);
         r1c = gr_info.region1_count = fr.getBits(3);
         gr_info.region1start = bandInfo[sfreq].longIdx[r0c+1] >> 1 ;
         gr_info.region2start = bandInfo[sfreq].longIdx[r0c+1+r1c+1] >> 1;
         gr_info.block_type = 0;
         gr_info.mixed_block_flag = 0;
       }
       gr_info.scalefac_scale = fr.get1Bit();
       gr_info.count1table_select = fr.get1Bit();
   }
}


#define         MPG_MD_JOINT_STEREO     1

void MP3FrameParams::getSideInfo(MP3SideInfo& si) {
  // First skip over the CRC if present:
  if (hasCRC) getBits(16);

  int single = -1;
  int ms_stereo, i_stereo;
  int sfreq = samplingFreqIndex;

  if (stereo == 1) {
    single = 0;
  }

  ms_stereo = (mode == MPG_MD_JOINT_STEREO) && (mode_ext & 0x2);
  i_stereo = (mode == MPG_MD_JOINT_STEREO) && (mode_ext & 0x1);

  if (isMPEG2) {
    getSideInfo2(*this, si, stereo, ms_stereo, sfreq, single);
  } else {
    getSideInfo1(*this, si, stereo, ms_stereo, sfreq, single);
  }
}

static void putSideInfo1(BitVector& bv,
			 MP3SideInfo const& si, Boolean isStereo) {
  int ch, gr, i;
  int stereo = isStereo ? 2 : 1;

  bv.putBits(si.main_data_begin,9);
  if (stereo == 1)
    bv.putBits(si.private_bits, 5);
  else
    bv.putBits(si.private_bits, 3);

  for (ch=0; ch<stereo; ch++) {
    bv.putBits(si.ch[ch].gr[1].scfsi, 4);
  }

  for (gr=0; gr<2; gr++) {
    for (ch=0; ch<stereo; ch++) {
      MP3SideInfo::gr_info_s_t const& gr_info = si.ch[ch].gr[gr];

      bv.putBits(gr_info.part2_3_length, 12);
      bv.putBits(gr_info.big_values, 9);
      bv.putBits(gr_info.global_gain, 8);
      bv.putBits(gr_info.scalefac_compress, 4);
      bv.put1Bit(gr_info.window_switching_flag);
      if (gr_info.window_switching_flag) {
	bv.putBits(gr_info.block_type, 2);
	bv.put1Bit(gr_info.mixed_block_flag);
	for (i=0; i<2; i++)
	  bv.putBits(gr_info.table_select[i], 5);
	for (i=0; i<3; i++)
	  bv.putBits(gr_info.subblock_gain[i], 3);
      }
      else {
	for (i=0; i<3; i++)
	  bv.putBits(gr_info.table_select[i], 5);
	bv.putBits(gr_info.region0_count, 4);
	bv.putBits(gr_info.region1_count, 3);
      }

      bv.put1Bit(gr_info.preflag);
      bv.put1Bit(gr_info.scalefac_scale);
      bv.put1Bit(gr_info.count1table_select);
    }
  }
}

static void putSideInfo2(BitVector& bv,
			 MP3SideInfo const& si, Boolean isStereo) {
  int ch, i;
  int stereo = isStereo ? 2 : 1;

  bv.putBits(si.main_data_begin,8);
  if (stereo == 1)
    bv.put1Bit(si.private_bits);
  else
    bv.putBits(si.private_bits, 2);

  for (ch=0; ch<stereo; ch++) {
    MP3SideInfo::gr_info_s_t const& gr_info = si.ch[ch].gr[0];

    bv.putBits(gr_info.part2_3_length, 12);
    bv.putBits(gr_info.big_values, 9);
    bv.putBits(gr_info.global_gain, 8);
    bv.putBits(gr_info.scalefac_compress, 9);
    bv.put1Bit(gr_info.window_switching_flag);
    if (gr_info.window_switching_flag) {
      bv.putBits(gr_info.block_type, 2);
      bv.put1Bit(gr_info.mixed_block_flag);
      for (i=0; i<2; i++)
	bv.putBits(gr_info.table_select[i], 5);
      for (i=0; i<3; i++)
	bv.putBits(gr_info.subblock_gain[i], 3);
    }
    else {
      for (i=0; i<3; i++)
	bv.putBits(gr_info.table_select[i], 5);
      bv.putBits(gr_info.region0_count, 4);
      bv.putBits(gr_info.region1_count, 3);
    }

    bv.put1Bit(gr_info.scalefac_scale);
    bv.put1Bit(gr_info.count1table_select);
  }
}

static void PutMP3SideInfoIntoFrame(MP3SideInfo const& si,
				    MP3FrameParams const& fr,
				    unsigned char* framePtr) {
  if (fr.hasCRC) framePtr += 2; // skip CRC

  BitVector bv(framePtr, 0, 8*fr.sideInfoSize);

  if (fr.isMPEG2) {
    putSideInfo2(bv, si, fr.isStereo);
  } else {
    putSideInfo1(bv, si, fr.isStereo);
  }
}


Boolean ZeroOutMP3SideInfo(unsigned char* framePtr, unsigned totFrameSize,
                           unsigned newBackpointer) {
  if (totFrameSize < 4) return False; // there's not enough data

  MP3FrameParams fr;
  fr.hdr =   ((unsigned)framePtr[0] << 24) | ((unsigned)framePtr[1] << 16)
           | ((unsigned)framePtr[2] << 8) | (unsigned)framePtr[3];
  fr.setParamsFromHeader();
  fr.setBytePointer(framePtr + 4, totFrameSize - 4); // skip hdr

  if (totFrameSize < 4 + fr.sideInfoSize) return False; // not enough data

  MP3SideInfo si;
  fr.getSideInfo(si);

  si.main_data_begin = newBackpointer; /* backpointer */
  /* set all four "part2_3_length" and "big_values" fields to zero: */
  si.ch[0].gr[0].part2_3_length = si.ch[0].gr[0].big_values = 0;
  si.ch[1].gr[0].part2_3_length = si.ch[1].gr[0].big_values = 0;
  si.ch[0].gr[1].part2_3_length = si.ch[0].gr[1].big_values = 0;
  si.ch[1].gr[1].part2_3_length = si.ch[1].gr[1].big_values = 0;

  PutMP3SideInfoIntoFrame(si, fr, framePtr + 4);

  return True;
}


static unsigned MP3BitrateToBitrateIndex(unsigned bitrate /* in kbps */,
					 Boolean isMPEG2) {
  for (unsigned i = 1; i < 15; ++i) {
    if (live_tabsel[isMPEG2][2][i] >= bitrate)
      return i;
  }

  // "bitrate" was larger than any possible, so return the largest possible:
  return 14;
}

static void outputHeader(unsigned char* toPtr, unsigned hdr) {
  toPtr[0] = (unsigned char)(hdr>>24);
  toPtr[1] = (unsigned char)(hdr>>16);
  toPtr[2] = (unsigned char)(hdr>>8);
  toPtr[3] = (unsigned char)(hdr);
}

static void assignADUBackpointer(MP3FrameParams const& fr,
				 unsigned aduSize,
				 MP3SideInfo& sideInfo,
				 unsigned& availableBytesForBackpointer) {
  // Give the ADU as large a backpointer as possible:
  unsigned maxBackpointerSize = fr.isMPEG2 ? 255 : 511;

  unsigned backpointerSize = availableBytesForBackpointer;
  if (backpointerSize > maxBackpointerSize) {
    backpointerSize = maxBackpointerSize;
  }

  // Store the new backpointer now:
  sideInfo.main_data_begin = backpointerSize;

  // Figure out how many bytes are available for the *next* ADU's backpointer:
  availableBytesForBackpointer
    = backpointerSize + fr.frameSize - fr.sideInfoSize ;
  if (availableBytesForBackpointer < aduSize) {
    availableBytesForBackpointer = 0;
  } else {
    availableBytesForBackpointer -= aduSize;
  }
}

unsigned TranscodeMP3ADU(unsigned char const* fromPtr, unsigned fromSize,
                      unsigned toBitrate,
		      unsigned char* toPtr, unsigned toMaxSize,
		      unsigned& availableBytesForBackpointer) {
  // Begin by parsing the input ADU's parameters:
  unsigned hdr, inFrameSize, inSideInfoSize, backpointer, inAduSize;
  MP3SideInfo sideInfo;
  if (!GetADUInfoFromMP3Frame(fromPtr, fromSize,
                              hdr, inFrameSize, sideInfo, inSideInfoSize,
			      backpointer, inAduSize)) {
    return 0;
  }
  fromPtr += (4+inSideInfoSize); // skip to 'main data'

  // Alter the 4-byte MPEG header to reflect the output ADU:
  // (different bitrate; mono; no CRC)
  Boolean isMPEG2 = ((hdr&0x00080000) == 0);
  unsigned toBitrateIndex = MP3BitrateToBitrateIndex(toBitrate, isMPEG2);
  hdr &=~ 0xF000; hdr |= (toBitrateIndex<<12); // set bitrate index
  hdr |= 0x10200; // turn on !error-prot and padding bits
  hdr &=~ 0xC0; hdr |= 0xC0; // set mode to 3 (mono)

  // Set up the rest of the parameters of the new ADU:
  MP3FrameParams outFr;
  outFr.hdr = hdr;
  outFr.setParamsFromHeader();

  // Figure out how big to make the output ADU:
  unsigned inAveAduSize = inFrameSize - inSideInfoSize;
  unsigned outAveAduSize = outFr.frameSize - outFr.sideInfoSize;
  unsigned desiredOutAduSize /*=inAduSize*outAveAduSize/inAveAduSize*/
    = (2*inAduSize*outAveAduSize + inAveAduSize)/(2*inAveAduSize);
      // this rounds to the nearest integer

  if (toMaxSize < (4 + outFr.sideInfoSize)) return 0;
  unsigned maxOutAduSize = toMaxSize - (4 + outFr.sideInfoSize);
  if (desiredOutAduSize > maxOutAduSize) {
    desiredOutAduSize = maxOutAduSize;
  }

  // Figure out the new sizes of the various 'part23 lengths',
  // and how much they are truncated:
  unsigned part23Length0a, part23Length0aTruncation;
  unsigned part23Length0b, part23Length0bTruncation;
  unsigned part23Length1a, part23Length1aTruncation;
  unsigned part23Length1b, part23Length1bTruncation;
  unsigned numAduBits
    = updateSideInfoSizes(sideInfo, outFr.isMPEG2,
			  fromPtr, 8*desiredOutAduSize,
			  part23Length0a, part23Length0aTruncation,
			  part23Length0b, part23Length0bTruncation,
			  part23Length1a, part23Length1aTruncation,
			  part23Length1b, part23Length1bTruncation);
#ifdef DEBUG
fprintf(stderr, "shrinkage %d->%d [(%d,%d),(%d,%d)] (trunc: [(%d,%d),(%d,%d)]) {%d}\n", inAduSize, (numAduBits+7)/8,
	      part23Length0a, part23Length0b, part23Length1a, part23Length1b,
	      part23Length0aTruncation, part23Length0bTruncation,
	      part23Length1aTruncation, part23Length1bTruncation,
	      maxOutAduSize);
#endif
 unsigned actualOutAduSize = (numAduBits+7)/8;

 // Give the new ADU an appropriate 'backpointer':
 assignADUBackpointer(outFr, actualOutAduSize, sideInfo, availableBytesForBackpointer);

 ///// Now output the new ADU:

 // 4-byte header
 outputHeader(toPtr, hdr); toPtr += 4;

 // side info
 PutMP3SideInfoIntoFrame(sideInfo, outFr, toPtr); toPtr += outFr.sideInfoSize;

 // 'main data', using the new lengths
 unsigned toBitOffset = 0;
 unsigned fromBitOffset = 0;

 /* rebuild portion 0a: */
 memmove(toPtr, fromPtr, (part23Length0a+7)/8);
 toBitOffset += part23Length0a;
 fromBitOffset += part23Length0a + part23Length0aTruncation;

 /* rebuild portion 0b: */
 shiftBits(toPtr, toBitOffset, fromPtr, fromBitOffset, part23Length0b);
 toBitOffset += part23Length0b;
 fromBitOffset += part23Length0b + part23Length0bTruncation;

 /* rebuild portion 1a: */
 shiftBits(toPtr, toBitOffset, fromPtr, fromBitOffset, part23Length1a);
 toBitOffset += part23Length1a;
 fromBitOffset += part23Length1a + part23Length1aTruncation;

 /* rebuild portion 1b: */
 shiftBits(toPtr, toBitOffset, fromPtr, fromBitOffset, part23Length1b);
 toBitOffset += part23Length1b;

 /* zero out any remaining bits (probably unnecessary, but...) */
 unsigned char const zero = '\0';
 shiftBits(toPtr, toBitOffset, &zero, 0,
	   actualOutAduSize*8 - numAduBits);

 return 4 + outFr.sideInfoSize + actualOutAduSize;
}
