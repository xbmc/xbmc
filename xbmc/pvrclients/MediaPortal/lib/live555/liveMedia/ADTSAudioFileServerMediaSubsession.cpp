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
// on demand, from an AAC audio file in ADTS format
// Implementation

#include "ADTSAudioFileServerMediaSubsession.hh"
#include "ADTSAudioFileSource.hh"
#include "MPEG4GenericRTPSink.hh"

ADTSAudioFileServerMediaSubsession*
ADTSAudioFileServerMediaSubsession::createNew(UsageEnvironment& env,
					     char const* fileName,
					     Boolean reuseFirstSource) {
  return new ADTSAudioFileServerMediaSubsession(env, fileName, reuseFirstSource);
}

ADTSAudioFileServerMediaSubsession
::ADTSAudioFileServerMediaSubsession(UsageEnvironment& env,
				    char const* fileName, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, fileName, reuseFirstSource) {
}

ADTSAudioFileServerMediaSubsession
::~ADTSAudioFileServerMediaSubsession() {
}

FramedSource* ADTSAudioFileServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 96; // kbps, estimate

  return ADTSAudioFileSource::createNew(envir(), fFileName);
}

RTPSink* ADTSAudioFileServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
  ADTSAudioFileSource* adtsSource = (ADTSAudioFileSource*)inputSource;
  return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					rtpPayloadTypeIfDynamic,
					adtsSource->samplingFrequency(),
					"audio", "AAC-hbr", adtsSource->configStr(),
					adtsSource->numChannels());
}
