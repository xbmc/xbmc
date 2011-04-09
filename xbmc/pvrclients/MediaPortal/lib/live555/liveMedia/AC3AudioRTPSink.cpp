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
// RTP sink for AC3 audio
// Implementation

#include "AC3AudioRTPSink.hh"

AC3AudioRTPSink::AC3AudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
				 u_int8_t rtpPayloadFormat,
				 u_int32_t rtpTimestampFrequency)
  : AudioRTPSink(env, RTPgs, rtpPayloadFormat,
		       rtpTimestampFrequency, "AC3") {
}

AC3AudioRTPSink::~AC3AudioRTPSink() {
}

AC3AudioRTPSink*
AC3AudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			   u_int8_t rtpPayloadFormat,
			   u_int32_t rtpTimestampFrequency) {
  return new AC3AudioRTPSink(env, RTPgs,
			     rtpPayloadFormat, rtpTimestampFrequency);
}

Boolean AC3AudioRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
                                 unsigned /*numBytesInFrame*/) const {
  // (For now) allow at most 1 frame in a single packet:
  return False;
}

void AC3AudioRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval frameTimestamp,
			 unsigned numRemainingBytes) {
  // Update the "NDU" header.
  // Also set the "Data Unit Header" for the frame, because we
  // have already allotted space for this, by virtue of the fact that
  // (for now) we pack only one frame in each RTP packet:
  unsigned char headers[2];
  headers[0] = numFramesUsedSoFar() + 1;

  Boolean isFragment = numRemainingBytes > 0 || fragmentationOffset > 0;
  unsigned const totalFrameSize
    = fragmentationOffset + numBytesInFrame + numRemainingBytes;
  unsigned const fiveEighthsPoint = totalFrameSize/2 + totalFrameSize/8;
  Boolean haveFiveEighths
    = fragmentationOffset == 0 && numBytesInFrame >= fiveEighthsPoint;
  headers[1] = (isFragment<<5)|(haveFiveEighths<<4); // F|B
      // Note: TYP==0, RDT==0 ???, T==0 ???

  setSpecialHeaderBytes(headers, sizeof headers);

  if (numRemainingBytes == 0) {
    // This packet contains the last (or only) fragment of the frame.
    // Set the RTP 'M' ('marker') bit:
    setMarkerBit();
  }

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
					     frameStart, numBytesInFrame,
					     frameTimestamp,
					     numRemainingBytes);
}

unsigned AC3AudioRTPSink::specialHeaderSize() const {
  // There's a 1 byte "NDU" header.
  // There's also a 1-byte "Data Unit Header" preceding each frame in
  // the RTP packet, but since we (for now) pack only one frame in
  // each RTP packet, we also count this here:
  return 1 + 1;
}
