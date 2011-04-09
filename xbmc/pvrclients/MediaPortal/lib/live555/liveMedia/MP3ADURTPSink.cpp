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
// RTP sink for 'ADUized' MP3 frames ("mpa-robust")
// Implementation

#include "MP3ADURTPSink.hh"

MP3ADURTPSink::MP3ADURTPSink(UsageEnvironment& env, Groupsock* RTPgs,
			     unsigned char RTPPayloadType)
  : AudioRTPSink(env, RTPgs, RTPPayloadType, 90000, "MPA-ROBUST") {
}

MP3ADURTPSink::~MP3ADURTPSink() {
}

MP3ADURTPSink*
MP3ADURTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			 unsigned char RTPPayloadType) {
  return new MP3ADURTPSink(env, RTPgs, RTPPayloadType);
}

static void badDataSize(UsageEnvironment& env, unsigned numBytesInFrame) {
  env << "MP3ADURTPSink::doSpecialFrameHandling(): invalid size ("
      << numBytesInFrame << ") of non-fragmented input ADU!\n";
}

void MP3ADURTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
					   unsigned char* frameStart,
					   unsigned numBytesInFrame,
					   struct timeval frameTimestamp,
					   unsigned numRemainingBytes) {
  // If this is the first (or only) fragment of an ADU, then
  // check the "ADU descriptor" (that should be at the front) for validity:
  if (fragmentationOffset == 0) {
    unsigned aduDescriptorSize;

    if (numBytesInFrame < 1) {
      badDataSize(envir(), numBytesInFrame);
      return;
    }
    if (frameStart[0]&0x40) {
      // We have a 2-byte ADU descriptor
      aduDescriptorSize = 2;
      if (numBytesInFrame < 2) {
	badDataSize(envir(), numBytesInFrame);
	return;
      }
      fCurADUSize = ((frameStart[0]&~0xC0)<<8) | frameStart[1];
    } else {
      // We have a 1-byte ADU descriptor
      aduDescriptorSize = 1;
      fCurADUSize = frameStart[0]&~0x80;
    }

    if (frameStart[0]&0x80) {
      envir() << "Unexpected \"C\" bit seen on non-fragment input ADU!\n";
      return;
    }

    // Now, check whether the ADU size in the ADU descriptor is consistent
    // with the total data size of (all fragments of) the input frame:
    unsigned expectedADUSize =
      fragmentationOffset + numBytesInFrame + numRemainingBytes
      - aduDescriptorSize;
    if (fCurADUSize != expectedADUSize) {
      envir() << "MP3ADURTPSink::doSpecialFrameHandling(): Warning: Input ADU size "
	      << expectedADUSize << " (=" << fragmentationOffset
	      << "+" << numBytesInFrame << "+" << numRemainingBytes
	      << "-" << aduDescriptorSize
	      << ") did not match the value (" << fCurADUSize
	      << ") in the ADU descriptor!\n";
      fCurADUSize = expectedADUSize;
    }
  } else {
    // This is the second (or subsequent) fragment.
    // Insert a new ADU descriptor:
    unsigned char aduDescriptor[2];
    aduDescriptor[0] = 0xC0|(fCurADUSize>>8);
    aduDescriptor[1] = fCurADUSize&0xFF;
    setSpecialHeaderBytes(aduDescriptor, 2);
  }

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
					     frameStart, numBytesInFrame,
					     frameTimestamp,
					     numRemainingBytes);
}

unsigned MP3ADURTPSink::specialHeaderSize() const {
  // Normally there's no special header.
  // (The "ADU descriptor" is already present in the data.)
  unsigned specialHeaderSize = 0;

  // However, if we're about to output the second (or subsequent) fragment
  // of a fragmented ADU, then we need to insert a new ADU descriptor at
  // the front of the packet:
  if (curFragmentationOffset() > 0) {
    specialHeaderSize = 2;
  }

  return specialHeaderSize;
}
