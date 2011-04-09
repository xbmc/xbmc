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
// RTP sink for MPEG video (RFC 2250)
// Implementation

#include "MPEG1or2VideoRTPSink.hh"
#include "MPEG1or2VideoStreamFramer.hh"

MPEG1or2VideoRTPSink::MPEG1or2VideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs)
  : VideoRTPSink(env, RTPgs, 32, 90000, "MPV") {
  fPictureState.temporal_reference = 0;
  fPictureState.picture_coding_type = fPictureState.vector_code_bits = 0;
}

MPEG1or2VideoRTPSink::~MPEG1or2VideoRTPSink() {
}

MPEG1or2VideoRTPSink*
MPEG1or2VideoRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new MPEG1or2VideoRTPSink(env, RTPgs);
}

Boolean MPEG1or2VideoRTPSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // Our source must be an appropriate framer:
  return source.isMPEG1or2VideoStreamFramer();
}

Boolean MPEG1or2VideoRTPSink::allowFragmentationAfterStart() const {
  return True;
}

Boolean MPEG1or2VideoRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* frameStart,
				 unsigned numBytesInFrame) const {
  // A 'frame' (which in this context can mean a header or a slice as well as a
  // complete picture) can appear at other than the first position in a packet
  // in all situations, EXCEPT when it follows the end of (i.e., the last slice
  // of) a picture.  I.e., the headers at the beginning of a picture must
  // appear at the start of a RTP packet.
  if (!fPreviousFrameWasSlice) return True;

  // A slice is already packed into this packet.  We allow this new 'frame'
  // to be packed after it, provided that it is also a slice:
  return numBytesInFrame >= 4
    && frameStart[0] == 0 && frameStart[1] == 0 && frameStart[2] == 1
    && frameStart[3] >= 1 && frameStart[3] <= 0xAF;
}

#define VIDEO_SEQUENCE_HEADER_START_CODE 0x000001B3
#define PICTURE_START_CODE               0x00000100

void MPEG1or2VideoRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval frameTimestamp,
			 unsigned numRemainingBytes) {
  Boolean thisFrameIsASlice = False; // until we learn otherwise
  if (isFirstFrameInPacket()) {
    fSequenceHeaderPresent = fPacketBeginsSlice = fPacketEndsSlice = False;
  }

  if (fragmentationOffset == 0) {
    // Begin by inspecting the 4-byte code at the start of the frame:
    if (numBytesInFrame < 4) return; // shouldn't happen
    unsigned startCode = (frameStart[0]<<24) | (frameStart[1]<<16)
                       | (frameStart[2]<<8) | frameStart[3];

    if (startCode == VIDEO_SEQUENCE_HEADER_START_CODE) {
      // This is a video sequence header
      fSequenceHeaderPresent = True;
    } else if (startCode == PICTURE_START_CODE) {
      // This is a picture header

      // Record the parameters of this picture:
      if (numBytesInFrame < 8) return; // shouldn't happen
      unsigned next4Bytes = (frameStart[4]<<24) | (frameStart[5]<<16)
	                  | (frameStart[6]<<8) | frameStart[7];
      unsigned char byte8 = numBytesInFrame == 8 ? 0 : frameStart[8];

      fPictureState.temporal_reference = (next4Bytes&0xFFC00000)>>(32-10);
      fPictureState.picture_coding_type = (next4Bytes&0x00380000)>>(32-(10+3));

      unsigned char FBV, BFC, FFV, FFC;
      FBV = BFC = FFV = FFC = 0;
      switch (fPictureState.picture_coding_type) {
      case 3:
	FBV = (byte8&0x40)>>6;
	BFC = (byte8&0x38)>>3;
	// fall through to:
      case 2:
	FFV = (next4Bytes&0x00000004)>>2;
	FFC = ((next4Bytes&0x00000003)<<1) | ((byte8&0x80)>>7);
      }

      fPictureState.vector_code_bits = (FBV<<7) | (BFC<<4) | (FFV<<3) | FFC;
    } else if ((startCode&0xFFFFFF00) == 0x00000100) {
      unsigned char lastCodeByte = startCode&0xFF;

      if (lastCodeByte <= 0xAF) {
	// This is (the start of) a slice
	thisFrameIsASlice = True;
      } else {
	// This is probably a GOP header; we don't do anything with this
      }
    } else {
      // The first 4 bytes aren't a code that we recognize.
      envir() << "Warning: MPEG1or2VideoRTPSink::doSpecialFrameHandling saw strange first 4 bytes "
	      << (void*)startCode << ", but we're not a fragment\n";
    }
  } else {
    // We're a fragment (other than the first) of a slice.
    thisFrameIsASlice = True;
  }

  if (thisFrameIsASlice) {
    // This packet begins a slice iff there's no fragmentation offset:
    fPacketBeginsSlice = (fragmentationOffset == 0);

    // This packet also ends a slice iff there are no fragments remaining:
    fPacketEndsSlice = (numRemainingBytes == 0);
  }

  // Set the video-specific header based on the parameters that we've seen.
  // Note that this may get done more than once, if several frames appear
  // in the packet.  That's OK, because this situation happens infrequently,
  // and we want the video-specific header to reflect the most up-to-date
  // information (in particular, from a Picture Header) anyway.
  unsigned videoSpecificHeader =
    // T == 0
    (fPictureState.temporal_reference<<16) |
    // AN == N == 0
    (fSequenceHeaderPresent<<13) |
    (fPacketBeginsSlice<<12) |
    (fPacketEndsSlice<<11) |
    (fPictureState.picture_coding_type<<8) |
    fPictureState.vector_code_bits;
  setSpecialHeaderWord(videoSpecificHeader);

  // Also set the RTP timestamp.  (As above, we do this for each frame
  // in the packet.)
  setTimestamp(frameTimestamp);

  // Set the RTP 'M' (marker) bit iff this frame ends (i.e., is the last
  // slice of) a picture (and there are no fragments remaining).
  // This relies on the source being a "MPEG1or2VideoStreamFramer".
  MPEG1or2VideoStreamFramer* framerSource = (MPEG1or2VideoStreamFramer*)fSource;
  if (framerSource != NULL && framerSource->pictureEndMarker()
      && numRemainingBytes == 0) {
    setMarkerBit();
    framerSource->pictureEndMarker() = False;
  }

  fPreviousFrameWasSlice = thisFrameIsASlice;
}

unsigned MPEG1or2VideoRTPSink::specialHeaderSize() const {
  // There's a 4 byte special audio header:
  return 4;
}
