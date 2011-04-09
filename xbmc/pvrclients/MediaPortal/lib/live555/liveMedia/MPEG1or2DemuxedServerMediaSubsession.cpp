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
// on demand, from a MPEG-1 or 2 demuxer.
// Implementation

#include "MPEG1or2DemuxedServerMediaSubsession.hh"
#include "MPEG1or2AudioStreamFramer.hh"
#include "MPEG1or2AudioRTPSink.hh"
#include "MPEG1or2VideoStreamFramer.hh"
#include "MPEG1or2VideoRTPSink.hh"
#include "AC3AudioStreamFramer.hh"
#include "AC3AudioRTPSink.hh"
#include "ByteStreamFileSource.hh"

MPEG1or2DemuxedServerMediaSubsession* MPEG1or2DemuxedServerMediaSubsession
::createNew(MPEG1or2FileServerDemux& demux, u_int8_t streamIdTag,
	    Boolean reuseFirstSource, Boolean iFramesOnly, double vshPeriod) {
  return new MPEG1or2DemuxedServerMediaSubsession(demux, streamIdTag,
						  reuseFirstSource,
						  iFramesOnly, vshPeriod);
}

MPEG1or2DemuxedServerMediaSubsession
::MPEG1or2DemuxedServerMediaSubsession(MPEG1or2FileServerDemux& demux,
				       u_int8_t streamIdTag, Boolean reuseFirstSource,
				       Boolean iFramesOnly, double vshPeriod)
  : OnDemandServerMediaSubsession(demux.envir(), reuseFirstSource),
    fOurDemux(demux), fStreamIdTag(streamIdTag),
    fIFramesOnly(iFramesOnly), fVSHPeriod(vshPeriod) {
}

MPEG1or2DemuxedServerMediaSubsession::~MPEG1or2DemuxedServerMediaSubsession() {
}

FramedSource* MPEG1or2DemuxedServerMediaSubsession
::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate) {
  FramedSource* es = NULL;
  do {
    es = fOurDemux.newElementaryStream(clientSessionId, fStreamIdTag);
    if (es == NULL) break;

    if ((fStreamIdTag&0xF0) == 0xC0 /*MPEG audio*/) {
      estBitrate = 128; // kbps, estimate
      return MPEG1or2AudioStreamFramer::createNew(envir(), es);
    } else if ((fStreamIdTag&0xF0) == 0xE0 /*video*/) {
      estBitrate = 500; // kbps, estimate
      return MPEG1or2VideoStreamFramer::createNew(envir(), es,
						  fIFramesOnly, fVSHPeriod);
    } else if (fStreamIdTag == 0xBD /*AC-3 audio*/) {
      estBitrate = 192; // kbps, estimate
      return AC3AudioStreamFramer::createNew(envir(), es);
    } else { // unknown stream type
      break;
    }
  } while (0);

  // An error occurred:
  Medium::close(es);
  return NULL;
}

RTPSink* MPEG1or2DemuxedServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* inputSource) {
  if ((fStreamIdTag&0xF0) == 0xC0 /*MPEG audio*/) {
    return MPEG1or2AudioRTPSink::createNew(envir(), rtpGroupsock);
  } else if ((fStreamIdTag&0xF0) == 0xE0 /*video*/) {
    return MPEG1or2VideoRTPSink::createNew(envir(), rtpGroupsock);
  } else if (fStreamIdTag == 0xBD /*AC-3 audio*/) {
    // Get the sampling frequency from the audio source; use it for the RTP frequency:
    AC3AudioStreamFramer* audioSource
      = (AC3AudioStreamFramer*)inputSource;
    return AC3AudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
				      audioSource->samplingRate());
  } else {
    return NULL;
  }
}

void MPEG1or2DemuxedServerMediaSubsession
::seekStreamSource(FramedSource* inputSource, double seekNPT) {
  float const dur = duration();
  unsigned const size = fOurDemux.fileSize();
  unsigned absBytePosition = dur == 0.0 ? 0 : (unsigned)((seekNPT/dur)*size);

  // "inputSource" is a 'framer'
  // Flush its data, to account for the seek that we're about to do:
  if ((fStreamIdTag&0xF0) == 0xC0 /*MPEG audio*/) {
    MPEG1or2AudioStreamFramer* framer = (MPEG1or2AudioStreamFramer*)inputSource;
    framer->flushInput();
  } else if ((fStreamIdTag&0xF0) == 0xE0 /*video*/) {
    MPEG1or2VideoStreamFramer* framer = (MPEG1or2VideoStreamFramer*)inputSource;
    framer->flushInput();
  }

  // "inputSource" is a filter; its input source is the original elem stream source:
  MPEG1or2DemuxedElementaryStream* elemStreamSource
    = (MPEG1or2DemuxedElementaryStream*)(((FramedFilter*)inputSource)->inputSource());

  // Next, get the original source demux:
  MPEG1or2Demux& sourceDemux = elemStreamSource->sourceDemux();

  // and flush its input buffers:
  sourceDemux.flushInput();

  // Then, get the original input file stream from the source demux:
  ByteStreamFileSource* inputFileSource
    = (ByteStreamFileSource*)(sourceDemux.inputSource());
  // Note: We can make that cast, because we know that the demux was originally
  // created from a "ByteStreamFileSource".

  // Do the appropriate seek within the input file stream:
  inputFileSource->seekToByteAbsolute(absBytePosition);
}

float MPEG1or2DemuxedServerMediaSubsession::duration() const {
  return fOurDemux.fileDuration();
}
