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
// Copyright (c) 2001-2004 Live Networks, Inc.  All rights reserved.
// Windows implementation of a generic audio input device
// Base class for both library versions:
//     One that uses Windows' built-in software mixer; another that doesn't.
// Implementation

#include "WindowsAudioInputDevice_common.hh"
#include <GroupsockHelper.hh>

////////// WindowsAudioInputDevice_common implementation //////////

unsigned WindowsAudioInputDevice_common::_bitsPerSample = 16;

WindowsAudioInputDevice_common
::WindowsAudioInputDevice_common(UsageEnvironment& env, int inputPortNumber,
			  unsigned char bitsPerSample,
			  unsigned char numChannels,
			  unsigned samplingFrequency,
			  unsigned granularityInMS)
  : AudioInputDevice(env, bitsPerSample, numChannels, samplingFrequency, granularityInMS),
    fCurPortIndex(-1), fHaveStarted(False) {
  _bitsPerSample = bitsPerSample;
}

WindowsAudioInputDevice_common::~WindowsAudioInputDevice_common() {
}

Boolean WindowsAudioInputDevice_common::initialSetInputPort(int portIndex) {
  if (!setInputPort(portIndex)) {
    char errMsgPrefix[100];
    sprintf(errMsgPrefix, "Failed to set audio input port number to %d: ", portIndex);
    char* errMsgSuffix = strDup(envir().getResultMsg());
    envir().setResultMsg(errMsgPrefix, errMsgSuffix);
    delete[] errMsgSuffix;
    return False;
  } else {
    return True;
  }
}

void WindowsAudioInputDevice_common::doGetNextFrame() {
  if (!fHaveStarted) {
    // Before reading the first audio data, flush any existing data:
    while (readHead != NULL) releaseHeadBuffer();
    fHaveStarted = True;
  }
  fTotalPollingDelay = 0;
  audioReadyPoller1();
}

void WindowsAudioInputDevice_common::doStopGettingFrames() {
  // Turn off the audio poller:
  envir().taskScheduler().unscheduleDelayedTask(nextTask()); nextTask() = NULL;
}

double WindowsAudioInputDevice_common::getAverageLevel() const {
  // If the input audio queue is empty, return the previous level,
  // otherwise use the input queue to recompute "averageLevel":
  if (readHead != NULL) {
    double levelTotal = 0.0;
    unsigned totNumSamples = 0;
    WAVEHDR* curHdr = readHead;
    while (1) {
      short* samplePtr = (short*)(curHdr->lpData);
      unsigned numSamples = blockSize/2;
      totNumSamples += numSamples;

      while (numSamples-- > 0) {
	short sample = *samplePtr++;
	if (sample < 0) sample = -sample;
	levelTotal += (unsigned short)sample;
      }

      if (curHdr == readTail) break;
      curHdr = curHdr->lpNext;
    }
    averageLevel = levelTotal/(totNumSamples*(double)0x8000);
  }
  return averageLevel;
}

void WindowsAudioInputDevice_common::audioReadyPoller(void* clientData) {
  WindowsAudioInputDevice_common* inputDevice = (WindowsAudioInputDevice_common*)clientData;
  inputDevice->audioReadyPoller1();
}

void WindowsAudioInputDevice_common::audioReadyPoller1() {
  if (readHead != NULL) {
    onceAudioIsReady();
  } else {
    unsigned const maxPollingDelay = (100 + fGranularityInMS)*1000;
    if (fTotalPollingDelay > maxPollingDelay) {
      // We've waited too long for the audio device - assume it's down:
      handleClosure(this);
      return;
    }

    // Try again after a short delay:
    unsigned const uSecondsToDelay = fGranularityInMS*1000;
    fTotalPollingDelay += uSecondsToDelay;
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecondsToDelay,
							     (TaskFunc*)audioReadyPoller, this);
  }
}

void WindowsAudioInputDevice_common::onceAudioIsReady() {
  fFrameSize = readFromBuffers(fTo, fMaxSize, fPresentationTime);
  if (fFrameSize == 0) {
    // The source is no longer readable
    handleClosure(this);
    return;
  }
  fDurationInMicroseconds = 1000000/fSamplingFrequency;

  // Call our own 'after getting' function.  Because we sometimes get here
  // after returning from a delay, we can call this directly, without risking
  // infinite recursion
  afterGetting(this);
}

static void CALLBACK waveInCallback(HWAVEIN /*hwi*/, UINT uMsg,
				    DWORD /*dwInstance*/, DWORD dwParam1, DWORD /*dwParam2*/) {
  switch (uMsg) {
  case WIM_DATA:
    WAVEHDR* hdr = (WAVEHDR*)dwParam1;
    WindowsAudioInputDevice_common::waveInProc(hdr);
    break;
  }
}

Boolean WindowsAudioInputDevice_common::openWavInPort(int index, unsigned numChannels, unsigned samplingFrequency, unsigned granularityInMS) {
	uSecsPerByte = (8*1e6)/(_bitsPerSample*numChannels*samplingFrequency);

	// Configure the port, based on the specified parameters:
    WAVEFORMATEX wfx;
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = numChannels;
    wfx.nSamplesPerSec  = samplingFrequency;
    wfx.wBitsPerSample  = _bitsPerSample;
    wfx.nBlockAlign     = (numChannels*_bitsPerSample)/8;
    wfx.nAvgBytesPerSec = samplingFrequency*wfx.nBlockAlign;
    wfx.cbSize          = 0;

    blockSize = (wfx.nAvgBytesPerSec*granularityInMS)/1000;

    // Use a 10-second input buffer, to allow for CPU competition from video, etc.,
    // and also for some audio cards that buffer as much as 5 seconds of audio.
    unsigned const bufferSeconds = 10;
    numBlocks = (bufferSeconds*1000)/granularityInMS;

    if (!waveIn_open(index, wfx)) return False;

    // Set this process's priority high. I'm not sure how much this is really needed,
    // but the "rat" code does this:
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	return True;
}

Boolean WindowsAudioInputDevice_common::waveIn_open(unsigned uid, WAVEFORMATEX& wfx) {
  if (shWaveIn != NULL) return True; // already open

  do {
    waveIn_reset();
    if (waveInOpen(&shWaveIn, uid, &wfx,
		   (DWORD)waveInCallback, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) break;

    // Allocate read buffers, and headers:
    readData = new unsigned char[numBlocks*blockSize];
    if (readData == NULL) break;

    readHdrs = new WAVEHDR[numBlocks];
    if (readHdrs == NULL) break;
    readHead = readTail = NULL;

    readTimes = new struct timeval[numBlocks];
    if (readTimes == NULL) break;

    // Initialize headers:
    for (unsigned i = 0; i < numBlocks; ++i) {
      readHdrs[i].lpData = (char*)&readData[i*blockSize];
      readHdrs[i].dwBufferLength = blockSize;
      readHdrs[i].dwFlags = 0;
      if (waveInPrepareHeader(shWaveIn, &readHdrs[i], sizeof (WAVEHDR)) != MMSYSERR_NOERROR) break;
      if (waveInAddBuffer(shWaveIn, &readHdrs[i], sizeof (WAVEHDR)) != MMSYSERR_NOERROR) break;
    }

    if (waveInStart(shWaveIn) != MMSYSERR_NOERROR) break;

    hAudioReady = CreateEvent(NULL, TRUE, FALSE, "waveIn Audio Ready");
    return True;
  } while (0);

  waveIn_reset();
  return False;
}

void WindowsAudioInputDevice_common::waveIn_close() {
  if (shWaveIn == NULL) return; // already closed

  waveInStop(shWaveIn);
  waveInReset(shWaveIn);

  for (unsigned i = 0; i < numBlocks; ++i) {
    if (readHdrs[i].dwFlags & WHDR_PREPARED) {
      waveInUnprepareHeader(shWaveIn, &readHdrs[i], sizeof (WAVEHDR));
    }
  }

  waveInClose(shWaveIn);
  waveIn_reset();
}

void WindowsAudioInputDevice_common::waveIn_reset() {
  shWaveIn = NULL;

  delete[] readData; readData = NULL;
  bytesUsedAtReadHead = 0;

  delete[] readHdrs; readHdrs = NULL;
  readHead = readTail = NULL;

  delete[] readTimes; readTimes = NULL;

  hAudioReady = NULL;
}

unsigned WindowsAudioInputDevice_common::readFromBuffers(unsigned char* to, unsigned numBytesWanted, struct timeval& creationTime) {
  // Begin by computing the creation time of (the first bytes of) this returned audio data:
  if (readHead != NULL) {
    int hdrIndex = readHead - readHdrs;
    creationTime = readTimes[hdrIndex];

    // Adjust this time to allow for any data that's already been read from this buffer:
    if (bytesUsedAtReadHead > 0) {
      creationTime.tv_usec += (unsigned)(uSecsPerByte*bytesUsedAtReadHead);
      creationTime.tv_sec += creationTime.tv_usec/1000000;
      creationTime.tv_usec %= 1000000;
    }
  }

  // Then, read from each available buffer, until we have the data that we want:
  unsigned numBytesRead = 0;
  while (readHead != NULL && numBytesRead < numBytesWanted) {
    unsigned thisRead = min(readHead->dwBytesRecorded - bytesUsedAtReadHead, numBytesWanted - numBytesRead);
    memmove(&to[numBytesRead], &readHead->lpData[bytesUsedAtReadHead], thisRead);
    numBytesRead += thisRead;
    bytesUsedAtReadHead += thisRead;
    if (bytesUsedAtReadHead == readHead->dwBytesRecorded) {
      // We're finished with the block; give it back to the device:
      releaseHeadBuffer();
    }
  }

  return numBytesRead;
}

void WindowsAudioInputDevice_common::releaseHeadBuffer() {
  WAVEHDR* toRelease = readHead;
  if (readHead == NULL) return;

  readHead = readHead->lpNext;
  if (readHead == NULL) readTail = NULL;

  toRelease->lpNext = NULL;
  toRelease->dwBytesRecorded = 0;
  toRelease->dwFlags &= ~WHDR_DONE;
  waveInAddBuffer(shWaveIn, toRelease, sizeof (WAVEHDR));
  bytesUsedAtReadHead = 0;
}

void WindowsAudioInputDevice_common::waveInProc(WAVEHDR* hdr) {
  unsigned hdrIndex = hdr - readHdrs;

  // Record the time that the data arrived:
  int dontCare;
  gettimeofday(&readTimes[hdrIndex], &dontCare);

  // Add the block to the tail of the queue:
  hdr->lpNext = NULL;
  if (readTail != NULL) {
    readTail->lpNext = hdr;
    readTail = hdr;
  } else {
    readHead = readTail = hdr;
  }
  SetEvent(hAudioReady);
}

HWAVEIN WindowsAudioInputDevice_common::shWaveIn = NULL;

unsigned WindowsAudioInputDevice_common::blockSize = 0;
unsigned WindowsAudioInputDevice_common::numBlocks = 0;

unsigned char* WindowsAudioInputDevice_common::readData = NULL;
DWORD WindowsAudioInputDevice_common::bytesUsedAtReadHead = 0;
double WindowsAudioInputDevice_common::uSecsPerByte = 0.0;
double WindowsAudioInputDevice_common::averageLevel = 0.0;

WAVEHDR* WindowsAudioInputDevice_common::readHdrs = NULL;
WAVEHDR* WindowsAudioInputDevice_common::readHead = NULL;
WAVEHDR* WindowsAudioInputDevice_common::readTail = NULL;

struct timeval* WindowsAudioInputDevice_common::readTimes = NULL;

HANDLE WindowsAudioInputDevice_common::hAudioReady = NULL;
