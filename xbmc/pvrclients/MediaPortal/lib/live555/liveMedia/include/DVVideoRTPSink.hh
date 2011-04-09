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
// RTP sink for DV video (RFC 3189)
// (Thanks to Ben Hutchings for prototyping this.)
// C++ header

#ifndef _DV_VIDEO_RTP_SINK_HH
#define _DV_VIDEO_RTP_SINK_HH

#ifndef _VIDEO_RTP_SINK_HH
#include "VideoRTPSink.hh"
#endif
#ifndef _DV_VIDEO_STREAM_FRAMER_HH
#include "DVVideoStreamFramer.hh"
#endif

class DVVideoRTPSink: public VideoRTPSink {
public:
  static DVVideoRTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat);
  char const* auxSDPLineFromFramer(DVVideoStreamFramer* framerSource);

protected:
  DVVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat);
	// called only by createNew()

  virtual ~DVVideoRTPSink();

private: // redefined virtual functions:
  virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);
  virtual void doSpecialFrameHandling(unsigned fragmentationOffset,
                                      unsigned char* frameStart,
                                      unsigned numBytesInFrame,
                                      struct timeval frameTimestamp,
                                      unsigned numRemainingBytes);
  virtual unsigned computeOverflowForNewFrame(unsigned newFrameSize) const;
  virtual char const* auxSDPLine();

private:
  char* fFmtpSDPLine;
};

#endif
