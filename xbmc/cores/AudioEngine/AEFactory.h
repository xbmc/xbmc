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

#include <vector>

#include "Interfaces/AE.h"
#include "threads/Thread.h"

class CSetting;

enum AEEngine
{
  AE_ENGINE_NULL,
  AE_ENGINE_COREAUDIO,
  AE_ENGINE_PULSE,
  AE_ENGINE_ACTIVE,
  AE_ENGINE_PIAUDIO
};

class CAEFactory
{
public:
  static IAE *GetEngine();
  static bool LoadEngine();
  static void UnLoadEngine();
  static bool StartEngine();
  static bool Suspend(); /** Suspends output and de-initializes output sink - used for external players or power saving */
  static bool Resume(); /** Resumes output after Suspend - re-initializes sink */
  static bool IsSuspended(); /** Returns true if output has been suspended */
  /* wrap engine interface */
  static IAESound *MakeSound(const std::string &file);
  static void FreeSound(IAESound *sound);
  static void SetSoundMode(const int mode);
  static void OnSettingsChange(std::string setting);
  static void EnumerateOutputDevices(AEDeviceList &devices, bool passthrough);
  static void VerifyOutputDevice(std::string &device, bool passthrough);
  static std::string GetDefaultDevice(bool passthrough);
  static bool SupportsRaw(AEDataFormat format);
  static bool SupportsDrain();

  /**
   * Returns true if current AudioEngine supports at lest two basic quality levels
   * @return true if quality setting is supported, otherwise false
   */
  static bool SupportsQualitySetting(void);
  static void SetMute(const bool enabled);
  static bool IsMuted();
  static float GetVolume();
  static void SetVolume(const float volume);
  static void Shutdown();
  static IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, 
    unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  static IAEStream *FreeStream(IAEStream *stream);
  static void GarbageCollect();

  static void SettingOptionsAudioDevicesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current);
  static void SettingOptionsAudioDevicesPassthroughFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current);
  static void SettingOptionsAudioQualityLevelsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsAudioStreamsilenceFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static bool IsSettingVisible(const std::string &condition, const std::string &value, const std::string &settingId);

  static void RegisterAudioCallback(IAudioCallback* pCallback);
  static void UnregisterAudioCallback();

private:
  static bool LoadEngine(enum AEEngine engine);
  static IAE *AE;

  static void SettingOptionsAudioDevicesFillerGeneral(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, bool passthrough);
};

