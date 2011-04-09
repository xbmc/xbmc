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
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Windows implementation of a generic audio input device
// Base class for both library versions:
//     One that uses Windows' built-in software mixer; another that doesn't.
// C++ header

#ifndef _WINDOWS_AUDIO_INPUT_DEVICE_COMMON_HH
#define _WINDOWS_AUDIO_INPUT_DEVICE_COMMON_HH

#ifndef _AUDIO_INPUT_DEVICE_HH
#include "AudioInputDevice.hh"
#endif

class WindowsAudioInputDevice_common: public AudioInputDevice {
public:
  static Boolean openWavInPort(int index, unsigned numChannels, unsigned samplingFrequency, unsigned granularityInMS);
  static void waveIn_close();
  static void waveInProc(WAVEHDR* hdr); // Windows audio callback function

protected:
  WindowsAudioInputDevice_common(UsageEnvironment& env, int inputPortNumber,
	unsigned char bitsPerSample, unsigned char numChannels,
	unsigned samplingFrequency, unsigned granularityInMS);
	// virtual base class

  virtual ~WindowsAudioInputDevice_common();

  Boolean initialSetInputPort(int portIndex);

protected:
  int fCurPortIndex;

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();
  virtual double getAverageLevel() const;

private:
  static void audioReadyPoller(void* clientData);

  void audioReadyPoller1();
  void onceAudioIsReady();

  // Audio input buffering:
  static Boolean waveIn_open(unsigned uid, WAVEFORMATEX& wfx);
  static void waveIn_reset(); // used to implement both of the above
  static unsigned readFromBuffers(unsigned char* to, unsigned numBytesWanted, struct timeval& creationTime);
  static void releaseHeadBuffer(); // from the input header queue

private:
  static unsigned _bitsPerSample;
  static HWAVEIN shWaveIn;
  static unsigned blockSize, numBlocks;
  static unsigned char* readData; // buffer for incoming audio data
  static DWORD bytesUsedAtReadHead; // number of bytes that have already been read at head
  static double uSecsPerByte; // used to adjust the time for # bytes consumed since arrival
  static double averageLevel;
  static WAVEHDR *readHdrs, *readHead, *readTail; // input header queue
  static struct timeval* readTimes;
  static HANDLE hAudioReady; // audio ready event

  Boolean fHaveStarted;
  unsigned fTotalPollingDelay; // uSeconds
};

#endif
