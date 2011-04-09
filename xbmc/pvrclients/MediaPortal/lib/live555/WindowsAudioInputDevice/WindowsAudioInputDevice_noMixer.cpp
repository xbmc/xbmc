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
// This version does not use Windows' built-in software mixer.
// Implementation

#include <WindowsAudioInputDevice_noMixer.hh>

////////// AudioInputPort definition //////////

class AudioInputPort {
public:
  void open(unsigned numChannels, unsigned samplingFrequency, unsigned granularityInMS);
  void open(); // open with default parameters
  void close();

public:
  int index;
  char name[MAXPNAMELEN];
};


////////// AudioInputDevice (remaining) implementation //////////

AudioInputDevice*
AudioInputDevice::createNew(UsageEnvironment& env, int inputPortNumber,
			    unsigned char bitsPerSample,
			    unsigned char numChannels,
			    unsigned samplingFrequency,
			    unsigned granularityInMS) {
  Boolean success;
  WindowsAudioInputDevice* newSource
    = new WindowsAudioInputDevice(env, inputPortNumber,
				  bitsPerSample, numChannels,
				  samplingFrequency, granularityInMS,
				  success);
  if (!success) {delete newSource; newSource = NULL;}

  return newSource;
}

AudioPortNames* AudioInputDevice::getPortNames() {
  WindowsAudioInputDevice::initializeIfNecessary();

  AudioPortNames* portNames = new AudioPortNames;
  portNames->numPorts = WindowsAudioInputDevice::numAudioInputPorts;
  portNames->portName = new char*[WindowsAudioInputDevice::numAudioInputPorts];

  for (unsigned i = 0; i < WindowsAudioInputDevice::numAudioInputPorts; ++i) {
    AudioInputPort& audioInputPort = WindowsAudioInputDevice::ourAudioInputPorts[i];

    portNames->portName[i] = strDup(audioInputPort.name);
  }

  return portNames;
}


////////// WindowsAudioInputDevice implementation //////////

WindowsAudioInputDevice
::WindowsAudioInputDevice(UsageEnvironment& env, int inputPortNumber,
			  unsigned char bitsPerSample,
			  unsigned char numChannels,
			  unsigned samplingFrequency,
			  unsigned granularityInMS,
			  Boolean& success)
  : WindowsAudioInputDevice_common(env, inputPortNumber,
		bitsPerSample, numChannels, samplingFrequency, granularityInMS) {
	success = initialSetInputPort(inputPortNumber);
}

WindowsAudioInputDevice::~WindowsAudioInputDevice() {
  if (fCurPortIndex >= 0) ourAudioInputPorts[fCurPortIndex].close();

  delete[] ourAudioInputPorts; ourAudioInputPorts = NULL;
  numAudioInputPorts = 0;
}

void WindowsAudioInputDevice::initializeIfNecessary() {
  if (ourAudioInputPorts != NULL) return; // we've already been initialized
  numAudioInputPorts = waveInGetNumDevs();
  ourAudioInputPorts = new AudioInputPort[numAudioInputPorts];

  // Initialize each audio input port
  for (unsigned i = 0; i < numAudioInputPorts; ++i) {
    AudioInputPort& port = ourAudioInputPorts[i];
    port.index = i;
    port.open(); // to set the port name
    port.close();
  }
}

Boolean WindowsAudioInputDevice::setInputPort(int portIndex) {
  initializeIfNecessary();

  if (portIndex < 0 || portIndex >= (int)numAudioInputPorts) { // bad index
    envir().setResultMsg("Bad input port index\n");
    return False;
  }

  // Check that this port is allowed:
  if (allowedDeviceNames != NULL) {
	  int i;
	  for (i = 0; allowedDeviceNames[i] != NULL; ++i) {
		  if (strncmp(ourAudioInputPorts[portIndex].name, allowedDeviceNames[i],
			  strlen(allowedDeviceNames[i])) == 0) {
			  // The allowed device name is a prefix of this port's name
			  break; // this port is allowed
		  }
	  }
	  if (allowedDeviceNames[i] == NULL) { // this port is not on the allowed list
		envir().setResultMsg("Access to this audio device is not allowed\n");
		return False;
	  }
  }

  if (portIndex != fCurPortIndex) {
    // The port has changed, so close the old one and open the new one:
    if (fCurPortIndex >= 0) ourAudioInputPorts[fCurPortIndex].close();;
    fCurPortIndex = portIndex;
    ourAudioInputPorts[fCurPortIndex].open(fNumChannels, fSamplingFrequency, fGranularityInMS);
  }
  fCurPortIndex = portIndex;
  return True;
}

unsigned WindowsAudioInputDevice::numAudioInputPorts = 0;

AudioInputPort* WindowsAudioInputDevice::ourAudioInputPorts = NULL;


////////// AudioInputPort implementation //////////

void AudioInputPort::open(unsigned numChannels, unsigned samplingFrequency, unsigned granularityInMS) {
  do {
    // Get the port name:
    WAVEINCAPS wic;
    if (waveInGetDevCaps(index, &wic, sizeof wic) != MMSYSERR_NOERROR) {
	    name[0] = '\0';
	    break;
	}
    strncpy(name, wic.szPname, MAXPNAMELEN);

	if (!WindowsAudioInputDevice_common::openWavInPort(index, numChannels, samplingFrequency, granularityInMS)) break;

    return;
  } while (0);

  // An error occurred:
  close();
}

void AudioInputPort::open() {
  open(1, 8000, 20);
}

void AudioInputPort::close() {
  WindowsAudioInputDevice_common::waveIn_close();
}
