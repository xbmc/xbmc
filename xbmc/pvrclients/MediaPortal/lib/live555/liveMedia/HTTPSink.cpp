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
// HTTP sinks
// Implementation

#include "HTTPSink.hh"
#include "GroupsockHelper.hh"

#include <string.h>
#if defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#endif

////////// HTTPSink //////////

HTTPSink* HTTPSink::createNew(UsageEnvironment& env, Port ourPort) {
  int ourSocket = -1;

  do {
    int ourSocket = setUpOurSocket(env, ourPort);
    if (ourSocket == -1) break;

    HTTPSink* newSink = new HTTPSink(env, ourSocket);
    if (newSink == NULL) break;

    appendPortNum(env, ourPort);

    return newSink;
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return NULL;
}

int HTTPSink::setUpOurSocket(UsageEnvironment& env, Port& ourPort) {
  int ourSocket = -1;

  do {
    ourSocket = setupStreamSocket(env, ourPort);
    if (ourSocket < 0) break;

    // Make sure we have a big send buffer:
    if (!increaseSendBufferTo(env, ourSocket, 50*1024)) break;

    if (listen(ourSocket, 1) < 0) { // we allow only one connection
      env.setResultErrMsg("listen() failed: ");
      break;
    }

    if (ourPort.num() == 0) {
      // bind() will have chosen a port for us; return it also:
      if (!getSourcePort(env, ourSocket, ourPort)) break;
    }

    return ourSocket;
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return -1;
}

void HTTPSink::appendPortNum(UsageEnvironment& env,
			     Port const& port) {
  char tmpBuf[10]; // large enough to hold a port # string
  sprintf(tmpBuf, " %d", ntohs(port.num()));
  env.appendToResultMsg(tmpBuf);
}


HTTPSink::HTTPSink(UsageEnvironment& env, int ourSocket)
  : MediaSink(env), fSocket(ourSocket), fClientSocket(-1) {
}

HTTPSink::~HTTPSink() {
  ::closeSocket(fSocket);
}

 Boolean HTTPSink::isUseableFrame(unsigned char* /*framePtr*/,
				  unsigned /*frameSize*/) {
  // default implementation
  return True;
}

Boolean HTTPSink::continuePlaying() {
  if (fSource == NULL) return False;

  if (fClientSocket < 0) {
    // We're still waiting for a connection from a client.
    // Try making one now.  (Recall that we're non-blocking)
    struct sockaddr_in clientAddr;
    SOCKLEN_T clientAddrLen = sizeof clientAddr;
    fClientSocket = accept(fSocket, (struct sockaddr*)&clientAddr,
			   &clientAddrLen);
    if (fClientSocket < 0) {
      int err = envir().getErrno();
      if (err != EWOULDBLOCK) {
	envir().setResultErrMsg("accept() failed: ");
	return False;
      }
    } else {
      // We made a connection; so send back a HTTP "OK", followed by other
      // information (in particular, "Content-Type:") that will make
      // client player tools happy:
      char okResponse[400];
      const char* responseFmt = "HTTP/1.1 200 OK\r\nCache-Control: no-cache\r\nPragma: no-cache\r\nContent-Length: 2147483647\r\nContent-Type: %s\r\n\r\n";
#if defined(IRIX) || defined(ALPHA) || defined(_QNX4) || defined(IMN_PIM) || defined(CRIS) || defined (VXWORKS)
      /* snprintf() isn't defined, so just use sprintf() - ugh! */
      sprintf(okResponse, responseFmt, fSource->MIMEtype());
#else
      snprintf(okResponse, sizeof okResponse, responseFmt, fSource->MIMEtype());
#endif
      send(fClientSocket, okResponse, strlen(okResponse), 0);
    }
  }

  fSource->getNextFrame(fBuffer, sizeof fBuffer,
			afterGettingFrame, this,
			ourOnSourceClosure, this);

  return True;
}

void HTTPSink::ourOnSourceClosure(void* clientData) {
  // No more input frames - we're done:
  HTTPSink* sink = (HTTPSink*) clientData;
  ::closeSocket(sink->fClientSocket);
  sink->fClientSocket = -1;
  onSourceClosure(sink);
}

void HTTPSink::afterGettingFrame(void* clientData, unsigned frameSize,
				 unsigned /*numTruncatedBytes*/,
				 struct timeval presentationTime,
				 unsigned /*durationInMicroseconds*/) {
  HTTPSink* sink = (HTTPSink*)clientData;
  sink->afterGettingFrame1(frameSize, presentationTime);
}

void HTTPSink::afterGettingFrame1(unsigned frameSize,
				 struct timeval /*presentationTime*/) {
  // Write the data back to our client socket (if we have one):
  if (fClientSocket >= 0 && isUseableFrame(fBuffer, frameSize)) {
    int sendResult
      = send(fClientSocket, (char*)(&fBuffer[0]), frameSize, 0);
    if (sendResult < 0) {
      int err = envir().getErrno();
      if (err != EWOULDBLOCK) {
	// The client appears to have gone; close him down,
	// and consider ourselves done:
	ourOnSourceClosure(this);
	return;
      }
    }
  }

  // Then try getting the next frame:
  continuePlaying();
}
