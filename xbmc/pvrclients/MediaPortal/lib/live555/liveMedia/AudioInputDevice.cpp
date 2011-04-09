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
// Copyright (c) 2001-2003 Live Networks, Inc.  All rights reserved.
// Generic audio input device (such as a microphone, or an input sound card)
// Implementation

#include <AudioInputDevice.hh>

AudioInputDevice
::AudioInputDevice(UsageEnvironment& env, unsigned char bitsPerSample,
		   unsigned char numChannels,
		   unsigned samplingFrequency, unsigned granularityInMS)
  : FramedSource(env), fBitsPerSample(bitsPerSample),
    fNumChannels(numChannels), fSamplingFrequency(samplingFrequency),
    fGranularityInMS(granularityInMS) {
}

AudioInputDevice::~AudioInputDevice() {
}

char** AudioInputDevice::allowedDeviceNames = NULL;

////////// AudioPortNames implementation //////////

AudioPortNames::AudioPortNames()
: numPorts(0), portName(NULL) {
}

AudioPortNames::~AudioPortNames() {
	for (unsigned i = 0; i < numPorts; ++i) delete portName[i];
	delete portName;
}
