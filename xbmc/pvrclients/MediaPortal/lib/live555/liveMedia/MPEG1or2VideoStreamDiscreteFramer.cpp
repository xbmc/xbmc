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
// A simplified version of "MPEG1or2VideoStreamFramer" that takes only
// complete, discrete frames (rather than an arbitrary byte stream) as input.
// This avoids the parsing and data copying overhead of the full
// "MPEG1or2VideoStreamFramer".
// Implementation

#include "MPEG1or2VideoStreamDiscreteFramer.hh"

MPEG1or2VideoStreamDiscreteFramer*
MPEG1or2VideoStreamDiscreteFramer::createNew(UsageEnvironment& env,
                                             FramedSource* inputSource,
                                             Boolean iFramesOnly,
                                             double vshPeriod) {
  // Need to add source type checking here???  #####
  return new MPEG1or2VideoStreamDiscreteFramer(env, inputSource,
                                               iFramesOnly, vshPeriod);
}

MPEG1or2VideoStreamDiscreteFramer
::MPEG1or2VideoStreamDiscreteFramer(UsageEnvironment& env,
                                    FramedSource* inputSource,
                                    Boolean iFramesOnly, double vshPeriod)
  : MPEG1or2VideoStreamFramer(env, inputSource, iFramesOnly, vshPeriod,
                              False/*don't create a parser*/),
    fLastNonBFrameTemporal_reference(0),
    fSavedVSHSize(0), fSavedVSHTimestamp(0.0),
    fIFramesOnly(iFramesOnly), fVSHPeriod(vshPeriod) {
  fLastNonBFramePresentationTime.tv_sec = 0;
  fLastNonBFramePresentationTime.tv_usec = 0;
}

MPEG1or2VideoStreamDiscreteFramer::~MPEG1or2VideoStreamDiscreteFramer() {
}

void MPEG1or2VideoStreamDiscreteFramer::doGetNextFrame() {
  // Arrange to read data (which should be a complete MPEG-1 or 2 video frame)
  // from our data source, directly into the client's input buffer.
  // After reading this, we'll do some parsing on the frame.
  fInputSource->getNextFrame(fTo, fMaxSize,
                             afterGettingFrame, this,
                             FramedSource::handleClosure, this);
}

void MPEG1or2VideoStreamDiscreteFramer
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  MPEG1or2VideoStreamDiscreteFramer* source
    = (MPEG1or2VideoStreamDiscreteFramer*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes,
                             presentationTime, durationInMicroseconds);
}

static double const frameRateFromCode[] = {
  0.0,          // forbidden
  24000/1001.0, // approx 23.976
  24.0,
  25.0,
  30000/1001.0, // approx 29.97
  30.0,
  50.0,
  60000/1001.0, // approx 59.94
  60.0,
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0,          // reserved
  0.0           // reserved
};

#define MILLION 1000000

void MPEG1or2VideoStreamDiscreteFramer
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) {
  // Check that the first 4 bytes are a system code:
  if (frameSize >= 4 && fTo[0] == 0 && fTo[1] == 0 && fTo[2] == 1) {
    fPictureEndMarker = True; // Assume that we have a complete 'picture' here

    u_int8_t nextCode = fTo[3];
    if (nextCode == 0xB3) { // VIDEO_SEQUENCE_HEADER_START_CODE
      // Note the following 'frame rate' code:
      if (frameSize >= 8) {
	u_int8_t frame_rate_code = fTo[7]&0x0F;
	fFrameRate = frameRateFromCode[frame_rate_code];
      }

      // Also, save away this Video Sequence Header, in case we need it later:
      // First, figure out how big it is:
      unsigned vshSize;
      for (vshSize = 4; vshSize < frameSize-3; ++vshSize) {
	if (fTo[vshSize] == 0 && fTo[vshSize+1] == 0 && fTo[vshSize+2] == 1 &&
	    (fTo[vshSize+3] == 0xB8 || fTo[vshSize+3] == 0x00)) break;
      }
      if (vshSize == frameSize-3) vshSize = frameSize; // There was nothing else following it
      if (vshSize <= sizeof fSavedVSHBuffer) {
	memmove(fSavedVSHBuffer, fTo, vshSize);
	fSavedVSHSize = vshSize;
	fSavedVSHTimestamp
	  = presentationTime.tv_sec + presentationTime.tv_usec/(double)MILLION;
      }
    } else if (nextCode == 0xB8) { // GROUP_START_CODE
      // If necessary, insert a saved Video Sequence Header in front of this:
      double pts = presentationTime.tv_sec + presentationTime.tv_usec/(double)MILLION;
      if (pts > fSavedVSHTimestamp + fVSHPeriod &&
	  fSavedVSHSize + frameSize <= fMaxSize) {
	memmove(&fTo[fSavedVSHSize], &fTo[0], frameSize); // make room for the header
	memmove(&fTo[0], fSavedVSHBuffer, fSavedVSHSize); // insert it
	frameSize += fSavedVSHSize;
	fSavedVSHTimestamp = pts;
      }
    }

    unsigned i = 3;
    if (nextCode == 0xB3 /*VIDEO_SEQUENCE_HEADER_START_CODE*/ ||
	nextCode == 0xB8 /*GROUP_START_CODE*/) {
      // Skip to the following PICTURE_START_CODE (if any):
      for (i += 4; i < frameSize; ++i) {
	if (fTo[i] == 0x00 /*PICTURE_START_CODE*/
	    && fTo[i-1] == 1 && fTo[i-2] == 0 && fTo[i-3] == 0) {
	  nextCode = fTo[i];
	  break;
	}
      }
    }

    if (nextCode == 0x00 /*PICTURE_START_CODE*/ && i+2 < frameSize) {
      // Get the 'temporal_reference' and 'picture_coding_type' from the
      // following 2 bytes:
      ++i;
      unsigned short temporal_reference = (fTo[i]<<2)|(fTo[i+1]>>6);
      unsigned char picture_coding_type = (fTo[i+1]&0x38)>>3;

      // If this is not an "I" frame, but we were asked for "I" frames only, then try again:
      if (fIFramesOnly && picture_coding_type != 1) {
	doGetNextFrame();
	return;
      }

      // If this is a "B" frame, then we have to tweak "presentationTime":
      if (picture_coding_type == 3/*B*/
	  && (fLastNonBFramePresentationTime.tv_usec > 0 ||
	      fLastNonBFramePresentationTime.tv_sec > 0)) {
	int trIncrement
            = fLastNonBFrameTemporal_reference - temporal_reference;
	if (trIncrement < 0) trIncrement += 1024; // field is 10 bits in size

	unsigned usIncrement = fFrameRate == 0.0 ? 0
	  : (unsigned)((trIncrement*MILLION)/fFrameRate);
	unsigned secondsToSubtract = usIncrement/MILLION;
	unsigned uSecondsToSubtract = usIncrement%MILLION;

	presentationTime = fLastNonBFramePresentationTime;
	if ((unsigned)presentationTime.tv_usec < uSecondsToSubtract) {
	  presentationTime.tv_usec += MILLION;
	  if (presentationTime.tv_sec > 0) --presentationTime.tv_sec;
	}
	presentationTime.tv_usec -= uSecondsToSubtract;
	if ((unsigned)presentationTime.tv_sec > secondsToSubtract) {
	  presentationTime.tv_sec -= secondsToSubtract;
	} else {
	  presentationTime.tv_sec = presentationTime.tv_usec = 0;
	}
      } else {
	fLastNonBFramePresentationTime = presentationTime;
	fLastNonBFrameTemporal_reference = temporal_reference;
      }
    }
  }

  // ##### Later:
  // - do "iFramesOnly" if requested

  // Complete delivery to the client:
  fFrameSize = frameSize;
  fNumTruncatedBytes = numTruncatedBytes;
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;
  afterGetting(this);
}
