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
// RTP sink for AC3 audio
// C++ header

#ifndef _AC3_AUDIO_RTP_SINK_HH
#define _AC3_AUDIO_RTP_SINK_HH

#ifndef _AUDIO_RTP_SINK_HH
#include "AudioRTPSink.hh"
#endif

class AC3AudioRTPSink: public AudioRTPSink {
public:
  static AC3AudioRTPSink* createNew(UsageEnvironment& env,
				    Groupsock* RTPgs,
				    u_int8_t rtpPayloadFormat,
				    u_int32_t rtpTimestampFrequency);

protected:
  AC3AudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		  u_int8_t rtpPayloadFormat,
		  u_int32_t rtpTimestampFrequency);
	// called only by createNew()

  virtual ~AC3AudioRTPSink();

private: // redefined virtual functions:
  virtual
  Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart,
					 unsigned numBytesInFrame) const;
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual unsigned specialHeaderSize() const;
};

#endif
