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
// Transcoder for ADUized MP3 frames
// Implementation

#include "MP3ADUTranscoder.hh"
#include "MP3Internals.hh"
#include <string.h>

MP3ADUTranscoder::MP3ADUTranscoder(UsageEnvironment& env,
				   unsigned outBitrate /* in kbps */,
				   FramedSource* inputSource)
  : FramedFilter(env, inputSource),
    fOutBitrate(outBitrate),
    fAvailableBytesForBackpointer(0),
    fOrigADU(new unsigned char[MAX_MP3_FRAME_SIZE]) {
}

MP3ADUTranscoder::~MP3ADUTranscoder() {
  delete[] fOrigADU;
}

MP3ADUTranscoder* MP3ADUTranscoder::createNew(UsageEnvironment& env,
					      unsigned outBitrate /* in kbps */,
					      FramedSource* inputSource) {
  // The source must be an MP3 ADU source:
  if (strcmp(inputSource->MIMEtype(), "audio/MPA-ROBUST") != 0) {
    env.setResultMsg(inputSource->name(), " is not an MP3 ADU source");
    return NULL;
  }

  return new MP3ADUTranscoder(env, outBitrate, inputSource);
}

void MP3ADUTranscoder::getAttributes() const {
  // Begin by getting the attributes from our input source:
  fInputSource->getAttributes();

  // Then modify them by appending the corrected bandwidth
  char buffer[30];
  sprintf(buffer, " bandwidth %d", outBitrate());
  envir().appendToResultMsg(buffer);
}

void MP3ADUTranscoder::doGetNextFrame() {
  fInputSource->getNextFrame(fOrigADU, MAX_MP3_FRAME_SIZE,
			     afterGettingFrame, this, handleClosure, this);
}

void MP3ADUTranscoder::afterGettingFrame(void* clientData,
					 unsigned numBytesRead,
					 unsigned numTruncatedBytes,
					 struct timeval presentationTime,
					 unsigned durationInMicroseconds) {
  MP3ADUTranscoder* transcoder = (MP3ADUTranscoder*)clientData;
  transcoder->afterGettingFrame1(numBytesRead, numTruncatedBytes,
				 presentationTime, durationInMicroseconds);
}

void MP3ADUTranscoder::afterGettingFrame1(unsigned numBytesRead,
					  unsigned numTruncatedBytes,
					  struct timeval presentationTime,
					  unsigned durationInMicroseconds) {
  fNumTruncatedBytes = numTruncatedBytes; // but can we handle this being >0? #####
  fPresentationTime = presentationTime;
  fDurationInMicroseconds = durationInMicroseconds;
  fFrameSize = TranscodeMP3ADU(fOrigADU, numBytesRead, fOutBitrate,
			    fTo, fMaxSize, fAvailableBytesForBackpointer);
  if (fFrameSize == 0) { // internal error - bad ADU data?
    handleClosure(this);
    return;
  }

  // Call our own 'after getting' function.  Because we're not a 'leaf'
  // source, we can call this directly, without risking infinite recursion.
  afterGetting(this);
}
