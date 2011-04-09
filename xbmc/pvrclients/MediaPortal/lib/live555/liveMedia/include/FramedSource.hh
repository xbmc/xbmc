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
// Framed Sources
// C++ header

#ifndef _FRAMED_SOURCE_HH
#define _FRAMED_SOURCE_HH

#ifndef _NET_COMMON_H
#include "NetCommon.h"
#endif
#ifndef _MEDIA_SOURCE_HH
#include "MediaSource.hh"
#endif

class FramedSource: public MediaSource {
public:
  static Boolean lookupByName(UsageEnvironment& env, char const* sourceName,
			      FramedSource*& resultSource);

  typedef void (afterGettingFunc)(void* clientData, unsigned frameSize,
				  unsigned numTruncatedBytes,
				  struct timeval presentationTime,
				  unsigned durationInMicroseconds);
  typedef void (onCloseFunc)(void* clientData);
  void getNextFrame(unsigned char* to, unsigned maxSize,
		    afterGettingFunc* afterGettingFunc,
		    void* afterGettingClientData,
		    onCloseFunc* onCloseFunc,
		    void* onCloseClientData);

  static void handleClosure(void* clientData);
      // This should be called (on ourself) if the source is discovered
      // to be closed (i.e., no longer readable)

  void stopGettingFrames();

  virtual unsigned maxFrameSize() const;
      // size of the largest possible frame that we may serve, or 0
      // if no such maximum is known (default)

  virtual void doGetNextFrame() = 0;
      // called by getNextFrame()

  Boolean isCurrentlyAwaitingData() const {return fIsCurrentlyAwaitingData;}

protected:
  FramedSource(UsageEnvironment& env); // abstract base class
  virtual ~FramedSource();

  static void afterGetting(FramedSource* source);
      // doGetNextFrame() should arrange for this to be called after the
      // frame has been read (*iff* it is read successfully)

  virtual void doStopGettingFrames();

protected:
  // The following variables are typically accessed/set by doGetNextFrame()
  unsigned char* fTo; // in
  unsigned fMaxSize; // in
  unsigned fFrameSize; // out
  unsigned fNumTruncatedBytes; // out
  struct timeval fPresentationTime; // out
  unsigned fDurationInMicroseconds; // out

private:
  // redefined virtual functions:
  virtual Boolean isFramedSource() const;

private:
  afterGettingFunc* fAfterGettingFunc;
  void* fAfterGettingClientData;
  onCloseFunc* fOnCloseFunc;
  void* fOnCloseClientData;

  Boolean fIsCurrentlyAwaitingData;
};

#endif
