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
// RTP sink for MPEG audio (RFC 2250)
// C++ header

#ifndef _MPEG_1OR2_AUDIO_RTP_SINK_HH
#define _MPEG_1OR2_AUDIO_RTP_SINK_HH

#ifndef _AUDIO_RTP_SINK_HH
#include "AudioRTPSink.hh"
#endif

class MPEG1or2AudioRTPSink: public AudioRTPSink {
public:
  static MPEG1or2AudioRTPSink* createNew(UsageEnvironment& env,
				     Groupsock* RTPgs);

protected:
  MPEG1or2AudioRTPSink(UsageEnvironment& env, Groupsock* RTPgs);
	// called only by createNew()

  virtual ~MPEG1or2AudioRTPSink();

private: // redefined virtual functions:
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual unsigned specialHeaderSize() const;
};

#endif
