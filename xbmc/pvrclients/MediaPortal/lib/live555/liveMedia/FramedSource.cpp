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
// Implementation

#include "FramedSource.hh"
#include <stdlib.h>

////////// FramedSource //////////

FramedSource::FramedSource(UsageEnvironment& env)
  : MediaSource(env),
    fAfterGettingFunc(NULL), fAfterGettingClientData(NULL),
    fOnCloseFunc(NULL), fOnCloseClientData(NULL),
    fIsCurrentlyAwaitingData(False) {
  fPresentationTime.tv_sec = fPresentationTime.tv_usec = 0; // initially
}

FramedSource::~FramedSource() {
}

Boolean FramedSource::isFramedSource() const {
  return True;
}

Boolean FramedSource::lookupByName(UsageEnvironment& env, char const* sourceName,
				   FramedSource*& resultSource) {
  resultSource = NULL; // unless we succeed

  MediaSource* source;
  if (!MediaSource::lookupByName(env, sourceName, source)) return False;

  if (!source->isFramedSource()) {
    env.setResultMsg(sourceName, " is not a framed source");
    return False;
  }

  resultSource = (FramedSource*)source;
  return True;
}

void FramedSource::getNextFrame(unsigned char* to, unsigned maxSize,
				afterGettingFunc* afterGettingFunc,
				void* afterGettingClientData,
				onCloseFunc* onCloseFunc,
				void* onCloseClientData) {
  // Make sure we're not already being read:
  if (fIsCurrentlyAwaitingData) {
    envir() << "FramedSource[" << this << "]::getNextFrame(): attempting to read more than once at the same time!\n";
    abort();
  }

  fTo = to;
  fMaxSize = maxSize;
  fNumTruncatedBytes = 0; // by default; could be changed by doGetNextFrame()
  fDurationInMicroseconds = 0; // by default; could be changed by doGetNextFrame()
  fAfterGettingFunc = afterGettingFunc;
  fAfterGettingClientData = afterGettingClientData;
  fOnCloseFunc = onCloseFunc;
  fOnCloseClientData = onCloseClientData;
  fIsCurrentlyAwaitingData = True;

  doGetNextFrame();
}

void FramedSource::afterGetting(FramedSource* source) {
  source->fIsCurrentlyAwaitingData = False;
      // indicates that we can be read again
      // Note that this needs to be done here, in case the "fAfterFunc"
      // called below tries to read another frame (which it usually will)

  if (source->fAfterGettingFunc != NULL) {
    (*(source->fAfterGettingFunc))(source->fAfterGettingClientData,
				   source->fFrameSize, source->fNumTruncatedBytes,
				   source->fPresentationTime,
				   source->fDurationInMicroseconds);
  }
}

void FramedSource::handleClosure(void* clientData) {
  FramedSource* source = (FramedSource*)clientData;
  source->fIsCurrentlyAwaitingData = False; // because we got a close instead
  if (source->fOnCloseFunc != NULL) {
    (*(source->fOnCloseFunc))(source->fOnCloseClientData);
  }
}

void FramedSource::stopGettingFrames() {
  fIsCurrentlyAwaitingData = False; // indicates that we can be read again

  // Perform any specialized action now:
  doStopGettingFrames();
}

void FramedSource::doStopGettingFrames() {
  // Default implementation: Do nothing
  // Subclasses may wish to specialize this so as to ensure that a
  // subsequent reader can pick up where this one left off.
}

unsigned FramedSource::maxFrameSize() const {
  // By default, this source has no maximum frame size.
  return 0;
}
