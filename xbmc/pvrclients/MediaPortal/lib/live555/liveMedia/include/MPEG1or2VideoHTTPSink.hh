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
// A HTTP Sink specifically for MPEG Video
// C++ header

#ifndef _MPEG_1OR2_VIDEO_HTTP_SINK_HH
#define _MPEG_1OR2_VIDEO_HTTP_SINK_HH

#ifndef _HTTP_SINK_HH
#include "HTTPSink.hh"
#endif

class MPEG1or2VideoHTTPSink: public HTTPSink {
public:
  static MPEG1or2VideoHTTPSink* createNew(UsageEnvironment& env, Port ourPort);
  // if ourPort.num() == 0, we'll choose (& return) port

protected:
  MPEG1or2VideoHTTPSink(UsageEnvironment& env, int ourSocket);
      // called only by createNew()
  virtual ~MPEG1or2VideoHTTPSink();

private: // redefined virtual functions:
  virtual Boolean isUseableFrame(unsigned char* framePtr, unsigned frameSize);

private:
  Boolean fHaveSeenFirstVSH;
};

#endif
