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
// 'ADU' MP3 streams (for improved loss-tolerance)
// C++ header

#ifndef _MP3_ADU_HH
#define _MP3_ADU_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

class ADUFromMP3Source: public FramedFilter {
public:
  static ADUFromMP3Source* createNew(UsageEnvironment& env,
				     FramedSource* inputSource,
				     Boolean includeADUdescriptors = True);

  void resetInput();
      // This is called whenever there's a discontinuity in the input MP3 source
      // (e.g., due to seeking within the source).  It causes any still-unprocessed
      // MP3 frame data within our queue to be discarded, so that it does not
      // erroneously get used by backpointers from the new MP3 frames.

  Boolean setScaleFactor(int scale);

protected:
  ADUFromMP3Source(UsageEnvironment& env,
		   FramedSource* inputSource,
		   Boolean includeADUdescriptors);
      // called only by createNew()
  virtual ~ADUFromMP3Source();

private:
  // Redefined virtual functions:
  virtual void doGetNextFrame();
  virtual char const* MIMEtype() const;

private:
  Boolean doGetNextFrame1();

private:
  Boolean fAreEnqueueingMP3Frame;
  class SegmentQueue* fSegments;
  Boolean fIncludeADUdescriptors;
  unsigned fTotalDataSizeBeforePreviousRead;
  int fScale;
  unsigned fFrameCounter;
};

class MP3FromADUSource: public FramedFilter {
public:
  static MP3FromADUSource* createNew(UsageEnvironment& env,
				     FramedSource* inputSource,
                                     Boolean includeADUdescriptors = True);

protected:
  MP3FromADUSource(UsageEnvironment& env,
		   FramedSource* inputSource,
		   Boolean includeADUdescriptors);
      // called only by createNew()
  virtual ~MP3FromADUSource();

private:
  // Redefined virtual functions:
  virtual void doGetNextFrame();
  virtual char const* MIMEtype() const;

private:
  Boolean needToGetAnADU();
  void insertDummyADUsIfNecessary();
  Boolean generateFrameFromHeadADU();

private:
  Boolean fAreEnqueueingADU;
  class SegmentQueue* fSegments;
  Boolean fIncludeADUdescriptors;
};

// Definitions of external C functions that implement various MP3 operations:
extern "C" int mp3ZeroOutSideInfo(unsigned char*, unsigned, unsigned);

#endif
