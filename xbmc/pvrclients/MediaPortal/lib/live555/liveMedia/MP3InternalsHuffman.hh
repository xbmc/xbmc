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
// MP3 internal implementation details (Huffman encoding)
// C++ header

#ifndef _MP3_INTERNALS_HUFFMAN_HH
#define _MP3_INTERNALS_HUFFMAN_HH

#ifndef _MP3_INTERNALS_HH
#include "MP3Internals.hh"
#endif

void updateSideInfoForHuffman(MP3SideInfo& sideInfo, Boolean isMPEG2,
			      unsigned char const* mainDataPtr,
			      unsigned p23L0, unsigned p23L1,
			      unsigned& part23Length0a,
			      unsigned& part23Length0aTruncation,
			      unsigned& part23Length0b,
			      unsigned& part23Length0bTruncation,
			      unsigned& part23Length1a,
			      unsigned& part23Length1aTruncation,
			      unsigned& part23Length1b,
			      unsigned& part23Length1bTruncation);

#define SSLIMIT 18

class MP3HuffmanEncodingInfo {
public:
  MP3HuffmanEncodingInfo(Boolean includeDecodedValues = False);
  ~MP3HuffmanEncodingInfo();

public:
  unsigned numSamples;
  unsigned allBitOffsets[SBLIMIT*SSLIMIT + 1];
  unsigned reg1Start, reg2Start, bigvalStart; /* special bit offsets */
  unsigned* decodedValues;
};

/* forward */
void MP3HuffmanDecode(MP3SideInfo::gr_info_s_t* gr, int isMPEG2,
		      unsigned char const* fromBasePtr,
		      unsigned fromBitOffset, unsigned fromLength,
		      unsigned& scaleFactorsLength,
		      MP3HuffmanEncodingInfo& hei);

extern unsigned char huffdec[]; // huffman table data

// The following are used if we process Huffman-decoded values
#ifdef FOUR_BYTE_SAMPLES
#define BYTES_PER_SAMPLE_VALUE 4
#else
#ifdef TWO_BYTE_SAMPLES
#define BYTES_PER_SAMPLE_VALUE 2
#else
// ONE_BYTE_SAMPLES
#define BYTES_PER_SAMPLE_VALUE 1
#endif
#endif

#ifdef DO_HUFFMAN_ENCODING
unsigned MP3HuffmanEncode(MP3SideInfo::gr_info_s_t const* gr,
			  unsigned char const* fromPtr,
			  unsigned char* toPtr, unsigned toBitOffset,
			  unsigned numHuffBits);
#endif

#endif
