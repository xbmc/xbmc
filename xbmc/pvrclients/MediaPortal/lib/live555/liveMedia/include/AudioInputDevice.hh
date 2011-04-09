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
// Generic audio input device (such as a microphone, or an input sound card)
// C++ header

#ifndef _AUDIO_INPUT_DEVICE_HH
#define _AUDIO_INPUT_DEVICE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class AudioPortNames {
public:
  AudioPortNames();
  virtual ~AudioPortNames();

  unsigned numPorts;
  char** portName;
};

class AudioInputDevice: public FramedSource {
public:
  unsigned char bitsPerSample() const { return fBitsPerSample; }
  unsigned char numChannels() const { return fNumChannels; }
  unsigned samplingFrequency() const { return fSamplingFrequency; }

  virtual Boolean setInputPort(int portIndex) = 0;
  virtual double getAverageLevel() const = 0;

  static AudioInputDevice*
  createNew(UsageEnvironment& env, int inputPortNumber,
	    unsigned char bitsPerSample, unsigned char numChannels,
	    unsigned samplingFrequency, unsigned granularityInMS = 20);
  static AudioPortNames* getPortNames();

  static char** allowedDeviceNames;
  // If this is set to non-NULL, then it's a NULL-terminated array of strings
  // of device names that we are allowed to access.

protected:
  AudioInputDevice(UsageEnvironment& env,
		   unsigned char bitsPerSample,
		   unsigned char numChannels,
		   unsigned samplingFrequency,
		   unsigned granularityInMS);
	// we're an abstract base class

  virtual ~AudioInputDevice();

protected:
  unsigned char fBitsPerSample, fNumChannels;
  unsigned fSamplingFrequency;
  unsigned fGranularityInMS;
};

#endif
