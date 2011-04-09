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
// A filter that breaks up an MPEG (1,2) audio elementary stream into frames
// C++ header

#ifndef _MPEG_1OR2_AUDIO_STREAM_FRAMER_HH
#define _MPEG_1OR2_AUDIO_STREAM_FRAMER_HH

#ifndef _FRAMED_FILTER_HH
#include "FramedFilter.hh"
#endif

class MPEG1or2AudioStreamFramer: public FramedFilter {
public:
  static MPEG1or2AudioStreamFramer*
  createNew(UsageEnvironment& env, FramedSource* inputSource,
	    Boolean syncWithInputSource = False);
  // If "syncWithInputSource" is True, the stream's presentation time
  // will be reset to that of the input source, whenever new data
  // is read from it.

  void flushInput(); // called if there is a discontinuity (seeking) in the input

private:
  MPEG1or2AudioStreamFramer(UsageEnvironment& env, FramedSource* inputSource,
			    Boolean syncWithInputSource);
      // called only by createNew()
  virtual ~MPEG1or2AudioStreamFramer();

  static void continueReadProcessing(void* clientData,
				     unsigned char* ptr, unsigned size,
				     struct timeval presentationTime);
  void continueReadProcessing();

  void resetPresentationTime(struct timeval newPresentationTime);
      // useful if we're being synced with a separate (e.g., video) stream

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  void reset();
  struct timeval currentFramePlayTime() const;

private:
  Boolean fSyncWithInputSource;
  struct timeval fNextFramePresentationTime;

private: // parsing state
  class MPEG1or2AudioStreamParser* fParser;
  friend class MPEG1or2AudioStreamParser; // hack
};

#endif
