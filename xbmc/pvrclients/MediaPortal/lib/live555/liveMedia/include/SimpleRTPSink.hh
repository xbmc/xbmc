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
// A simple RTP sink that packs frames into each outgoing
//     packet, without any fragmentation or special headers.
// C++ header

#ifndef _SIMPLE_RTP_SINK_HH
#define _SIMPLE_RTP_SINK_HH

#ifndef _MULTI_FRAMED_RTP_SINK_HH
#include "MultiFramedRTPSink.hh"
#endif

class SimpleRTPSink: public MultiFramedRTPSink {
public:
  static SimpleRTPSink*
  createNew(UsageEnvironment& env, Groupsock* RTPgs,
	    unsigned char rtpPayloadFormat,
	    unsigned rtpTimestampFrequency,
	    char const* sdpMediaTypeString,
	    char const* rtpPayloadFormatName,
	    unsigned numChannels = 1,
	    Boolean allowMultipleFramesPerPacket = True,
	    Boolean doNormalMBitRule = True);
  // "doNormalMBitRule" means: If the medium is video, set the RTP "M"
  // bit on each outgoing packet iff it contains the last (or only)
  // fragment of a frame.  (Otherwise, leave the "M" bit unset.)
protected:
  SimpleRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		unsigned char rtpPayloadFormat,
		unsigned rtpTimestampFrequency,
		char const* sdpMediaTypeString,
		char const* rtpPayloadFormatName,
		unsigned numChannels,
		Boolean allowMultipleFramesPerPacket,
		Boolean doNormalMBitRule);
	// called only by createNew()

  virtual ~SimpleRTPSink();

protected: // redefined virtual functions
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual
  Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
					 unsigned numBytesInFrame) const;
  virtual char const* sdpMediaType() const;

private:
  char const* fSDPMediaTypeString;
  Boolean fAllowMultipleFramesPerPacket;
  Boolean fSetMBitOnLastFrames;
};

#endif
