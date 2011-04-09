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
// RTP sink for 'ADUized' MP3 frames ("mpa-robust")
// C++ header

#ifndef _MP3_ADU_RTP_SINK_HH
#define _MP3_ADU_RTP_SINK_HH

#ifndef _AUDIO_RTP_SINK_HH
#include "AudioRTPSink.hh"
#endif

class MP3ADURTPSink: public AudioRTPSink {
public:
  static MP3ADURTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs,
				  unsigned char RTPPayloadType);

protected:
  virtual ~MP3ADURTPSink();

private:
  MP3ADURTPSink(UsageEnvironment& env, Groupsock* RTPgs,
		unsigned char RTPPayloadType);
	// called only by createNew()


private:
  // Redefined virtual functions:
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual unsigned specialHeaderSize() const;

private:
  unsigned fCurADUSize; // used when fragmenting over multiple RTP packets
};

#endif
