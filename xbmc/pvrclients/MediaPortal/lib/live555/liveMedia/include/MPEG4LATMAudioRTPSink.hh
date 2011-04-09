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
// RTP sink for MPEG-4 audio, using LATM multiplexing (RFC 3016)
// (Note that the initial 'size' field is assumed to be present at the start of
//  each frame.)
// C++ header

#ifndef _MPEG4_LATM_AUDIO_RTP_SINK_HH
#define _MPEG4_LATM_AUDIO_RTP_SINK_HH

#ifndef _AUDIO_RTP_SINK_HH
#include "AudioRTPSink.hh"
#endif

class MPEG4LATMAudioRTPSink: public AudioRTPSink {
public:
  static MPEG4LATMAudioRTPSink* createNew(UsageEnvironment& env,
					  Groupsock* RTPgs,
					  unsigned char rtpPayloadFormat,
					  u_int32_t rtpTimestampFrequency,
					  char const* streamMuxConfigString,
					  unsigned numChannels,
					  Boolean allowMultipleFramesPerPacket = False);

protected:
  MPEG4LATMAudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs,
			unsigned char rtpPayloadFormat,
			u_int32_t rtpTimestampFrequency,
			char const* streamMuxConfigString,
			unsigned numChannels,
			Boolean allowMultipleFramesPerPacket);
	// called only by createNew()

  virtual ~MPEG4LATMAudioRTPSink();

private: // redefined virtual functions:
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual Boolean
  frameCanAppearAfterPacketStart(unsigned char const* frameStart,
				 unsigned numBytesInFrame) const;

  virtual char const* auxSDPLine(); // for the "a=fmtp:" SDP line

private:
  char const* fStreamMuxConfigString;
  char* fFmtpSDPLine;
  Boolean fAllowMultipleFramesPerPacket;
};

#endif
