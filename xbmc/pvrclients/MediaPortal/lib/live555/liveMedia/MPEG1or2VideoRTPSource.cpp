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
// MPEG-1 or MPEG-2 Video RTP Sources
// Implementation

#include "MPEG1or2VideoRTPSource.hh"

MPEG1or2VideoRTPSource*
MPEG1or2VideoRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			      unsigned char rtpPayloadFormat,
			      unsigned rtpTimestampFrequency) {
  return new MPEG1or2VideoRTPSource(env, RTPgs, rtpPayloadFormat,
				rtpTimestampFrequency);
}

MPEG1or2VideoRTPSource::MPEG1or2VideoRTPSource(UsageEnvironment& env,
				       Groupsock* RTPgs,
				       unsigned char rtpPayloadFormat,
				       unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency){
}

MPEG1or2VideoRTPSource::~MPEG1or2VideoRTPSource() {
}

Boolean MPEG1or2VideoRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  // There's a 4-byte video-specific header
  if (packet->dataSize() < 4) return False;

  u_int32_t header = ntohl(*(unsigned*)(packet->data()));

  u_int32_t sBit = header&0x00002000; // sequence-header-present
  u_int32_t bBit = header&0x00001000; // beginning-of-slice
  u_int32_t eBit = header&0x00000800; // end-of-slice

  fCurrentPacketBeginsFrame = (sBit|bBit) != 0;
  fCurrentPacketCompletesFrame = ((sBit&~bBit)|eBit) != 0;

  resultSpecialHeaderSize = 4;
  return True;
}

Boolean MPEG1or2VideoRTPSource
::packetIsUsableInJitterCalculation(unsigned char* packet,
				    unsigned packetSize) {
  // There's a 4-byte video-specific header
  if (packetSize < 4) return False;

  // Extract the "Picture-Type" field from this, to determine whether
  // this packet can be used in jitter calculations:
  unsigned header = ntohl(*(unsigned*)packet);

  unsigned short pictureType = (header>>8)&0x7;
  if (pictureType == 1) { // an I frame
    return True;
  } else { // a P, B, D, or other unknown frame type
    return False;
  }
}

char const* MPEG1or2VideoRTPSource::MIMEtype() const {
  return "video/MPEG";
}

