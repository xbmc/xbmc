#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "system.h"
#include "threads/Thread.h"

#include "Interfaces/AEStream.h"
#include "Interfaces/AESound.h"
#include "AEFactory.h"

namespace PiAudioAE
{

class CPiAudioAE : public IAE
{
protected:
  friend class ::CAEFactory;
  CPiAudioAE();
  virtual ~CPiAudioAE();
  virtual bool  Initialize();

public:
  virtual bool   Suspend();
  virtual bool   Resume();
  virtual void   OnSettingsChange(const std::string& setting);

  virtual float GetVolume();
  virtual void  SetVolume(const float volume);
  virtual void  SetMute(const bool enabled);
  virtual bool  IsMuted();
  virtual void  SetSoundMode(const int mode) {}

  /* returns a new stream for data in the specified format */
  virtual IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  virtual IAEStream *FreeStream(IAEStream *stream);

  /* returns a new sound object */
  virtual IAESound *MakeSound(const std::string& file);
  virtual void      FreeSound(IAESound *sound);

  virtual void GarbageCollect() {};
  virtual void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  virtual std::string GetDefaultDevice(bool passthrough);
  virtual bool IsSettingVisible(const std::string &settingId);

  virtual bool SupportsRaw(AEDataFormat format);
  virtual bool SupportsDrain();

  virtual void OnLostDevice() {}
  virtual void OnResetDevice() {}

protected:
  void UpdateStreamSilence();
  // polled via the interface
  float m_aeVolume;
  bool m_aeMuted;
};
};
