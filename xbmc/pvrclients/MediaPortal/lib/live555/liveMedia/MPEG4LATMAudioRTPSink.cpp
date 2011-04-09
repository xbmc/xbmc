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
// RTP sink for MPEG-4 audio, using LATM multiplexing (RFC 3016)
// Implementation

#include "MPEG4LATMAudioRTPSink.hh"

MPEG4LATMAudioRTPSink
::MPEG4LATMAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
			u_int8_t rtpPayloadFormat,
			u_int32_t rtpTimestampFrequency,
			char const* streamMuxConfigString,
			unsigned numChannels,
			Boolean allowMultipleFramesPerPacket)
  : AudioRTPSink(env, RTPgs, rtpPayloadFormat,
		 rtpTimestampFrequency, "MP4A-LATM", numChannels),
  fStreamMuxConfigString(strDup(streamMuxConfigString)),
  fAllowMultipleFramesPerPacket(allowMultipleFramesPerPacket) {
  // Set up the "a=fmtp:" SDP line for this stream:
  char const* fmtpFmt =
    "a=fmtp:%d "
    "cpresent=0;config=%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + strlen(fStreamMuxConfigString);
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
	  rtpPayloadType(),
	  fStreamMuxConfigString);
  fFmtpSDPLine = strDup(fmtp);
  delete[] fmtp;
}

MPEG4LATMAudioRTPSink::~MPEG4LATMAudioRTPSink() {
  delete[] fFmtpSDPLine;
  delete[] (char*)fStreamMuxConfigString;
}

MPEG4LATMAudioRTPSink*
MPEG4LATMAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
				 u_int8_t rtpPayloadFormat,
				 u_int32_t rtpTimestampFrequency,
				 char const* streamMuxConfigString,
				 unsigned numChannels,
				 Boolean allowMultipleFramesPerPacket) {
  return new MPEG4LATMAudioRTPSink(env, RTPgs, rtpPayloadFormat,
				   rtpTimestampFrequency, streamMuxConfigString,
				   numChannels,
				   allowMultipleFramesPerPacket);
}

Boolean MPEG4LATMAudioRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
                                 unsigned /*numBytesInFrame*/) const {
  return fAllowMultipleFramesPerPacket;
}

void MPEG4LATMAudioRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval frameTimestamp,
			 unsigned numRemainingBytes) {
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

char const* MPEG4LATMAudioRTPSink::auxSDPLine() {
  return fFmtpSDPLine;
}
