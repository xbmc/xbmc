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
// A simple RTP sink that packs frames into each outgoing
//     packet, without any fragmentation or special headers.
// Implementation

#include "SimpleRTPSink.hh"

SimpleRTPSink::SimpleRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
			     unsigned char rtpPayloadFormat,
			     unsigned rtpTimestampFrequency,
			     char const* sdpMediaTypeString,
			     char const* rtpPayloadFormatName,
			     unsigned numChannels,
			     Boolean allowMultipleFramesPerPacket,
			     Boolean doNormalMBitRule)
  : MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat,
		       rtpTimestampFrequency, rtpPayloadFormatName,
		       numChannels),
    fAllowMultipleFramesPerPacket(allowMultipleFramesPerPacket) {
  fSDPMediaTypeString
    = strDup(sdpMediaTypeString == NULL ? "unknown" : sdpMediaTypeString);
  fSetMBitOnLastFrames
    = strcmp(fSDPMediaTypeString, "video") == 0 && doNormalMBitRule;
}

SimpleRTPSink::~SimpleRTPSink() {
  delete[] (char*)fSDPMediaTypeString;
}

SimpleRTPSink*
SimpleRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			 unsigned char rtpPayloadFormat,
			 unsigned rtpTimestampFrequency,
			 char const* sdpMediaTypeString,
			 char const* rtpPayloadFormatName,
			 unsigned numChannels,
			 Boolean allowMultipleFramesPerPacket,
			 Boolean doNormalMBitRule) {
  return new SimpleRTPSink(env, RTPgs,
			   rtpPayloadFormat, rtpTimestampFrequency,
			   sdpMediaTypeString, rtpPayloadFormatName,
			   numChannels,
			   allowMultipleFramesPerPacket,
			   doNormalMBitRule);
}

void SimpleRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
					   unsigned char* frameStart,
					   unsigned numBytesInFrame,
					   struct timeval frameTimestamp,
					   unsigned numRemainingBytes) {
  if (numRemainingBytes == 0) {
    // This packet contains the last (or only) fragment of the frame.
    // Set the RTP 'M' ('marker') bit, if appropriate:
    if (fSetMBitOnLastFrames) setMarkerBit();
  }

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
					     frameStart, numBytesInFrame,
					     frameTimestamp,
					     numRemainingBytes);
}

Boolean SimpleRTPSink::
frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
			       unsigned /*numBytesInFrame*/) const {
  return fAllowMultipleFramesPerPacket;
}

char const* SimpleRTPSink::sdpMediaType() const {
  return fSDPMediaTypeString;
}
