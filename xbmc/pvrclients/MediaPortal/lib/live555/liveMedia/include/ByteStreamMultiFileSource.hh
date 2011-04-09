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
// C++ header

#ifndef _BYTE_STREAM_MULTI_FILE_SOURCE_HH
#define _BYTE_STREAM_MULTI_FILE_SOURCE_HH

#ifndef _BYTE_STREAM_FILE_SOURCE_HH
#include "ByteStreamFileSource.hh"
#endif

class ByteStreamMultiFileSource: public FramedSource {
public:
  static ByteStreamMultiFileSource*
  createNew(UsageEnvironment& env, char const** fileNameArray,
	    unsigned preferredFrameSize = 0, unsigned playTimePerFrame = 0);
  // A 'filename' of NULL indicates the end of the array

  Boolean haveStartedNewFile() const { return fHaveStartedNewFile; }
  // True iff the most recently delivered frame was the first from a newly-opened file

protected:
  ByteStreamMultiFileSource(UsageEnvironment& env, char const** fileNameArray,
			    unsigned preferredFrameSize, unsigned playTimePerFrame);
	// called only by createNew()

  virtual ~ByteStreamMultiFileSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  static void onSourceClosure(void* clientData);
  void onSourceClosure1();
  static void afterGettingFrame(void* clientData,
				unsigned frameSize, unsigned numTruncatedBytes,
                                struct timeval presentationTime,
				unsigned durationInMicroseconds);

private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  unsigned fNumSources;
  unsigned fCurrentlyReadSourceNumber;
  Boolean fHaveStartedNewFile;
  char const** fFileNameArray;
  ByteStreamFileSource** fSourceArray;
};

#endif
