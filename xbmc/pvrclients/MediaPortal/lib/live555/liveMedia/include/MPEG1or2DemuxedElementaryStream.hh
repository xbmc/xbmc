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
// A MPEG 1 or 2 Elementary Stream, demultiplexed from a Program Stream
// C++ header

#ifndef _MPEG_1OR2_DEMUXED_ELEMENTARY_STREAM_HH
#define _MPEG_1OR2_DEMUXED_ELEMENTARY_STREAM_HH

#ifndef _MPEG_1OR2_DEMUX_HH
#include "MPEG1or2Demux.hh"
#endif

class MPEG1or2DemuxedElementaryStream: public FramedSource {
public:
  MPEG1or2Demux::SCR lastSeenSCR() const { return fLastSeenSCR; }

  unsigned char mpegVersion() const { return fMPEGversion; }

  MPEG1or2Demux& sourceDemux() const { return fOurSourceDemux; }

private: // We are created only by a MPEG1or2Demux (a friend)
  MPEG1or2DemuxedElementaryStream(UsageEnvironment& env,
			      u_int8_t streamIdTag,
			      MPEG1or2Demux& sourceDemux);
  virtual ~MPEG1or2DemuxedElementaryStream();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();
  virtual char const* MIMEtype() const;
  virtual unsigned maxFrameSize() const;

private:
  static void afterGettingFrame(void* clientData,
				unsigned frameSize, unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);

  void afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
			  struct timeval presentationTime,
			  unsigned durationInMicroseconds);

private:
  u_int8_t fOurStreamIdTag;
  MPEG1or2Demux& fOurSourceDemux;
  char const* fMIMEtype;
  MPEG1or2Demux::SCR fLastSeenSCR;
  unsigned char fMPEGversion;

  friend class MPEG1or2Demux;
};

#endif
