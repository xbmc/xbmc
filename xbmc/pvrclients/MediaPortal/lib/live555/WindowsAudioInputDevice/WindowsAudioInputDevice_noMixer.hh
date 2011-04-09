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
// This version does not use Windows' built-in software mixer.
// C++ header
//
// To use this, call "AudioInputDevice::createNew()".
// You can also call "AudioInputDevice::getPortNames()" to get a list
// of port names.

#ifndef _WINDOWS_AUDIO_INPUT_DEVICE_NOMIXER_HH
#define _WINDOWS_AUDIO_INPUT_DEVICE_NOMIXER_HH

#ifndef _WINDOWS_AUDIO_INPUT_DEVICE_COMMON_HH
#include "WindowsAudioInputDevice_common.hh"
#endif

class WindowsAudioInputDevice: public WindowsAudioInputDevice_common {
private:
  friend class AudioInputDevice;
  WindowsAudioInputDevice(UsageEnvironment& env, int inputPortNumber,
	unsigned char bitsPerSample, unsigned char numChannels,
	unsigned samplingFrequency, unsigned granularityInMS,
	Boolean& success);
	// called only by createNew()

  virtual ~WindowsAudioInputDevice();

  static void initializeIfNecessary();

private:
  // redefined virtual functions:
  virtual Boolean setInputPort(int portIndex);

private:
  static unsigned numAudioInputPorts;
  static class AudioInputPort* ourAudioInputPorts;
};

#endif
