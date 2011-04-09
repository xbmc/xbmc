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
// A filter that breaks up an MPEG (1,2) audio elementary stream into frames
// Implementation

#include "MPEG1or2AudioStreamFramer.hh"
#include "StreamParser.hh"
#include "MP3Internals.hh"
#include <GroupsockHelper.hh>

////////// MPEG1or2AudioStreamParser definition //////////

class MPEG1or2AudioStreamParser: public StreamParser {
public:
  MPEG1or2AudioStreamParser(MPEG1or2AudioStreamFramer* usingSource,
			FramedSource* inputSource);
  virtual ~MPEG1or2AudioStreamParser();

public:
  unsigned parse(unsigned& numTruncatedBytes);
      // returns the size of the frame that was acquired, or 0 if none was

  void registerReadInterest(unsigned char* to, unsigned maxSize);

  MP3FrameParams const& currentFrame() const { return fCurrentFrame; }

private:
  unsigned char* fTo;
  unsigned fMaxSize;

  // Parameters of the most recently read frame:
  MP3FrameParams fCurrentFrame; // also works for layer I or II
};


////////// MPEG1or2AudioStreamFramer implementation //////////

MPEG1or2AudioStreamFramer
::MPEG1or2AudioStreamFramer(UsageEnvironment& env, FramedSource* inputSource,
			    Boolean syncWithInputSource)
  : FramedFilter(env, inputSource),
    fSyncWithInputSource(syncWithInputSource) {
  reset();

  fParser = new MPEG1or2AudioStreamParser(this, inputSource);
}

MPEG1or2AudioStreamFramer::~MPEG1or2AudioStreamFramer() {
  delete fParser;
}

MPEG1or2AudioStreamFramer*
MPEG1or2AudioStreamFramer::createNew(UsageEnvironment& env,
				     FramedSource* inputSource,
				     Boolean syncWithInputSource) {
  // Need to add source type checking here???  #####
  return new MPEG1or2AudioStreamFramer(env, inputSource, syncWithInputSource);
}

void MPEG1or2AudioStreamFramer::flushInput() {
  reset();
  fParser->flushInput();
}

void MPEG1or2AudioStreamFramer::reset() {
  // Use the current wallclock time as the initial 'presentation time':
  struct timeval timeNow;
  gettimeofday(&timeNow, NULL);
  resetPresentationTime(timeNow);
}

void MPEG1or2AudioStreamFramer
::resetPresentationTime(struct timeval newPresentationTime) {
  fNextFramePresentationTime = newPresentationTime;
}

void MPEG1or2AudioStreamFramer::doGetNextFrame() {
  fParser->registerReadInterest(fTo, fMaxSize);
  continueReadProcessing();
}

#define MILLION 1000000

static unsigned const numSamplesByLayer[4] = {0, 384, 1152, 1152};

struct timeval MPEG1or2AudioStreamFramer::currentFramePlayTime() const {
  MP3FrameParams const& fr = fParser->currentFrame();
  unsigned const numSamples = numSamplesByLayer[fr.layer];

  struct timeval result;
  unsigned const freq = fr.samplingFreq*(1 + fr.isMPEG2);
  if (freq == 0) {
    result.tv_sec = 0;
    result.tv_usec = 0;
    return result;
  }

  // result is numSamples/freq
  unsigned const uSeconds
    = ((numSamples*2*MILLION)/freq + 1)/2; // rounds to nearest integer

  result.tv_sec = uSeconds/MILLION;
  result.tv_usec = uSeconds%MILLION;
  return result;
}

void MPEG1or2AudioStreamFramer
::continueReadProcessing(void* clientData,
			 unsigned char* /*ptr*/, unsigned /*size*/,
			 struct timeval presentationTime) {
  MPEG1or2AudioStreamFramer* framer = (MPEG1or2AudioStreamFramer*)clientData;
  if (framer->fSyncWithInputSource) {
    framer->resetPresentationTime(presentationTime);
  }
  framer->continueReadProcessing();
}

void MPEG1or2AudioStreamFramer::continueReadProcessing() {
  unsigned acquiredFrameSize = fParser->parse(fNumTruncatedBytes);
  if (acquiredFrameSize > 0) {
    // We were able to acquire a frame from the input.
    // It has already been copied to the reader's space.
    fFrameSize = acquiredFrameSize;

    // Also set the presentation time, and increment it for next time,
    // based on the length of this frame:
    fPresentationTime = fNextFramePresentationTime;
    struct timeval framePlayTime = currentFramePlayTime();
    fDurationInMicroseconds = framePlayTime.tv_sec*MILLION + framePlayTime.tv_usec;
    fNextFramePresentationTime.tv_usec += framePlayTime.tv_usec;
    fNextFramePresentationTime.tv_sec
      += framePlayTime.tv_sec + fNextFramePresentationTime.tv_usec/MILLION;
    fNextFramePresentationTime.tv_usec %= MILLION;

    // Call our own 'after getting' function.  Because we're not a 'leaf'
    // source, we can call this directly, without risking infinite recursion.
    afterGetting(this);
  } else {
    // We were unable to parse a complete frame from the input, because:
    // - we had to read more data from the source stream, or
    // - the source stream has ended.
  }
}


////////// MPEG1or2AudioStreamParser implementation //////////

MPEG1or2AudioStreamParser
::MPEG1or2AudioStreamParser(MPEG1or2AudioStreamFramer* usingSource,
			FramedSource* inputSource)
  : StreamParser(inputSource, FramedSource::handleClosure, usingSource,
		 &MPEG1or2AudioStreamFramer::continueReadProcessing, usingSource) {
}

MPEG1or2AudioStreamParser::~MPEG1or2AudioStreamParser() {
}

void MPEG1or2AudioStreamParser::registerReadInterest(unsigned char* to,
						 unsigned maxSize) {
  fTo = to;
  fMaxSize = maxSize;
}

unsigned MPEG1or2AudioStreamParser::parse(unsigned& numTruncatedBytes) {
  try {
    saveParserState();

    // We expect a MPEG audio header (first 11 bits set to 1) at the start:
    while (((fCurrentFrame.hdr = test4Bytes())&0xFFE00000) != 0xFFE00000) {
      skipBytes(1);
      saveParserState();
    }

    fCurrentFrame.setParamsFromHeader();

    // Copy the frame to the requested destination:
    unsigned frameSize = fCurrentFrame.frameSize + 4; // include header
    if (frameSize > fMaxSize) {
      numTruncatedBytes = frameSize - fMaxSize;
      frameSize = fMaxSize;
    } else {
      numTruncatedBytes = 0;
    }

    getBytes(fTo, frameSize);
    skipBytes(numTruncatedBytes);

    return frameSize;
  } catch (int /*e*/) {
#ifdef DEBUG
    fprintf(stderr, "MPEG1or2AudioStreamParser::parse() EXCEPTION (This is normal behavior - *not* an error)\n");
#endif
    return 0;  // the parsing got interrupted
  }
}
