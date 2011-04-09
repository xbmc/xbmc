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
// AC3 Audio RTP Sources
// Implementation

#include "AC3AudioRTPSource.hh"

AC3AudioRTPSource*
AC3AudioRTPSource::createNew(UsageEnvironment& env,
			      Groupsock* RTPgs,
			      unsigned char rtpPayloadFormat,
			      unsigned rtpTimestampFrequency) {
  return new AC3AudioRTPSource(env, RTPgs, rtpPayloadFormat,
				rtpTimestampFrequency);
}

AC3AudioRTPSource::AC3AudioRTPSource(UsageEnvironment& env,
				       Groupsock* rtpGS,
				       unsigned char rtpPayloadFormat,
				       unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, rtpGS,
			 rtpPayloadFormat, rtpTimestampFrequency) {
}

AC3AudioRTPSource::~AC3AudioRTPSource() {
}

Boolean AC3AudioRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  unsigned char* headerStart = packet->data();
  unsigned packetSize = packet->dataSize();

  // There's a 1-byte "NDU" header, containing the number of frames
  // present in this RTP packet.
  if (packetSize < 2) return False;
  unsigned char numFrames = headerStart[0];
  if (numFrames == 0) return False;

  // TEMP: We can't currently handle packets containing > 1 frame #####
  if (numFrames > 1) {
    envir() << "AC3AudioRTPSource::processSpecialHeader(): packet contains "
	    << numFrames << " frames (we can't handle this!)\n";
    return False;
  }

  // We current can't handle packets that consist only of redundant data:
  unsigned char typ_field = headerStart[1] >> 6;
  if (typ_field >= 2) return False;

  fCurrentPacketBeginsFrame = fCurrentPacketCompletesFrame;
  // whether the *previous* packet ended a frame

  // The RTP "M" (marker) bit indicates the last fragment of a frame:
  fCurrentPacketCompletesFrame = packet->rtpMarkerBit();

  resultSpecialHeaderSize = 2;
  return True;
}

char const* AC3AudioRTPSource::MIMEtype() const {
  return "audio/AC3";
}

