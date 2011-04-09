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
// File Sinks
// C++ header

#ifndef _FILE_SINK_HH
#define _FILE_SINK_HH

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif

class FileSink: public MediaSink {
public:
  static FileSink* createNew(UsageEnvironment& env, char const* fileName,
			     unsigned bufferSize = 20000,
			     Boolean oneFilePerFrame = False);
  // "bufferSize" should be at least as large as the largest expected
  //   input frame.
  // "oneFilePerFrame" - if True - specifies that each input frame will
  //   be written to a separate file (using the presentation time as a
  //   file name suffix).  The default behavior ("oneFilePerFrame" == False)
  //   is to output all incoming data into a single file.

  void addData(unsigned char* data, unsigned dataSize,
	       struct timeval presentationTime);
  // (Available in case a client wants to add extra data to the output file)

protected:
  FileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
	   char const* perFrameFileNamePrefix);
      // called only by createNew()
  virtual ~FileSink();

protected:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  virtual void afterGettingFrame1(unsigned frameSize,
				  struct timeval presentationTime);

  FILE* fOutFid;
  unsigned char* fBuffer;
  unsigned fBufferSize;
  char* fPerFrameFileNamePrefix; // used if "oneFilePerFrame" is True
  char* fPerFrameFileNameBuffer; // used if "oneFilePerFrame" is True

private: // redefined virtual functions:
  virtual Boolean continuePlaying();
};


#endif
