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
// A simplified version of "MPEG1or2VideoStreamFramer" that takes only
// complete, discrete frames (rather than an arbitrary byte stream) as input.
// This avoids the parsing and data copying overhead of the full
// "MPEG1or2VideoStreamFramer".
// C++ header

#ifndef _MPEG1or2_VIDEO_STREAM_DISCRETE_FRAMER_HH
#define _MPEG1or2_VIDEO_STREAM_DISCRETE_FRAMER_HH

#ifndef _MPEG1or2_VIDEO_STREAM_FRAMER_HH
#include "MPEG1or2VideoStreamFramer.hh"
#endif

#define VSH_MAX_SIZE 1000

class MPEG1or2VideoStreamDiscreteFramer: public MPEG1or2VideoStreamFramer {
public:
  static MPEG1or2VideoStreamDiscreteFramer*
  createNew(UsageEnvironment& env, FramedSource* inputSource,
            Boolean iFramesOnly = False,
            double vshPeriod = 5.0); // see MPEG1or2VideoStreamFramer.hh

private:
  MPEG1or2VideoStreamDiscreteFramer(UsageEnvironment& env,
                                    FramedSource* inputSource,
                                    Boolean iFramesOnly, double vshPeriod);
  // called only by createNew()
  virtual ~MPEG1or2VideoStreamDiscreteFramer();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize,
                          unsigned numTruncatedBytes,
                          struct timeval presentationTime,
                          unsigned durationInMicroseconds);

private:
  struct timeval fLastNonBFramePresentationTime;
  unsigned fLastNonBFrameTemporal_reference;

  // A saved copy of the most recently seen 'video_sequence_header',
  // in case we need to insert it into the stream periodically:
  unsigned char fSavedVSHBuffer[VSH_MAX_SIZE];
  unsigned fSavedVSHSize;
  double fSavedVSHTimestamp;
  Boolean fIFramesOnly;
  double fVSHPeriod;
};

#endif
