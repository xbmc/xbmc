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
// on demand, from a H263 video file.
// Implementation

// Author: Bernhard Feiten. // Based on MPEG4VideoFileServerMediaSubsession
// Updated by Ross FInlayson (December 2007)

#include "H263plusVideoFileServerMediaSubsession.hh"
#include "H263plusVideoRTPSink.hh"
#include "ByteStreamFileSource.hh"
#include "H263plusVideoStreamFramer.hh"

H263plusVideoFileServerMediaSubsession*
H263plusVideoFileServerMediaSubsession::createNew(UsageEnvironment& env,
						  char const* fileName,
						  Boolean reuseFirstSource) {
  return new H263plusVideoFileServerMediaSubsession(env, fileName, reuseFirstSource);
}

H263plusVideoFileServerMediaSubsession
::H263plusVideoFileServerMediaSubsession(UsageEnvironment& env,
					 char const* fileName,
					 Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource) {
}

H263plusVideoFileServerMediaSubsession::~H263plusVideoFileServerMediaSubsession() {
}

FramedSource* H263plusVideoFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 500; // kbps, estimate ??

  // Create the video source:
  ByteStreamFileSource* fileSource = ByteStreamFileSource::createNew(envir(), fFileName);
  if (fileSource == NULL) return NULL;
  fFileSize = fileSource->fileSize();

  // Create a framer for the Video Elementary Stream:
  return H263plusVideoStreamFramer::createNew(envir(), fileSource);
}

RTPSink* H263plusVideoFileServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
								  unsigned char rtpPayloadTypeIfDynamic,
								  FramedSource* /*inputSource*/) {
  return H263plusVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
