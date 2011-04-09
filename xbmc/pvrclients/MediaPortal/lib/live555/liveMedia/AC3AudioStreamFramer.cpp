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
// A filter that breaks up an AC3 audio elementary stream into frames
// Implementation

#include "AC3AudioStreamFramer.hh"
#include "StreamParser.hh"
#include <GroupsockHelper.hh>

////////// AC3AudioStreamParser definition //////////

class AC3FrameParams {
public:
  AC3FrameParams() : samplingFreq(0) {}
  // 8-byte header at the start of each frame:
  //  u_int32_t hdr0, hdr1;
  unsigned hdr0, hdr1;

  // parameters derived from the headers
  unsigned kbps, samplingFreq, frameSize;

  void setParamsFromHeader();
};

class AC3AudioStreamParser: public StreamParser {
public:
  AC3AudioStreamParser(AC3AudioStreamFramer* usingSource,
			FramedSource* inputSource);
  virtual ~AC3AudioStreamParser();

public:
  Boolean testStreamCode(unsigned char ourStreamCode,
			 unsigned char* ptr, unsigned size);
     // returns True iff the initial stream code is ours
  unsigned parseFrame(unsigned& numTruncatedBytes);
     // returns the size of the frame that was acquired, or 0 if none was

  void registerReadInterest(unsigned char* to, unsigned maxSize);

  AC3FrameParams const& currentFrame() const { return fCurrentFrame; }

  Boolean haveParsedAFrame() const { return fHaveParsedAFrame; }
  void readAndSaveAFrame();

private:
  static void afterGettingSavedFrame(void* clientData, unsigned frameSize,
				     unsigned numTruncatedBytes,
                                     struct timeval presentationTime,
				     unsigned durationInMicroseconds);
  void afterGettingSavedFrame1(unsigned frameSize);
  static void onSavedFrameClosure(void* clientData);
  void onSavedFrameClosure1();

private:
  AC3AudioStreamFramer* fUsingSource;
  unsigned char* fTo;
  unsigned fMaxSize;

  Boolean fHaveParsedAFrame;
  unsigned char* fSavedFrame;
  unsigned fSavedFrameSize;
  char fSavedFrameFlag;

  // Parameters of the most recently read frame:
  AC3FrameParams fCurrentFrame;
};


////////// AC3AudioStreamFramer implementation //////////

AC3AudioStreamFramer::AC3AudioStreamFramer(UsageEnvironment& env,
					   FramedSource* inputSource,
					   unsigned char streamCode)
  : FramedFilter(env, inputSource), fOurStreamCode(streamCode) {
  // Use the current wallclock time as the initial 'presentation time':
  gettimeofday(&fNextFramePresentationTime, NULL);

  fParser = new AC3AudioStreamParser(this, inputSource);
}

AC3AudioStreamFramer::~AC3AudioStreamFramer() {
  delete fParser;
}

AC3AudioStreamFramer*
AC3AudioStreamFramer::createNew(UsageEnvironment& env,
				FramedSource* inputSource,
				unsigned char streamCode) {
  // Need to add source type checking here???  #####
  return new AC3AudioStreamFramer(env, inputSource, streamCode);
}

unsigned AC3AudioStreamFramer::samplingRate() {
  if (!fParser->haveParsedAFrame()) {
    // Because we haven't yet parsed a frame, we don't yet know the input
    // stream's sampling rate.  So, we first need to read a frame
    // (into a special buffer that we keep around for later use).
    fParser->readAndSaveAFrame();
  }

  return fParser->currentFrame().samplingFreq;
}

void AC3AudioStreamFramer::flushInput() {
  fParser->flushInput();
}

void AC3AudioStreamFramer::doGetNextFrame() {
  fParser->registerReadInterest(fTo, fMaxSize);
  parseNextFrame();
}

#define MILLION 1000000

struct timeval AC3AudioStreamFramer::currentFramePlayTime() const {
  AC3FrameParams const& fr = fParser->currentFrame();
  unsigned const numSamples = 1536;
  unsigned const freq = fr.samplingFreq;

  // result is numSamples/freq
  unsigned const uSeconds = (freq == 0) ? 0
    : ((numSamples*2*MILLION)/freq + 1)/2; // rounds to nearest integer

  struct timeval result;
  result.tv_sec = uSeconds/MILLION;
  result.tv_usec = uSeconds%MILLION;
  return result;
}

void AC3AudioStreamFramer
::handleNewData(void* clientData, unsigned char* ptr, unsigned size,
		struct timeval /*presentationTime*/) {
  AC3AudioStreamFramer* framer = (AC3AudioStreamFramer*)clientData;
  framer->handleNewData(ptr, size);
}

void AC3AudioStreamFramer
::handleNewData(unsigned char* ptr, unsigned size) {
  if (!fParser->testStreamCode(fOurStreamCode, ptr, size)) {
    // This block of data is not for us; try again:
    parseNextFrame();
    return;
  }

  // Now that we know that this data is for us, get the next frame:
  parseNextFrame();
}

void AC3AudioStreamFramer::parseNextFrame() {
  unsigned acquiredFrameSize = fParser->parseFrame(fNumTruncatedBytes);
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


////////// AC3AudioStreamParser implementation //////////

static int const kbpsTable[] = {32,  40,  48,  56,  64,  80,  96, 112,
				128, 160, 192, 224, 256, 320, 384, 448,
				512, 576, 640};

void AC3FrameParams::setParamsFromHeader() {
  unsigned char byte4 = hdr1 >> 24;

  unsigned char kbpsIndex = (byte4&0x3E) >> 1;
  if (kbpsIndex > 18) kbpsIndex = 18;
  kbps = kbpsTable[kbpsIndex];

  unsigned char samplingFreqIndex = (byte4&0xC0) >> 6;
  switch (samplingFreqIndex) {
  case 0:
    samplingFreq = 48000;
    frameSize = 4*kbps;
    break;
  case 1:
    samplingFreq = 44100;
    frameSize = 2*(320*kbps/147 + (byte4&1));
    break;
  case 2:
  case 3: // not legal?
    samplingFreq = 32000;
    frameSize = 6*kbps;
  }
}

AC3AudioStreamParser
::AC3AudioStreamParser(AC3AudioStreamFramer* usingSource,
			FramedSource* inputSource)
  : StreamParser(inputSource, FramedSource::handleClosure, usingSource,
		 &AC3AudioStreamFramer::handleNewData, usingSource),
    fUsingSource(usingSource), fHaveParsedAFrame(False),
    fSavedFrame(NULL), fSavedFrameSize(0) {
}

AC3AudioStreamParser::~AC3AudioStreamParser() {
}

void AC3AudioStreamParser::registerReadInterest(unsigned char* to,
						 unsigned maxSize) {
  fTo = to;
  fMaxSize = maxSize;
}

Boolean AC3AudioStreamParser
::testStreamCode(unsigned char ourStreamCode,
		 unsigned char* ptr, unsigned size) {
  if (size < 4) return False; // shouldn't happen
  unsigned char streamCode = *ptr;

  if (streamCode == ourStreamCode) {
    // Remove the first 4 bytes from the stream:
    memmove(ptr, ptr + 4, size - 4);
    totNumValidBytes() = totNumValidBytes() - 4;

    return True;
  } else {
    // Discard all of the data that was just read:
    totNumValidBytes() = totNumValidBytes() - size;

    return False;
  }
}

unsigned AC3AudioStreamParser::parseFrame(unsigned& numTruncatedBytes) {
  if (fSavedFrameSize > 0) {
    // We've already read and parsed a frame.  Use it instead:
    memmove(fTo, fSavedFrame, fSavedFrameSize);
    delete[] fSavedFrame; fSavedFrame = NULL;
    unsigned frameSize = fSavedFrameSize;
    fSavedFrameSize = 0;
    return frameSize;
  }

  try {
    saveParserState();

    // We expect an AC3 audio header (first 2 bytes == 0x0B77) at the start:
    while (1) {
      unsigned next4Bytes = test4Bytes();
      if (next4Bytes>>16 == 0x0B77) break;
      skipBytes(1);
      saveParserState();
    }
    fCurrentFrame.hdr0 = get4Bytes();
    fCurrentFrame.hdr1 = test4Bytes();

    fCurrentFrame.setParamsFromHeader();
    fHaveParsedAFrame = True;

    // Copy the frame to the requested destination:
    unsigned frameSize = fCurrentFrame.frameSize;
    if (frameSize > fMaxSize) {
      numTruncatedBytes = frameSize - fMaxSize;
      frameSize = fMaxSize;
    } else {
      numTruncatedBytes = 0;
    }

    fTo[0] = fCurrentFrame.hdr0 >> 24;
    fTo[1] = fCurrentFrame.hdr0 >> 16;
    fTo[2] = fCurrentFrame.hdr0 >> 8;
    fTo[3] = fCurrentFrame.hdr0;
    getBytes(&fTo[4], frameSize-4);
    skipBytes(numTruncatedBytes);

    return frameSize;
  } catch (int /*e*/) {
#ifdef DEBUG
    fUsingSource->envir() << "AC3AudioStreamParser::parseFrame() EXCEPTION (This is normal behavior - *not* an error)\n";
#endif
    return 0;  // the parsing got interrupted
  }
}

void AC3AudioStreamParser::readAndSaveAFrame() {
  unsigned const maxAC3FrameSize = 4000;
  fSavedFrame = new unsigned char[maxAC3FrameSize];
  fSavedFrameSize = 0;

  fSavedFrameFlag = 0;
  fUsingSource->getNextFrame(fSavedFrame, maxAC3FrameSize,
			     afterGettingSavedFrame, this,
			     onSavedFrameClosure, this);
  fUsingSource->envir().taskScheduler().doEventLoop(&fSavedFrameFlag);
}

void AC3AudioStreamParser
::afterGettingSavedFrame(void* clientData, unsigned frameSize,
			 unsigned /*numTruncatedBytes*/,
			 struct timeval /*presentationTime*/,
			 unsigned /*durationInMicroseconds*/) {
  AC3AudioStreamParser* parser = (AC3AudioStreamParser*)clientData;
  parser->afterGettingSavedFrame1(frameSize);
}

void AC3AudioStreamParser
::afterGettingSavedFrame1(unsigned frameSize) {
  fSavedFrameSize = frameSize;
  fSavedFrameFlag = ~0;
}

void AC3AudioStreamParser::onSavedFrameClosure(void* clientData) {
  AC3AudioStreamParser* parser = (AC3AudioStreamParser*)clientData;
  parser->onSavedFrameClosure1();
}

void AC3AudioStreamParser::onSavedFrameClosure1() {
  delete[] fSavedFrame; fSavedFrame = NULL;
  fSavedFrameSize = 0;
  fSavedFrameFlag = ~0;
}
