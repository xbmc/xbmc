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
// A filter for converting a stream of MPEG PES packets to a MPEG-2 Transport Stream
// Implementation

#include "MPEG2TransportStreamFromPESSource.hh"

#define MAX_PES_PACKET_SIZE (6+65535)

MPEG2TransportStreamFromPESSource* MPEG2TransportStreamFromPESSource
::createNew(UsageEnvironment& env, MPEG1or2DemuxedElementaryStream* inputSource) {
  return new MPEG2TransportStreamFromPESSource(env, inputSource);
}

MPEG2TransportStreamFromPESSource
::MPEG2TransportStreamFromPESSource(UsageEnvironment& env,
				    MPEG1or2DemuxedElementaryStream* inputSource)
  : MPEG2TransportStreamMultiplexor(env),
    fInputSource(inputSource) {
  fInputBuffer = new unsigned char[MAX_PES_PACKET_SIZE];
}

MPEG2TransportStreamFromPESSource::~MPEG2TransportStreamFromPESSource() {
  Medium::close(fInputSource);
  delete[] fInputBuffer;
}

void MPEG2TransportStreamFromPESSource::doStopGettingFrames() {
  fInputSource->stopGettingFrames();
}

void MPEG2TransportStreamFromPESSource
::awaitNewBuffer(unsigned char* /*oldBuffer*/) {
  fInputSource->getNextFrame(fInputBuffer, MAX_PES_PACKET_SIZE,
			     afterGettingFrame, this,
			     FramedSource::handleClosure, this);
}

void MPEG2TransportStreamFromPESSource
::afterGettingFrame(void* clientData, unsigned frameSize,
		    unsigned numTruncatedBytes,
		    struct timeval presentationTime,
		    unsigned durationInMicroseconds) {
  MPEG2TransportStreamFromPESSource* source
    = (MPEG2TransportStreamFromPESSource*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes,
			    presentationTime, durationInMicroseconds);
}

void MPEG2TransportStreamFromPESSource
::afterGettingFrame1(unsigned frameSize,
		     unsigned /*numTruncatedBytes*/,
		     struct timeval /*presentationTime*/,
		     unsigned /*durationInMicroseconds*/) {
  if (frameSize < 4) return;

  handleNewBuffer(fInputBuffer, frameSize,
		  fInputSource->mpegVersion(), fInputSource->lastSeenSCR());
}
