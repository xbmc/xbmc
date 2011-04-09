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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a DV video file.
// Implementation

#include "DVVideoFileServerMediaSubsession.hh"
#include "DVVideoRTPSink.hh"
#include "ByteStreamFileSource.hh"
#include "DVVideoStreamFramer.hh"

DVVideoFileServerMediaSubsession*
DVVideoFileServerMediaSubsession::createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource) {
  return new DVVideoFileServerMediaSubsession(env, fileName, reuseFirstSource);
}

DVVideoFileServerMediaSubsession
::DVVideoFileServerMediaSubsession(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource),
    fFileDuration(0.0) {
}

DVVideoFileServerMediaSubsession::~DVVideoFileServerMediaSubsession() {
}

FramedSource* DVVideoFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  // Create the video source:
  ByteStreamFileSource* fileSource = ByteStreamFileSource::createNew(envir(), fFileName);
  if (fileSource == NULL) return NULL;
  fFileSize = fileSource->fileSize();

  // Create a framer for the Video Elementary Stream:
  DVVideoStreamFramer* framer = DVVideoStreamFramer::createNew(envir(), fileSource, True/*the file source is seekable*/);
  
  // Use the framer to figure out the file's duration:
  unsigned frameSize;
  double frameDuration;
  if (framer->getFrameParameters(frameSize, frameDuration)) {
    fFileDuration = (float)(((int64_t)fFileSize*frameDuration)/(frameSize*1000000.0));
    estBitrate = (unsigned)((8000.0*frameSize)/frameDuration); // in kbps
  } else {
    estBitrate = 50000; // kbps, estimate
  }

  return framer;
}

RTPSink* DVVideoFileServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
							    unsigned char rtpPayloadTypeIfDynamic,
							    FramedSource* /*inputSource*/) {
  return DVVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

char const* DVVideoFileServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  return ((DVVideoRTPSink*)rtpSink)->auxSDPLineFromFramer((DVVideoStreamFramer*)inputSource);
}

float DVVideoFileServerMediaSubsession::duration() const {
  return fFileDuration;
}

void DVVideoFileServerMediaSubsession::seekStreamSource(FramedSource* inputSource, double seekNPT) {
  // First, get the file source from "inputSource" (a framer):
  DVVideoStreamFramer* framer = (DVVideoStreamFramer*)inputSource;
  ByteStreamFileSource* fileSource = (ByteStreamFileSource*)(framer->inputSource());

  // Then figure out where to seek to within the file:
  if (fFileDuration > 0.0) {
    u_int64_t seekByteNumber = (u_int64_t)(((int64_t)fFileSize*seekNPT)/fFileDuration);
    fileSource->seekToByteAbsolute(seekByteNumber);
  }
}
