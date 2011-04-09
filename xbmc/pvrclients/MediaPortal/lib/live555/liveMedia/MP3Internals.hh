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
// C++ header

#ifndef _MP3_INTERNALS_HH
#define _MP3_INTERNALS_HH

#ifndef _BOOLEAN_HH
#include "Boolean.hh"
#endif
#ifndef _BIT_VECTOR_HH
#include "BitVector.hh"
#endif

typedef struct MP3SideInfo {
  unsigned main_data_begin;
  unsigned private_bits;
  typedef struct gr_info_s {
    int scfsi;
    unsigned part2_3_length;
    unsigned big_values;
    unsigned global_gain;
    unsigned scalefac_compress;
    unsigned window_switching_flag;
    unsigned block_type;
    unsigned mixed_block_flag;
    unsigned table_select[3];
    unsigned region0_count;
    unsigned region1_count;
    unsigned subblock_gain[3];
    unsigned maxband[3];
    unsigned maxbandl;
    unsigned maxb;
    unsigned region1start;
    unsigned region2start;
    unsigned preflag;
    unsigned scalefac_scale;
    unsigned count1table_select;
    double *full_gain[3];
    double *pow2gain;
  } gr_info_s_t;
  struct {
    gr_info_s_t gr[2];
  } ch[2];
} MP3SideInfo_t;

#define SBLIMIT 32
#define MAX_MP3_FRAME_SIZE 2500 /* also big enough for an 'ADU'ized frame */

class MP3FrameParams {
public:
  MP3FrameParams();
  ~MP3FrameParams();

  // 4-byte MPEG header:
  unsigned hdr;

  // a buffer that can be used to hold the rest of the frame:
  unsigned char frameBytes[MAX_MP3_FRAME_SIZE];

  // public parameters derived from the header
  void setParamsFromHeader(); // this sets them
  Boolean isMPEG2;
  unsigned layer; // currently only 3 is supported
  unsigned bitrate; // in kbps
  unsigned samplingFreq;
  Boolean isStereo;
  Boolean isFreeFormat;
  unsigned frameSize; // doesn't include the initial 4-byte header
  unsigned sideInfoSize;
  Boolean hasCRC;

  void setBytePointer(unsigned char const* restOfFrame,
		      unsigned totNumBytes) {// called during setup
    bv.setup((unsigned char*)restOfFrame, 0, 8*totNumBytes);
  }

  // other, public parameters used when parsing input (perhaps get rid of)
  unsigned oldHdr, firstHdr;

  // Extract (unpack) the side info from the frame into a struct:
  void getSideInfo(MP3SideInfo& si);

  // The bit pointer used for reading data from frame data
  unsigned getBits(unsigned numBits) { return bv.getBits(numBits); }
  unsigned get1Bit() { return bv.get1Bit(); }

private:
  BitVector bv;

  // other, private parameters derived from the header
  unsigned bitrateIndex;
  unsigned samplingFreqIndex;
  Boolean isMPEG2_5;
  Boolean padding;
  Boolean extension;
  unsigned mode;
  unsigned mode_ext;
  Boolean copyright;
  Boolean original;
  unsigned emphasis;
  unsigned stereo;

private:
  unsigned computeSideInfoSize();
};

unsigned ComputeFrameSize(unsigned bitrate, unsigned samplingFreq,
			  Boolean usePadding, Boolean isMPEG2,
			  unsigned char layer);

Boolean GetADUInfoFromMP3Frame(unsigned char const* framePtr,
			       unsigned totFrameSize,
			       unsigned& hdr, unsigned& frameSize,
			       MP3SideInfo& sideInfo, unsigned& sideInfoSize,
			       unsigned& backpointer, unsigned& aduSize);

Boolean ZeroOutMP3SideInfo(unsigned char* framePtr, unsigned totFrameSize,
			   unsigned newBackpointer);

unsigned TranscodeMP3ADU(unsigned char const* fromPtr, unsigned fromSize,
		      unsigned toBitrate,
		      unsigned char* toPtr, unsigned toMaxSize,
		      unsigned& availableBytesForBackpointer);
  // returns the size of the resulting ADU (0 on failure)

#endif
