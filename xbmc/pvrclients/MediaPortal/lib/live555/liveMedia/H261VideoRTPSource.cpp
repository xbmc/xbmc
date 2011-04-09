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
// H.261 Video RTP Sources
// Implementation

#include "H261VideoRTPSource.hh"

H261VideoRTPSource*
H261VideoRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
				  unsigned char rtpPayloadFormat,
				  unsigned rtpTimestampFrequency) {
  return new H261VideoRTPSource(env, RTPgs, rtpPayloadFormat,
				    rtpTimestampFrequency);
}

H261VideoRTPSource
::H261VideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			 unsigned char rtpPayloadFormat,
			 unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency),
  fLastSpecialHeader(0) {
}

H261VideoRTPSource::~H261VideoRTPSource() {
}

Boolean H261VideoRTPSource
::processSpecialHeader(BufferedPacket* packet,
                       unsigned& resultSpecialHeaderSize) {
  // There's a 4-byte video-specific header
  if (packet->dataSize() < 4) return False;

  unsigned char* headerStart = packet->data();
  fLastSpecialHeader
    = (headerStart[0]<<24)|(headerStart[1]<<16)|(headerStart[2]<<8)|headerStart[3];

#ifdef DELIVER_COMPLETE_FRAMES
  fCurrentPacketBeginsFrame = fCurrentPacketCompletesFrame;
  // whether the *previous* packet ended a frame

  // The RTP "M" (marker) bit indicates the last fragment of a frame:
  fCurrentPacketCompletesFrame = packet->rtpMarkerBit();
#endif

  resultSpecialHeaderSize = 4;
  return True;
}

char const* H261VideoRTPSource::MIMEtype() const {
  return "video/H261";
}
