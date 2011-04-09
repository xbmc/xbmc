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
// A filter that breaks up an H263 video elementary stream into frames.
// Author Benhard Feiten

#ifndef _H263PLUS_VIDEO_STREAM_FRAMER_HH
#define _H263PLUS_VIDEO_STREAM_FRAMER_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif


class H263plusVideoStreamFramer: public FramedFilter {
public:

  static H263plusVideoStreamFramer* createNew(UsageEnvironment& env, FramedSource* inputSource);

  Boolean& pictureEndMarker() { return fPictureEndMarker; }    // a hack for implementing the RTP 'M' bit

protected:
  // Constructor called only by createNew(), or by subclass constructors
  H263plusVideoStreamFramer(UsageEnvironment& env,
			                FramedSource* inputSource,
			                Boolean createParser = True);
  virtual ~H263plusVideoStreamFramer();


public:
  static void continueReadProcessing(void* clientData,
				     unsigned char* ptr, unsigned size,
				     struct timeval presentationTime);
  void continueReadProcessing();

private:
  virtual void doGetNextFrame();
  virtual Boolean isH263plusVideoStreamFramer() const;

protected:
  double   fFrameRate;    // Note: For MPEG-4, this is really a 'tick rate' ??
  unsigned fPictureCount; // hack used to implement doGetNextFrame() ??
  Boolean  fPictureEndMarker;

private:
  class H263plusVideoStreamParser* fParser;
  struct timeval fPresentationTimeBase;
};

#endif
