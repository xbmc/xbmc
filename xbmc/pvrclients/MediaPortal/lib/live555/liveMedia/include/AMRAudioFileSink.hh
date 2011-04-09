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
// AMR Audio File Sinks
// C++ header

#ifndef _AMR_AUDIO_FILE_SINK_HH
#define _AMR_AUDIO_FILE_SINK_HH

#ifndef _FILE_SINK_HH
#include "FileSink.hh"
#endif

class AMRAudioFileSink: public FileSink {
public:
  static AMRAudioFileSink* createNew(UsageEnvironment& env, char const* fileName,
				     unsigned bufferSize = 10000,
				     Boolean oneFilePerFrame = False);
  // (See "FileSink.hh" for a description of these parameters.)

protected:
  AMRAudioFileSink(UsageEnvironment& env, FILE* fid, unsigned bufferSize,
		   char const* perFrameFileNamePrefix);
      // called only by createNew()
  virtual ~AMRAudioFileSink();

protected: // redefined virtual functions:
  virtual Boolean sourceIsCompatibleWithUs(MediaSource& source);
  virtual void afterGettingFrame1(unsigned frameSize,
				  struct timeval presentationTime);

protected:
  Boolean fHaveWrittenHeader;
};

#endif
