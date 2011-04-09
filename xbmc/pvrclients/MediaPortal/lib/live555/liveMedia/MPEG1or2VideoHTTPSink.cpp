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
// Implementation

#include "MPEG1or2VideoHTTPSink.hh"

////////// MPEG1or2VideoHTTPSink //////////

MPEG1or2VideoHTTPSink* MPEG1or2VideoHTTPSink::createNew(UsageEnvironment& env, Port ourPort) {
  int ourSocket = -1;

  do {
    int ourSocket = setUpOurSocket(env, ourPort);
    if (ourSocket == -1) break;

    MPEG1or2VideoHTTPSink* newSink = new MPEG1or2VideoHTTPSink(env, ourSocket);
    if (newSink == NULL) break;

    appendPortNum(env, ourPort);

    return newSink;
  } while (0);

  if (ourSocket != -1) ::closeSocket(ourSocket);
  return NULL;
}

MPEG1or2VideoHTTPSink::MPEG1or2VideoHTTPSink(UsageEnvironment& env, int ourSocket)
  : HTTPSink(env, ourSocket), fHaveSeenFirstVSH(False) {
}

MPEG1or2VideoHTTPSink::~MPEG1or2VideoHTTPSink() {
}

#define VIDEO_SEQUENCE_HEADER_START_CODE 0x000001B3

Boolean MPEG1or2VideoHTTPSink::isUseableFrame(unsigned char* framePtr,
					  unsigned frameSize) {
  // Some clients get confused if the data we give them does not start
  // with a 'video_sequence_header', so we ignore any frames that precede
  // the first 'video_sequence_header':

  // Sanity check: a frame with < 4 bytes is never valid:
  if (frameSize < 4) return False;

  if (fHaveSeenFirstVSH) return True;

  unsigned first4Bytes
    = (framePtr[0]<<24)|(framePtr[1]<<16)|(framePtr[2]<<8)|framePtr[3];

  if (first4Bytes == VIDEO_SEQUENCE_HEADER_START_CODE) {
    fHaveSeenFirstVSH = True;
    return True;
  } else {
    return False;
  }
}
