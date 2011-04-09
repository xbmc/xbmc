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
// DV Video RTP Sources
// Implementation

#include "DVVideoRTPSource.hh"

DVVideoRTPSource*
DVVideoRTPSource::createNew(UsageEnvironment& env,
			    Groupsock* RTPgs,
			    unsigned char rtpPayloadFormat,
			    unsigned rtpTimestampFrequency) {
  return new DVVideoRTPSource(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency);
}

DVVideoRTPSource::DVVideoRTPSource(UsageEnvironment& env,
				   Groupsock* rtpGS,
				   unsigned char rtpPayloadFormat,
				   unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, rtpGS,
			 rtpPayloadFormat, rtpTimestampFrequency) {
}

DVVideoRTPSource::~DVVideoRTPSource() {
}

#define DV_DIF_BLOCK_SIZE 80
#define DV_SECTION_HEADER 0x1F

Boolean DVVideoRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  unsigned const packetSize = packet->dataSize();
  if (packetSize < DV_DIF_BLOCK_SIZE) return False; // TARFU!
  
  u_int8_t const* data = packet->data();
  fCurrentPacketBeginsFrame = data[0] == DV_SECTION_HEADER && (data[1]&0xf8) == 0 && data[2] == 0; // thanks to Ben Hutchings

  // The RTP "M" (marker) bit indicates the last fragment of a frame:
  fCurrentPacketCompletesFrame = packet->rtpMarkerBit();

  // There is no special header
  resultSpecialHeaderSize = 0;
  return True;
}

char const* DVVideoRTPSource::MIMEtype() const {
  return "video/DV";
}

