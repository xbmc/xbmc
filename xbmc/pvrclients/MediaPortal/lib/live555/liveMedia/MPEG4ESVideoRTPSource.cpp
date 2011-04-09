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
// MP4V-ES video RTP stream sources
// Implementation

#include "MPEG4ESVideoRTPSource.hh"

///////// MPEG4ESVideoRTPSource implementation ////////

//##### NOTE: INCOMPLETE!!! #####

MPEG4ESVideoRTPSource*
MPEG4ESVideoRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs,
				   unsigned char rtpPayloadFormat,
				   unsigned rtpTimestampFrequency) {
  return new MPEG4ESVideoRTPSource(env, RTPgs, rtpPayloadFormat,
				   rtpTimestampFrequency);
}

MPEG4ESVideoRTPSource
::MPEG4ESVideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
			unsigned char rtpPayloadFormat,
			unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency) {
}

MPEG4ESVideoRTPSource::~MPEG4ESVideoRTPSource() {
}

Boolean MPEG4ESVideoRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  // The packet begins a frame iff its data begins with a system code
  // (i.e., 0x000001??)
  fCurrentPacketBeginsFrame
    = packet->dataSize() >= 4 && (packet->data())[0] == 0
    && (packet->data())[1] == 0 && (packet->data())[2] == 1;

  // The RTP "M" (marker) bit indicates the last fragment of a frame:
  fCurrentPacketCompletesFrame = packet->rtpMarkerBit();

  // There is no special header
  resultSpecialHeaderSize = 0;
  return True;
}

char const* MPEG4ESVideoRTPSource::MIMEtype() const {
  return "video/MP4V-ES";
}
