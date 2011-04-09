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
// MPEG-1 or MPEG-2 Audio RTP Sources
// Implementation

#include "MPEG1or2AudioRTPSource.hh"

MPEG1or2AudioRTPSource*
MPEG1or2AudioRTPSource::createNew(UsageEnvironment& env,
			      Groupsock* RTPgs,
			      unsigned char rtpPayloadFormat,
			      unsigned rtpTimestampFrequency) {
  return new MPEG1or2AudioRTPSource(env, RTPgs, rtpPayloadFormat,
				rtpTimestampFrequency);
}

MPEG1or2AudioRTPSource::MPEG1or2AudioRTPSource(UsageEnvironment& env,
				       Groupsock* rtpGS,
				       unsigned char rtpPayloadFormat,
				       unsigned rtpTimestampFrequency)
  : MultiFramedRTPSource(env, rtpGS,
			 rtpPayloadFormat, rtpTimestampFrequency) {
}

MPEG1or2AudioRTPSource::~MPEG1or2AudioRTPSource() {
}

Boolean MPEG1or2AudioRTPSource
::processSpecialHeader(BufferedPacket* packet,
                       unsigned& resultSpecialHeaderSize) {
  // There's a 4-byte header indicating fragmentation.
  if (packet->dataSize() < 4) return False;

  // Note: This fragmentation header is actually useless to us, because
  // it doesn't tell us whether or not this RTP packet *ends* a
  // fragmented frame.  Thus, we can't use it to properly set
  // "fCurrentPacketCompletesFrame".  Instead, we assume that even
  // a partial audio frame will be usable to clients.

  resultSpecialHeaderSize = 4;
  return True;
}

char const* MPEG1or2AudioRTPSource::MIMEtype() const {
  return "audio/MPEG";
}

