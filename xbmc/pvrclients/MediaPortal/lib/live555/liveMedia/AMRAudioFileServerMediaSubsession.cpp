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
// on demand, from an AMR audio file.
// Implementation

#include "AMRAudioFileServerMediaSubsession.hh"
#include "AMRAudioRTPSink.hh"
#include "AMRAudioFileSource.hh"

AMRAudioFileServerMediaSubsession*
AMRAudioFileServerMediaSubsession::createNew(UsageEnvironment& env,
					     char const* fileName,
					     Boolean reuseFirstSource) {
  return new AMRAudioFileServerMediaSubsession(env, fileName, reuseFirstSource);
}

AMRAudioFileServerMediaSubsession
::AMRAudioFileServerMediaSubsession(UsageEnvironment& env,
				    char const* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource) {
}

AMRAudioFileServerMediaSubsession
::~AMRAudioFileServerMediaSubsession() {
}

FramedSource* AMRAudioFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 10; // kbps, estimate

  return AMRAudioFileSource::createNew(envir(), fFileName);
}

RTPSink* AMRAudioFileServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
  AMRAudioFileSource* amrSource = (AMRAudioFileSource*)inputSource;
  return AMRAudioRTPSink::createNew(envir(), rtpGroupsock,
				    rtpPayloadTypeIfDynamic,
				    amrSource->isWideband(),
				    amrSource->numChannels());
}
