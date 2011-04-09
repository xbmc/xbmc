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
// A MPEG 1 or 2 Elementary Stream, demultiplexed from a Program Stream
// Implementation

#include "MPEG1or2DemuxedElementaryStream.hh"

////////// MPEG1or2DemuxedElementaryStream //////////

MPEG1or2DemuxedElementaryStream::
MPEG1or2DemuxedElementaryStream(UsageEnvironment& env, u_int8_t streamIdTag,
			    MPEG1or2Demux& sourceDemux)
  : FramedSource(env),
    fOurStreamIdTag(streamIdTag), fOurSourceDemux(sourceDemux), fMPEGversion(0) {
  // Set our MIME type string for known media types:
  if ((streamIdTag&0xE0) == 0xC0) {
    fMIMEtype = "audio/MPEG";
  } else if ((streamIdTag&0xF0) == 0xE0) {
    fMIMEtype = "video/MPEG";
  } else {
    fMIMEtype = MediaSource::MIMEtype();
  }
}

MPEG1or2DemuxedElementaryStream::~MPEG1or2DemuxedElementaryStream() {
  fOurSourceDemux.noteElementaryStreamDeletion(this);
}

void MPEG1or2DemuxedElementaryStream::doGetNextFrame() {
  fOurSourceDemux.getNextFrame(fOurStreamIdTag, fTo, fMaxSize,
			       afterGettingFrame, this,
			       handleClosure, this);
}

void MPEG1or2DemuxedElementaryStream::doStopGettingFrames() {
  fOurSourceDemux.stopGettingFrames(fOurStreamIdTag);
}

char const* MPEG1or2DemuxedElementaryStream::MIMEtype() const {
  return fMIMEtype;
}

unsigned MPEG1or2DemuxedElementaryStream::maxFrameSize() const {
  return 6+65535;
      // because the MPEG spec allows for PES packets as large as
      // (6 + 65535) bytes (header + data)
}

void MPEG1or2DemuxedElementaryStream
::afterGettingFrame(void* clientData,
		    unsigned frameSize, unsigned numTruncatedBytes,
		    struct timeval presentationTime,
		    unsigned durationInMicroseconds) {
  MPEG1or2DemuxedElementaryStream* stream
    = (MPEG1or2DemuxedElementaryStream*)clientData;
  stream->afterGettingFrame1(frameSize, numTruncatedBytes,
			     presentationTime, durationInMicroseconds);
}

void MPEG1or2DemuxedElementaryStream
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
		     struct timeval presentationTime,
		     unsigned durationInMicroseconds) {
  fFrameSize = frameSize;
  fNumTruncatedBytes = numTruncatedBytes;
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;

  fLastSeenSCR = fOurSourceDemux.lastSeenSCR();
  fMPEGversion = fOurSourceDemux.mpegVersion();

  FramedSource::afterGetting(this);
}
