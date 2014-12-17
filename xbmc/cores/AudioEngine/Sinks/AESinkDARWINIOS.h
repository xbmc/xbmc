#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "cores/AudioEngine/Interfaces/AESink.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#define DO_440HZ_TONE_TEST 0

#if DO_440HZ_TONE_TEST
typedef struct {
  float currentPhase;
  float phaseIncrement;
} SineWaveGenerator;
#endif

class AERingBuffer;
class CAAudioUnitSink;

class CAESinkDARWINIOS : public IAESink
{
public:
  virtual const char *GetName() { return "DARWINIOS"; }

  CAESinkDARWINIOS();
  virtual ~CAESinkDARWINIOS();

  virtual bool Initialize(AEAudioFormat &format, std::string &device);
  virtual void Deinitialize();

  virtual void         GetDelay(AEDelayStatus& status);
  virtual double       GetCacheTotal   ();
  virtual unsigned int AddPackets      (uint8_t **data, unsigned int frames, unsigned int offset);
  virtual void         Drain           ();
  virtual bool         HasVolume       ();
  static void          EnumerateDevicesEx(AEDeviceInfoList &list, bool force = false);

private:
  static AEDeviceInfoList m_devices;
  CAEDeviceInfo      m_info;
  AEAudioFormat      m_format;

  CAAudioUnitSink   *m_audioSink;
#if DO_440HZ_TONE_TEST
  SineWaveGenerator  m_SineWaveGenerator;
#endif
};
