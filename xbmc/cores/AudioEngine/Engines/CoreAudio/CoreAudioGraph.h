#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#if defined(TARGET_DARWIN_OSX)
#include <list>

#include "ICoreAudioSource.h"
#include <AudioToolbox/AUGraph.h>
#include <CoreAudio/CoreAudio.h>

#define MAX_CONNECTION_LIMIT   8
#define MAXIMUM_MIXER_CHANNELS 9

class CAUMatrixMixer;
class CAUOutputDevice;
class CCoreAudioMixMap;

class CCoreAudioGraph
{
public:
  CCoreAudioGraph();
  ~CCoreAudioGraph();

  bool             Open(ICoreAudioSource *pSource, AEAudioFormat &format, AudioDeviceID deviceId,
                     bool allowMixing, AudioChannelLayoutTag layoutTag, float initVolume);
  bool             Close();
  bool             Start();
  bool             Stop();
  AudioChannelLayoutTag GetChannelLayoutTag(int layout);
  bool             SetInputSource(ICoreAudioSource *pSource);
  bool             SetCurrentVolume(Float32 vol);
  CAUOutputDevice* DestroyUnit(CAUOutputDevice *outputUnit);
  CAUOutputDevice* CreateUnit(AEAudioFormat &format);
  int              GetFreeBus();
  void             ReleaseBus(int busNumber);
  bool             IsBusFree(int busNumber);
  int              GetMixerChannelOffset(int busNumber);

private:
  AUGraph           m_audioGraph;

  CAUOutputDevice  *m_inputUnit;
  CAUOutputDevice  *m_audioUnit;
  CAUMatrixMixer   *m_mixerUnit;

  int               m_reservedBusNumber[MAX_CONNECTION_LIMIT];
  bool              m_initialized;
  AudioDeviceID     m_deviceId;
  bool              m_allowMixing;
  CCoreAudioMixMap *m_mixMap;

  typedef std::list<CAUOutputDevice*> AUUnitList;
  AUUnitList        m_auUnitList;
};

#endif
