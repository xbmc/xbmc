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
// MPEG4-GENERIC ("audio", "video", or "application") RTP stream sinks
// Implementation

#include "MPEG4GenericRTPSink.hh"

MPEG4GenericRTPSink
::MPEG4GenericRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		      u_int8_t rtpPayloadFormat,
		      u_int32_t rtpTimestampFrequency,
		      char const* sdpMediaTypeString,
		      char const* mpeg4Mode, char const* configString,
		      unsigned numChannels)
  : MultiFramedRTPSink(env, RTPgs, rtpPayloadFormat,
		       rtpTimestampFrequency, "MPEG4-GENERIC", numChannels),
  fSDPMediaTypeString(strDup(sdpMediaTypeString)),
  fMPEG4Mode(strDup(mpeg4Mode)), fConfigString(strDup(configString)) {
  // Check whether "mpeg4Mode" is one that we handle:
  if (mpeg4Mode == NULL) {
    env << "MPEG4GenericRTPSink error: NULL \"mpeg4Mode\" parameter\n";
  } else if (strcmp(mpeg4Mode, "AAC-hbr") != 0) {
    env << "MPEG4GenericRTPSink error: Unknown \"mpeg4Mode\" parameter: \""
	<< mpeg4Mode << "\"\n";
  }

  // Set up the "a=fmtp:" SDP line for this stream:
  char const* fmtpFmt =
    "a=fmtp:%d "
    "streamtype=%d;profile-level-id=1;"
    "mode=%s;sizelength=13;indexlength=3;indexdeltalength=3;"
    "config=%s\r\n";
  unsigned fmtpFmtSize = strlen(fmtpFmt)
    + 3 /* max char len */
    + 3 /* max char len */
    + strlen(fMPEG4Mode)
    + strlen(fConfigString);
  char* fmtp = new char[fmtpFmtSize];
  sprintf(fmtp, fmtpFmt,
	  rtpPayloadType(),
	  strcmp(fSDPMediaTypeString, "video") == 0 ? 4 : 5,
	  fMPEG4Mode,
	  fConfigString);
  fFmtpSDPLine = strDup(fmtp);
  delete[] fmtp;
}

MPEG4GenericRTPSink::~MPEG4GenericRTPSink() {
  delete[] fFmtpSDPLine;
  delete[] (char*)fConfigString;
  delete[] (char*)fMPEG4Mode;
  delete[] (char*)fSDPMediaTypeString;
}

MPEG4GenericRTPSink*
MPEG4GenericRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			       u_int8_t rtpPayloadFormat,
			       u_int32_t rtpTimestampFrequency,
			       char const* sdpMediaTypeString,
			       char const* mpeg4Mode,
			       char const* configString, unsigned numChannels) {
  return new MPEG4GenericRTPSink(env, RTPgs, rtpPayloadFormat,
				 rtpTimestampFrequency,
				 sdpMediaTypeString, mpeg4Mode,
				 configString, numChannels);
}

Boolean MPEG4GenericRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
                                 unsigned /*numBytesInFrame*/) const {
  // (For now) allow at most 1 frame in a single packet:
  return False;
}

void MPEG4GenericRTPSink
::doSpecialFrameHandling(unsigned fragmentationOffset,
			 unsigned char* frameStart,
			 unsigned numBytesInFrame,
			 struct timeval frameTimestamp,
			 unsigned numRemainingBytes) {
  // Set the "AU Header Section".  This is 4 bytes: 2 bytes for the
  // initial "AU-headers-length" field, and 2 bytes for the first
  // (and only) "AU Header":
  unsigned fullFrameSize
    = fragmentationOffset + numBytesInFrame + numRemainingBytes;
  unsigned char headers[4];
  headers[0] = 0; headers[1] = 16 /* bits */; // AU-headers-length
  headers[2] = fullFrameSize >> 5; headers[3] = (fullFrameSize&0x1F)<<3;

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

unsigned MPEG4GenericRTPSink::specialHeaderSize() const {
  return 2 + 2;
}

char const* MPEG4GenericRTPSink::sdpMediaType() const {
  return fSDPMediaTypeString;
}

char const* MPEG4GenericRTPSink::auxSDPLine() {
  return fFmtpSDPLine;
}
