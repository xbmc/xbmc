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
// A simple UDP sink (i.e., without RTP or other headers added); one frame per packet
// Implementation

#include "BasicUDPSink.hh"
#include <GroupsockHelper.hh>

BasicUDPSink* BasicUDPSink::createNew(UsageEnvironment& env, Groupsock* gs,
				      unsigned maxPayloadSize) {
  return new BasicUDPSink(env, gs, maxPayloadSize);
}

BasicUDPSink::BasicUDPSink(UsageEnvironment& env, Groupsock* gs,
			   unsigned maxPayloadSize)
  : MediaSink(env),
    fGS(gs), fMaxPayloadSize(maxPayloadSize) {
  fOutputBuffer = new unsigned char[fMaxPayloadSize];
}

BasicUDPSink::~BasicUDPSink() {
  delete[] fOutputBuffer;
}

Boolean BasicUDPSink::continuePlaying() {
  // Record the fact that we're starting to play now:
  gettimeofday(&fNextSendTime, NULL);

  // Arrange to get and send the first payload.
  // (This will also schedule any future sends.)
  continuePlaying1();
  return True;
}

void BasicUDPSink::continuePlaying1() {
  if (fSource != NULL) {
    fSource->getNextFrame(fOutputBuffer, fMaxPayloadSize,
			  afterGettingFrame, this,
			  onSourceClosure, this);
  }
}

void BasicUDPSink::afterGettingFrame(void* clientData, unsigned frameSize,
				     unsigned numTruncatedBytes,
				     struct timeval /*presentationTime*/,
				     unsigned durationInMicroseconds) {
  BasicUDPSink* sink = (BasicUDPSink*)clientData;
  sink->afterGettingFrame1(frameSize, numTruncatedBytes, durationInMicroseconds);
}

void BasicUDPSink::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
				      unsigned durationInMicroseconds) {
  if (numTruncatedBytes > 0) {
    envir() << "BasicUDPSink::afterGettingFrame1(): The input frame data was too large for our spcified maximum payload size ("
	    << fMaxPayloadSize << ").  "
	    << numTruncatedBytes << " bytes of trailing data was dropped!\n";
  }

  // Send the packet:
  fGS->output(envir(), fGS->ttl(), fOutputBuffer, frameSize);

  // Figure out the time at which the next packet should be sent, based
  // on the duration of the payload that we just read:
  fNextSendTime.tv_usec += durationInMicroseconds;
  fNextSendTime.tv_sec += fNextSendTime.tv_usec/1000000;
  fNextSendTime.tv_usec %= 1000000;

  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);
  int uSecondsToGo;
  if (fNextSendTime.tv_sec < timeNow.tv_sec) {
    uSecondsToGo = 0; // prevents integer underflow if too far behind
  } else {
    uSecondsToGo = (fNextSendTime.tv_sec - timeNow.tv_sec)*1000000
      + (fNextSendTime.tv_usec - timeNow.tv_usec);
  }

  // Delay this amount of time:
  nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecondsToGo,
							   (TaskFunc*)sendNext, this);
}

// The following is called after each delay between packet sends:
void BasicUDPSink::sendNext(void* firstArg) {
  BasicUDPSink* sink = (BasicUDPSink*)firstArg;
  sink->continuePlaying1();
}
