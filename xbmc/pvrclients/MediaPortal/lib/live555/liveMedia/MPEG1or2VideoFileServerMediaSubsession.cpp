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
// on demand, from a MPEG-1 or 2 Elementary Stream video file.
// Implementation

#include "MPEG1or2VideoFileServerMediaSubsession.hh"
#include "MPEG1or2VideoRTPSink.hh"
#include "ByteStreamFileSource.hh"
#include "MPEG1or2VideoStreamFramer.hh"

MPEG1or2VideoFileServerMediaSubsession*
MPEG1or2VideoFileServerMediaSubsession::createNew(UsageEnvironment& env,
						  char const* fileName,
						  Boolean reuseFirstSource,
						  Boolean iFramesOnly,
						  double vshPeriod) {
  return new MPEG1or2VideoFileServerMediaSubsession(env, fileName, reuseFirstSource,
						    iFramesOnly, vshPeriod);
}

MPEG1or2VideoFileServerMediaSubsession
::MPEG1or2VideoFileServerMediaSubsession(UsageEnvironment& env,
					 char const* fileName,
					 Boolean reuseFirstSource,
					 Boolean iFramesOnly,
					 double vshPeriod)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource),
    fIFramesOnly(iFramesOnly), fVSHPeriod(vshPeriod) {
}

MPEG1or2VideoFileServerMediaSubsession
::~MPEG1or2VideoFileServerMediaSubsession() {
}

FramedSource* MPEG1or2VideoFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 500; // kbps, estimate

  ByteStreamFileSource* fileSource
    = ByteStreamFileSource::createNew(envir(), fFileName);
  if (fileSource == NULL) return NULL;
  fFileSize = fileSource->fileSize();

  return MPEG1or2VideoStreamFramer
    ::createNew(envir(), fileSource, fIFramesOnly, fVSHPeriod);
}

RTPSink* MPEG1or2VideoFileServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char /*rtpPayloadTypeIfDynamic*/,
		   FramedSource* /*inputSource*/) {
  return MPEG1or2VideoRTPSink::createNew(envir(), rtpGroupsock);
}
