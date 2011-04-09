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
// A source that consists of multiple byte-stream files, read sequentially
// Implementation

#include "ByteStreamMultiFileSource.hh"

ByteStreamMultiFileSource
::ByteStreamMultiFileSource(UsageEnvironment& env, char const** fileNameArray,
			    unsigned preferredFrameSize, unsigned playTimePerFrame)
  : FramedSource(env),
    fPreferredFrameSize(preferredFrameSize), fPlayTimePerFrame(playTimePerFrame),
    fCurrentlyReadSourceNumber(0), fHaveStartedNewFile(False) {
    // Begin by counting the number of sources:
    for (fNumSources = 0; ; ++fNumSources) {
      if (fileNameArray[fNumSources] == NULL) break;
    }

    // Next, copy the source file names into our own array:
    fFileNameArray = new char const*[fNumSources];
    if (fFileNameArray == NULL) return;
    unsigned i;
    for (i = 0; i < fNumSources; ++i) {
      fFileNameArray[i] = strDup(fileNameArray[i]);
    }

    // Next, set up our array of component ByteStreamFileSources
    // Don't actually create these yet; instead, do this on demand
    fSourceArray = new ByteStreamFileSource*[fNumSources];
    if (fSourceArray == NULL) return;
    for (i = 0; i < fNumSources; ++i) {
      fSourceArray[i] = NULL;
    }
}

ByteStreamMultiFileSource::~ByteStreamMultiFileSource() {
  unsigned i;
  for (i = 0; i < fNumSources; ++i) {
    Medium::close(fSourceArray[i]);
  }
  delete[] fSourceArray;

  for (i = 0; i < fNumSources; ++i) {
    delete[] (char*)(fFileNameArray[i]);
  }
  delete[] fFileNameArray;
}

ByteStreamMultiFileSource* ByteStreamMultiFileSource
::createNew(UsageEnvironment& env, char const** fileNameArray,
	    unsigned preferredFrameSize, unsigned playTimePerFrame) {
  ByteStreamMultiFileSource* newSource
    = new ByteStreamMultiFileSource(env, fileNameArray,
				    preferredFrameSize, playTimePerFrame);

  return newSource;
}

void ByteStreamMultiFileSource::doGetNextFrame() {
  do {
    // First, check whether we've run out of sources:
    if (fCurrentlyReadSourceNumber >= fNumSources) break;

    fHaveStartedNewFile = False;
    ByteStreamFileSource*& source
      = fSourceArray[fCurrentlyReadSourceNumber];
    if (source == NULL) {
      // The current source hasn't been created yet.  Do this now:
      source = ByteStreamFileSource::createNew(envir(),
		       fFileNameArray[fCurrentlyReadSourceNumber],
		       fPreferredFrameSize, fPlayTimePerFrame);
      if (source == NULL) break;
      fHaveStartedNewFile = True;
    }

    // (Attempt to) read from the current source.
    source->getNextFrame(fTo, fMaxSize,
			       afterGettingFrame, this,
			       onSourceClosure, this);
    return;
  } while (0);

  // An error occurred; consider ourselves closed:
  handleClosure(this);
}

void ByteStreamMultiFileSource
  ::afterGettingFrame(void* clientData,
		      unsigned frameSize, unsigned numTruncatedBytes,
		      struct timeval presentationTime,
		      unsigned durationInMicroseconds) {
  ByteStreamMultiFileSource* source
    = (ByteStreamMultiFileSource*)clientData;
  source->fFrameSize = frameSize;
  source->fNumTruncatedBytes = numTruncatedBytes;
  source->fPresentationTime = presentationTime;
  source->fDurationInMicroseconds = durationInMicroseconds;
  FramedSource::afterGetting(source);
}

void ByteStreamMultiFileSource::onSourceClosure(void* clientData) {
  ByteStreamMultiFileSource* source
    = (ByteStreamMultiFileSource*)clientData;
  source->onSourceClosure1();
}

void ByteStreamMultiFileSource::onSourceClosure1() {
  // This routine was called because the currently-read source was closed
  // (probably due to EOF).  Close this source down, and move to the
  // next one:
  ByteStreamFileSource*& source
    = fSourceArray[fCurrentlyReadSourceNumber++];
  Medium::close(source);
  source = NULL;

  // Try reading again:
  doGetNextFrame();
}
