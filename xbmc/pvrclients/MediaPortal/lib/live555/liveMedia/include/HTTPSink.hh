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
// HTTP Sinks
// C++ header

#ifndef _HTTP_SINK_HH
#define _HTTP_SINK_HH

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif
#ifndef _NET_ADDRESS_HH
#include "NetAddress.hh"
#endif

class HTTPSink: public MediaSink {
public:
  static HTTPSink* createNew(UsageEnvironment& env, Port ourPort);
  // if ourPort.num() == 0, we'll choose (& return) port

protected:
  HTTPSink(UsageEnvironment& env, int ourSocket); // called only by createNew()
  virtual ~HTTPSink();

  virtual Boolean isUseableFrame(unsigned char* framePtr, unsigned frameSize);
      // by default, True, but can be redefined by subclasses to select
      // which frames get passed to the HTTP client.

private: // redefined virtual functions:
  virtual Boolean continuePlaying();

protected:
  static int setUpOurSocket(UsageEnvironment& env, Port& ourPort);
  static void appendPortNum(UsageEnvironment& env, Port const& port);

private:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				unsigned numTruncatedBytes,
				struct timeval presentationTime,
				unsigned durationInMicroseconds);
  void afterGettingFrame1(unsigned frameSize, struct timeval presentationTime);

  static void ourOnSourceClosure(void* clientData);

  int fSocket;
  unsigned char fBuffer[10000]; // size should really be an attribute of source #####
  int fClientSocket;
};

#endif
