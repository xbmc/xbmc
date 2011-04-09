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
// RTP sink for MPEG audio (RFC 2250)
// Implementation

#include "MPEG1or2AudioRTPSink.hh"

MPEG1or2AudioRTPSink::MPEG1or2AudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs)
  : AudioRTPSink(env, RTPgs, 14, 90000, "MPA") {
}

MPEG1or2AudioRTPSink::~MPEG1or2AudioRTPSink() {
}

MPEG1or2AudioRTPSink*
MPEG1or2AudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs) {
  return new MPEG1or2AudioRTPSink(env, RTPgs);
}

void MPEG1or2AudioRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
					      unsigned char* frameStart,
					      unsigned numBytesInFrame,
					      struct timeval frameTimestamp,
					      unsigned numRemainingBytes) {
  // If this is the 1st frame in the 1st packet, set the RTP 'M' (marker)
  // bit (because this is considered the start of a talk spurt):
  if (isFirstPacket() && isFirstFrameInPacket()) {
    setMarkerBit();
  }

  // If this is the first frame in the packet, set the lower half of the
  // audio-specific header (to the "fragmentationOffset"):
  if (isFirstFrameInPacket()) {
    setSpecialHeaderWord(fragmentationOffset&0xFFFF);
  }

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
					     frameStart, numBytesInFrame,
					     frameTimestamp,
					     numRemainingBytes);
}

unsigned MPEG1or2AudioRTPSink::specialHeaderSize() const {
  // There's a 4 byte special audio header:
  return 4;
}
