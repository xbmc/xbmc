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
// RTP sink for AMR audio (RFC 3267)
// Implementation

// NOTE: At present, this is just a limited implementation, supporting:
// octet-alignment only; no interleaving; no frame CRC; no robust-sorting.

#include "AMRAudioRTPSink.hh"
#include "AMRAudioSource.hh"

AMRAudioRTPSink*
AMRAudioRTPSink::createNew(UsageEnvironment& env, Groupsock* RTPgs,
			   unsigned char rtpPayloadFormat,
			   Boolean sourceIsWideband,
			   unsigned numChannelsInSource) {
  return new AMRAudioRTPSink(env, RTPgs, rtpPayloadFormat,
			     sourceIsWideband, numChannelsInSource);
}

AMRAudioRTPSink
::AMRAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		  unsigned char rtpPayloadFormat,
		  Boolean sourceIsWideband, unsigned numChannelsInSource)
  : AudioRTPSink(env, RTPgs, rtpPayloadFormat,
		 sourceIsWideband ? 16000 : 8000,
		 sourceIsWideband ? "AMR-WB": "AMR",
		 numChannelsInSource),
  fSourceIsWideband(sourceIsWideband), fFmtpSDPLine(NULL) {
}

AMRAudioRTPSink::~AMRAudioRTPSink() {
  delete[] fFmtpSDPLine;
}

Boolean AMRAudioRTPSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // Our source must be an AMR audio source:
  if (!source.isAMRAudioSource()) return False;

  // Also, the source must be wideband iff we asked for this:
  AMRAudioSource& amrSource = (AMRAudioSource&)source;
  if ((amrSource.isWideband()^fSourceIsWideband) != 0) return False;

  // Also, the source must have the same number of channels that we
  // specified.  (It could, in principle, have more, but we don't
  // support that.)
  if (amrSource.numChannels() != numChannels()) return False;

  // Also, because in our current implementation we output only one
  // frame in each RTP packet, this means that for multi-channel audio,
  // each 'frame-block' will be split over multiple RTP packets, which
  // may violate the spec.  Warn about this:
  if (amrSource.numChannels() > 1) {
    envir() << "AMRAudioRTPSink: Warning: Input source has " << amrSource.numChannels()
	    << " audio channels.  In the current implementation, the multi-frame frame-block will be split over multiple RTP packets\n";
  }

  return True;
}

void AMRAudioRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset,
					     unsigned char* frameStart,
					     unsigned numBytesInFrame,
					     struct timeval frameTimestamp,
					     unsigned numRemainingBytes) {
  // If this is the 1st frame in the 1st packet, set the RTP 'M' (marker)
  // bit (because this is considered the start of a talk spurt):
  if (isFirstPacket() && isFirstFrameInPacket()) {
    setMarkerBit();
  }

  // If this is the first frame in the packet, set the 1-byte payload
  // header (using CMR 15)
  if (isFirstFrameInPacket()) {
    u_int8_t payloadHeader = 0xF0;
    setSpecialHeaderBytes(&payloadHeader, 1, 0);
  }

  // Set the TOC field for the current frame, based on the "FT" and "Q"
  // values from our source:
  AMRAudioSource* amrSource = (AMRAudioSource*)fSource;
  if (amrSource == NULL) return; // sanity check

  u_int8_t toc = amrSource->lastFrameHeader();
  // Clear the "F" bit, because we're the last frame in this packet: #####
  toc &=~ 0x80;
  setSpecialHeaderBytes(&toc, 1, 1+numFramesUsedSoFar());

  // Important: Also call our base class's doSpecialFrameHandling(),
  // to set the packet's timestamp:
  MultiFramedRTPSink::doSpecialFrameHandling(fragmentationOffset,
                                             frameStart, numBytesInFrame,
                                             frameTimestamp,
                                             numRemainingBytes);
}

Boolean AMRAudioRTPSink
::frameCanAppearAfterPacketStart(unsigned char const* /*frameStart*/,
				 unsigned /*numBytesInFrame*/) const {
  // For now, pack only one AMR frame into each outgoing RTP packet: #####
  return False;
}

unsigned AMRAudioRTPSink::specialHeaderSize() const {
  // For now, because we're packing only one frame per packet,
  // there's just a 1-byte payload header, plus a 1-byte TOC #####
  return 2;
}

char const* AMRAudioRTPSink::auxSDPLine() {
  if (fFmtpSDPLine == NULL) {
    // Generate a "a=fmtp:" line with "octet-aligned=1"
    // (That is the only non-default parameter.)
    char buf[100];
    sprintf(buf, "a=fmtp:%d octet-align=1\r\n", rtpPayloadType());
    delete[] fFmtpSDPLine; fFmtpSDPLine = strDup(buf);
  }
  return fFmtpSDPLine;
}
