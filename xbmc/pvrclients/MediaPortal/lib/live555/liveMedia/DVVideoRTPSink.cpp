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
// RTP sink for DV video (RFC 3189)
// (Thanks to Ben Hutchings for prototyping this.)
// Implementation

#include "DVVideoRTPSink.hh"

////////// DVVideoRTPSink implementation //////////

DVVideoRTPSink
::DVVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat)
  : VideoRTPSink(env, RTPgs, rtpPayloadFormat, 90000, "DV"),
    fFmtpSDPLine(NULL) {
}

DVVideoRTPSink::~DVVideoRTPSink() {
  delete[] fFmtpSDPLine;
}

DVVideoRTPSink*
DVVideoRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat) {
  return new DVVideoRTPSink(env, RTPgs, rtpPayloadFormat);
}

Boolean DVVideoRTPSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // Our source must be an appropriate framer:
  return source.isDVVideoStreamFramer();
}

void DVVideoRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
					      unsigned char* /*frameStart*/,
					      unsigned /*numBytesInFrame*/,
					      struct timeval frameTimestamp,
					      unsigned numRemainingBytes) {
  if (numRemainingBytes == 0) {
    // This packet contains the last (or only) fragment of the frame.
    // Set the RTP 'M' ('marker') bit:
    setMarkerBit();
  }

  // Also set the RTP timestamp:
  setTimestamp(frameTimestamp);
}

unsigned DVVideoRTPSink::computeOverflowForNewFrame(unsigned newFrameSize) const {
  unsigned initialOverflow = MultiFramedRTPSink::computeOverflowForNewFrame(newFrameSize);

  // Adjust (increase) this overflow, if necessary, so that the amount of frame data that we use is an integral number
  // of DIF blocks:
  unsigned numFrameBytesUsed = newFrameSize - initialOverflow;
  initialOverflow += numFrameBytesUsed%DV_DIF_BLOCK_SIZE;

  return initialOverflow;
}

char const* DVVideoRTPSink::auxSDPLine() {
  // Generate a new "a=fmtp:" line each time, using parameters from
  // our framer source (in case they've changed since the last time that
  // we were called):
  DVVideoStreamFramer* framerSource = (DVVideoStreamFramer*)fSource;
  if (framerSource == NULL) return NULL; // we don't yet have a source

  return auxSDPLineFromFramer(framerSource);
}

char const* DVVideoRTPSink::auxSDPLineFromFramer(DVVideoStreamFramer* framerSource) {
  char const* const profileName = framerSource->profileName();
  if (profileName == NULL) return NULL;

  char const* const fmtpSDPFmt = "a=fmtp:%d encode=%s;audio=bundled\r\n";
  unsigned fmtpSDPFmtSize = strlen(fmtpSDPFmt)
    + 3 // max payload format code length
    + strlen(profileName);
  delete[] fFmtpSDPLine; // if it already exists
  fFmtpSDPLine = new char[fmtpSDPFmtSize];  
  sprintf(fFmtpSDPLine, fmtpSDPFmt, rtpPayloadType(), profileName);

  return fFmtpSDPLine;
}
