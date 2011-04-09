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
// A RTP source for a simple RTP payload format that
//     - doesn't have any special headers following the RTP header
//     - doesn't have any special framing apart from the packet data itself
// Implementation

#include "SimpleRTPSource.hh"
#include <string.h>

SimpleRTPSource*
SimpleRTPSource::createNew(UsageEnvironment& env,
			   Groupsock* RTPgs,
			   unsigned char rtpPayloadFormat,
			   unsigned rtpTimestampFrequency,
			   char const* mimeTypeString,
			   unsigned offset, Boolean doNormalMBitRule) {
  return new SimpleRTPSource(env, RTPgs, rtpPayloadFormat,
			     rtpTimestampFrequency,
			     mimeTypeString, offset, doNormalMBitRule);
}

SimpleRTPSource
::SimpleRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned char rtpPayloadFormat,
		  unsigned rtpTimestampFrequency,
		  char const* mimeTypeString,
		  unsigned offset, Boolean doNormalMBitRule)
  : MultiFramedRTPSource(env, RTPgs,
			 rtpPayloadFormat, rtpTimestampFrequency),
    fMIMEtypeString(strDup(mimeTypeString)), fOffset(offset) {
  fUseMBitForFrameEnd
    = strncmp(mimeTypeString, "video/", 6) == 0 && doNormalMBitRule;
}

SimpleRTPSource::~SimpleRTPSource() {
  delete[] (char*)fMIMEtypeString;
}

Boolean SimpleRTPSource
::processSpecialHeader(BufferedPacket* packet,
		       unsigned& resultSpecialHeaderSize) {
  fCurrentPacketCompletesFrame
    = !fUseMBitForFrameEnd || packet->rtpMarkerBit();

  resultSpecialHeaderSize = fOffset;
  return True;
}

char const* SimpleRTPSource::MIMEtype() const {
  if (fMIMEtypeString == NULL) return MultiFramedRTPSource::MIMEtype();

  return fMIMEtypeString;
}
