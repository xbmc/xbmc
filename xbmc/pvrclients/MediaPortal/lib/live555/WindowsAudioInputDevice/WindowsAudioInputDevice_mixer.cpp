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
// This version uses Windows' built-in software mixer.
// Implementation

#include <WindowsAudioInputDevice_mixer.hh>

////////// Mixer and AudioInputPort definition //////////

class AudioInputPort {
public:
  int tag;
  DWORD dwComponentType;
  char name[MIXER_LONG_NAME_CHARS];
};

class Mixer {
public:
  Mixer();
  virtual ~Mixer();

  void open(unsigned numChannels, unsigned samplingFrequency, unsigned granularityInMS);
  void open(); // open with default parameters
  void getPortsInfo();
  Boolean enableInputPort(unsigned portIndex, char const*& errReason, MMRESULT& errCode);
  void close();

  unsigned index;
  HMIXER hMixer; // valid when open
  DWORD dwRecLineID; // valid when open
  unsigned numPorts;
  AudioInputPort* ports;
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
  portNames->numPorts = WindowsAudioInputDevice::numInputPortsTotal;
  portNames->portName = new char*[WindowsAudioInputDevice::numInputPortsTotal];

  // If there's more than one mixer, print only the port name.
  // If there's two or more mixers, also include the mixer name
  // (to disambiguate port names that may be the same name in different mixers)
  char portNameBuffer[2*MAXPNAMELEN+10/*slop*/];
  char mixerNameBuffer[MAXPNAMELEN];
  char const* portNameFmt;
  if (WindowsAudioInputDevice::numMixers <= 1) {
    portNameFmt = "%s";
  } else {
    portNameFmt = "%s (%s)";
  }

  unsigned curPortNum = 0;
  for (unsigned i = 0; i < WindowsAudioInputDevice::numMixers; ++i) {
    Mixer& mixer = WindowsAudioInputDevice::ourMixers[i];

    if (WindowsAudioInputDevice::numMixers <= 1) {
      mixerNameBuffer[0] = '\0';
    } else {
      strncpy(mixerNameBuffer, mixer.name, sizeof mixerNameBuffer);
#if 0
      // Hack: Simplify the mixer name, by truncating after the first space character:
      for (int k = 0; k < sizeof mixerNameBuffer && mixerNameBuffer[k] != '\0'; ++k) {
	if (mixerNameBuffer[k] == ' ') {
	  mixerNameBuffer[k] = '\0';
	  break;
	}
      }
#endif
    }

    for (unsigned j = 0; j < mixer.numPorts; ++j) {
      sprintf(portNameBuffer, portNameFmt, mixer.ports[j].name, mixerNameBuffer);
      portNames->portName[curPortNum++] = strDup(portNameBuffer);
    }
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
		bitsPerSample, numChannels, samplingFrequency, granularityInMS),
	fCurMixerId(-1) {
	success = initialSetInputPort(inputPortNumber);
}

WindowsAudioInputDevice::~WindowsAudioInputDevice() {
  if (fCurMixerId >= 0) ourMixers[fCurMixerId].close();

  delete[] ourMixers; ourMixers = NULL;
  numMixers = numInputPortsTotal = 0;
}

void WindowsAudioInputDevice::initializeIfNecessary() {
  if (ourMixers != NULL) return; // we've already been initialized
  numMixers = mixerGetNumDevs();
  ourMixers = new Mixer[numMixers];

  // Initialize each mixer:
  numInputPortsTotal = 0;
  for (unsigned i = 0; i < numMixers; ++i) {
    Mixer& mixer = ourMixers[i];
    mixer.index = i;
    mixer.open();
    if (mixer.hMixer != NULL) {
      // This device has a valid mixer.  Get information about its ports:
      mixer.getPortsInfo();
      mixer.close();

      if (mixer.numPorts == 0) continue;

      numInputPortsTotal += mixer.numPorts;
    } else {
      mixer.ports = NULL;
      mixer.numPorts = 0;
    }
  }
}

Boolean WindowsAudioInputDevice::setInputPort(int portIndex) {
  initializeIfNecessary();

  if (portIndex < 0 || portIndex >= (int)numInputPortsTotal) { // bad index
    envir().setResultMsg("Bad input port index\n");
    return False;
  }

  // Find the mixer and port that corresponds to "portIndex":
  int newMixerId, portWithinMixer, portIndexCount = 0;
  for (newMixerId = 0; newMixerId < (int)numMixers; ++newMixerId) {
    int prevPortIndexCount = portIndexCount;
    portIndexCount += ourMixers[newMixerId].numPorts;
    if (portIndexCount > portIndex) { // it's with this mixer
      portWithinMixer = portIndex - prevPortIndexCount;
      break;
    }
  }

  // Check that this mixer is allowed:
  if (allowedDeviceNames != NULL) {
	  int i;
	  for (i = 0; allowedDeviceNames[i] != NULL; ++i) {
		  if (strncmp(ourMixers[newMixerId].name, allowedDeviceNames[i],
			  strlen(allowedDeviceNames[i])) == 0) {
			  // The allowed device name is a prefix of this mixer's name
			  break; // this mixer is allowed
		  }
	  }
	  if (allowedDeviceNames[i] == NULL) { // this mixer is not on the allowed list
		envir().setResultMsg("Access to this audio device is not allowed\n");
		return False;
	  }
  }

  if (newMixerId != fCurMixerId) {
    // The mixer has changed, so close the old one and open the new one:
    if (fCurMixerId >= 0) ourMixers[fCurMixerId].close();
    fCurMixerId = newMixerId;
    ourMixers[fCurMixerId].open(fNumChannels, fSamplingFrequency, fGranularityInMS);
  }
  if (portIndex != fCurPortIndex) {
    // Change the input port:
    fCurPortIndex = portIndex;
    char const* errReason;
    MMRESULT errCode;
    if (!ourMixers[newMixerId].enableInputPort(portWithinMixer, errReason, errCode)) {
      char resultMsg[100];
      sprintf(resultMsg, "Failed to enable input port: %s failed (0x%08x)\n", errReason, errCode);
      envir().setResultMsg(resultMsg);
      return False;
    }
    // Later, may also need to transfer 'gain' to new port #####
  }
  return True;
}

unsigned WindowsAudioInputDevice::numMixers = 0;

Mixer* WindowsAudioInputDevice::ourMixers = NULL;

unsigned WindowsAudioInputDevice::numInputPortsTotal = 0;


////////// Mixer and AudioInputPort implementation //////////

Mixer::Mixer()
  : hMixer(NULL), dwRecLineID(0), numPorts(0), ports(NULL) {
}

Mixer::~Mixer() {
  delete[] ports;
}

void Mixer::open(unsigned numChannels, unsigned samplingFrequency, unsigned granularityInMS) {
  HMIXER newHMixer = NULL;
  do {
    MIXERCAPS mc;
    if (mixerGetDevCaps(index, &mc, sizeof mc) != MMSYSERR_NOERROR) break;

    // Copy the mixer name:
    strncpy(name, mc.szPname, MAXPNAMELEN);

    // Find the correct line for this mixer:
    unsigned i, uWavIn;
    unsigned nWavIn = waveInGetNumDevs();
    for (i = 0; i < nWavIn; ++i) {
      WAVEINCAPS wic;
      if (waveInGetDevCaps(i, &wic, sizeof wic) != MMSYSERR_NOERROR) continue;

      MIXERLINE ml;
      ml.cbStruct = sizeof ml;
      ml.Target.dwType  = MIXERLINE_TARGETTYPE_WAVEIN;
      strncpy(ml.Target.szPname, wic.szPname, MAXPNAMELEN);
      ml.Target.vDriverVersion = wic.vDriverVersion;
      ml.Target.wMid = wic.wMid;
      ml.Target.wPid = wic.wPid;

      if (mixerGetLineInfo((HMIXEROBJ)index, &ml, MIXER_GETLINEINFOF_TARGETTYPE/*|MIXER_OBJECTF_MIXER*/) == MMSYSERR_NOERROR) {
				// this is the right line
	uWavIn = i;
	dwRecLineID = ml.dwLineID;
	break;
      }
    }
    if (i >= nWavIn) break; // error: we couldn't find the right line

    if (mixerOpen(&newHMixer, index, (unsigned long)NULL, (unsigned long)NULL, MIXER_OBJECTF_MIXER) != MMSYSERR_NOERROR) break;
    if (newHMixer == NULL) break;

    // Sanity check: re-call "mixerGetDevCaps()" using the mixer device handle:
    if (mixerGetDevCaps((UINT)newHMixer, &mc, sizeof mc) != MMSYSERR_NOERROR) break;
    if (mc.cDestinations < 1) break; // error: this mixer has no destinations

	if (!WindowsAudioInputDevice_common::openWavInPort(uWavIn, numChannels, samplingFrequency, granularityInMS)) break;

    hMixer = newHMixer;
    return;
  } while (0);

  // An error occurred:
  close();
}

void Mixer::open() {
  open(1, 8000, 20);
}

void Mixer::getPortsInfo() {
  MIXERCAPS mc;
  mixerGetDevCaps((UINT)hMixer, &mc, sizeof mc);

  MIXERLINE mlt;
  unsigned i;
  for (i = 0; i < mc.cDestinations; ++i) {
    memset(&mlt, 0, sizeof mlt);
    mlt.cbStruct = sizeof mlt;
    mlt.dwDestination = i;
    if (mixerGetLineInfo((HMIXEROBJ)hMixer, &mlt, MIXER_GETLINEINFOF_DESTINATION) != MMSYSERR_NOERROR) continue;
    if (mlt.dwLineID == dwRecLineID) break; // this is the destination we're interested in
  }
  ports = new AudioInputPort[mlt.cConnections];

  numPorts = mlt.cConnections;
  for (i = 0; i < numPorts; ++i) {
    MIXERLINE mlc;
    memcpy(&mlc, &mlt, sizeof mlc);
    mlc.dwSource = i;
    mixerGetLineInfo((HMIXEROBJ)hMixer, &mlc, MIXER_GETLINEINFOF_SOURCE/*|MIXER_OBJECTF_HMIXER*/);
    ports[i].tag = mlc.dwLineID;
	ports[i].dwComponentType = mlc.dwComponentType;
    strncpy(ports[i].name, mlc.szName, MIXER_LONG_NAME_CHARS);
  }

  // Make the microphone the first port in the list:
  for (i = 1; i < numPorts; ++i) {
#ifdef OLD_MICROPHONE_TESTING_CODE
    if (_strnicmp("mic", ports[i].name, 3) == 0 ||
	_strnicmp("mik", ports[i].name, 3) == 0) {
#else
	if (ports[i].dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE) {
#endif
      AudioInputPort tmp = ports[0];
      ports[0] = ports[i];
      ports[i] = tmp;
    }
  }
}

Boolean Mixer::enableInputPort(unsigned portIndex, char const*& errReason, MMRESULT& errCode) {
  errReason = NULL; // unless there's an error
  AudioInputPort& port = ports[portIndex];

  MIXERCONTROL mc;
  mc.cMultipleItems = 1; // in case it doesn't get set below
  MIXERLINECONTROLS mlc;
#if 0 // the following doesn't seem to be needed, and can fail:
  mlc.cbStruct = sizeof mlc;
  mlc.pamxctrl = &mc;
  mlc.cbmxctrl = sizeof (MIXERCONTROL);
  mlc.dwLineID = port.tag;
  mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
  if ((errCode = mixerGetLineControls((HMIXEROBJ)hMixer, &mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE/*|MIXER_OBJECTF_HMIXER*/)) != MMSYSERR_NOERROR) {
    errReason = "mixerGetLineControls()";
    return False;
  }
#endif

  MIXERLINE ml;
  memset(&ml, 0, sizeof (MIXERLINE));
  ml.cbStruct = sizeof (MIXERLINE);
  ml.dwLineID = port.tag;
  if ((errCode = mixerGetLineInfo((HMIXEROBJ)hMixer, &ml, MIXER_GETLINEINFOF_LINEID)) != MMSYSERR_NOERROR) {
    errReason = "mixerGetLineInfo()1";
    return False;
  }

  char portname[MIXER_LONG_NAME_CHARS+1];
  strncpy(portname, ml.szName, MIXER_LONG_NAME_CHARS);

  memset(&ml, 0, sizeof (MIXERLINE));
  ml.cbStruct = sizeof (MIXERLINE);
  ml.dwLineID = dwRecLineID;
  if ((errCode = mixerGetLineInfo((HMIXEROBJ)hMixer, &ml, MIXER_GETLINEINFOF_LINEID/*|MIXER_OBJECTF_HMIXER*/)) != MMSYSERR_NOERROR) {
    errReason = "mixerGetLineInfo()2";
    return False;
  }

  // Get Mixer/MUX control information (need control id to set and get control details)
  mlc.cbStruct = sizeof mlc;
  mlc.dwLineID = ml.dwLineID;
  mlc.cControls = 1;
  mc.cbStruct = sizeof mc; // Needed???#####
  mc.dwControlID = 0xDEADBEEF; // For testing #####
  mlc.pamxctrl = &mc;
  mlc.cbmxctrl = sizeof mc;
  mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX; // Single Select
  if ((errCode = mixerGetLineControls((HMIXEROBJ)hMixer, &mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE/*|MIXER_OBJECTF_HMIXER*/)) != MMSYSERR_NOERROR) {
    mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER; // Multiple Select
    mixerGetLineControls((HMIXEROBJ)hMixer, &mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE/*|MIXER_OBJECTF_HMIXER*/);
  }

  unsigned matchLine = 0;
  if (mc.cMultipleItems > 1) {
    // Before getting control, we need to know which line to grab.
    // We figure this out by listing the lines, and comparing names:
    MIXERCONTROLDETAILS mcd;
    mcd.cbStruct = sizeof mcd;
    mcd.cChannels = ml.cChannels;
    mcd.cMultipleItems = mc.cMultipleItems;
    MIXERCONTROLDETAILS_LISTTEXT* mcdlText = new MIXERCONTROLDETAILS_LISTTEXT[mc.cMultipleItems];
    mcd.cbDetails = sizeof (MIXERCONTROLDETAILS_LISTTEXT);
    mcd.paDetails = mcdlText;

    if (mc.dwControlID != 0xDEADBEEF) { // we know the control id for real
      mcd.dwControlID = mc.dwControlID;
      if ((errCode = mixerGetControlDetails((HMIXEROBJ)hMixer, &mcd, MIXER_GETCONTROLDETAILSF_LISTTEXT/*|MIXER_OBJECTF_HMIXER*/)) != MMSYSERR_NOERROR) {
	delete[] mcdlText;
	errReason = "mixerGetControlDetails()1";
	return False;
      }
    } else {
      // Hack: We couldn't find a MUX or MIXER control, so try to guess the control id:
      for (mc.dwControlID = 0; mc.dwControlID < 32; ++mc.dwControlID) {
	mcd.dwControlID = mc.dwControlID;
	if ((errCode = mixerGetControlDetails((HMIXEROBJ)hMixer, &mcd, MIXER_GETCONTROLDETAILSF_LISTTEXT/*|MIXER_OBJECTF_HMIXER*/)) == MMSYSERR_NOERROR) break;
      }
      if (mc.dwControlID == 32) { // unable to guess mux/mixer control id
	delete[] mcdlText;
	errReason = "mixerGetControlDetails()2";
	return False;
      }
    }

    for (unsigned i = 0; i < mcd.cMultipleItems; ++i) {
      if (strcmp(mcdlText[i].szName, portname) == 0) {
	matchLine = i;
	break;
      }
    }
    delete[] mcdlText;
  }

  // Now get control itself:
  MIXERCONTROLDETAILS mcd;
  mcd.cbStruct = sizeof mcd;
  mcd.dwControlID = mc.dwControlID;
  mcd.cChannels = ml.cChannels;
  mcd.cMultipleItems = mc.cMultipleItems;
  MIXERCONTROLDETAILS_BOOLEAN* mcdbState = new MIXERCONTROLDETAILS_BOOLEAN[mc.cMultipleItems];
  mcd.paDetails = mcdbState;
  mcd.cbDetails = sizeof (MIXERCONTROLDETAILS_BOOLEAN);

  if ((errCode = mixerGetControlDetails((HMIXEROBJ)hMixer, &mcd, MIXER_GETCONTROLDETAILSF_VALUE/*|MIXER_OBJECTF_HMIXER*/)) != MMSYSERR_NOERROR) {
    delete[] mcdbState;
    errReason = "mixerGetControlDetails()3";
    return False;
  }

  for (unsigned j = 0; j < mcd.cMultipleItems; ++j) {
    mcdbState[j].fValue = (j == matchLine);
  }

  if ((errCode = mixerSetControlDetails((HMIXEROBJ)hMixer, &mcd, MIXER_OBJECTF_HMIXER)) != MMSYSERR_NOERROR) {
    delete[] mcdbState;
    errReason = "mixerSetControlDetails()";
    return False;
  }
  delete[] mcdbState;

  return True;
}


void Mixer::close() {
  WindowsAudioInputDevice_common::waveIn_close();
  if (hMixer != NULL) mixerClose(hMixer);
  hMixer = NULL; dwRecLineID = 0;
}
