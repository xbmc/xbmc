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

#include "AEFactory.h"

#include "Engines/ActiveAE/ActiveAE.h"

#include "guilib/LocalizeStrings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

IAE* CAEFactory::AE = NULL;
static float  g_fVolume = 1.0f;
static bool   g_bMute = false;

IAE *CAEFactory::GetEngine()
{
  return AE;
}

bool CAEFactory::LoadEngine()
{
  /* can only load the engine once, XBMC restart is required to change it */
  if (AE)
    return false;

  AE = new ActiveAE::CActiveAE();

  if (AE && !AE->CanInit())
  {
    delete AE;
    AE = NULL;
  }

  return AE != NULL;
}

void CAEFactory::UnLoadEngine()
{
  if(AE)
  {
    AE->Shutdown();
    delete AE;
    AE = NULL;
  }
}

bool CAEFactory::StartEngine()
{
  if (!AE)
    return false;

  if (AE->Initialize())
    return true;

  delete AE;
  AE = NULL;
  return false;
}

bool CAEFactory::Suspend()
{
  if(AE)
    return AE->Suspend();

  return false;
}

bool CAEFactory::Resume()
{
  if(AE)
    return AE->Resume();

  return false;
}

bool CAEFactory::IsSuspended()
{
  if(AE)
    return AE->IsSuspended();

  /* No engine to process audio */
  return true;
}

/* engine wrapping */
IAESound *CAEFactory::MakeSound(const std::string &file)
{
  if(AE)
    return AE->MakeSound(file);
  
  return NULL;
}

void CAEFactory::FreeSound(IAESound *sound)
{
  if(AE)
    AE->FreeSound(sound);
}

void CAEFactory::SetSoundMode(const int mode)
{
  if(AE)
    AE->SetSoundMode(mode);
}

void CAEFactory::OnSettingsChange(std::string setting)
{
  if(AE)
    AE->OnSettingsChange(setting);
}

void CAEFactory::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
  if(AE)
    AE->EnumerateOutputDevices(devices, passthrough);
}

void CAEFactory::VerifyOutputDevice(std::string &device, bool passthrough)
{
  AEDeviceList devices;
  EnumerateOutputDevices(devices, passthrough);
  std::string firstDevice;

  for (AEDeviceList::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt)
  {
    /* remember the first device so we can default to it if required */
    if (firstDevice.empty())
      firstDevice = deviceIt->second;

    if (deviceIt->second == device)
      return;
    else if (deviceIt->first == device)
    {
      device = deviceIt->second;
      return;
    }
  }

  /* if the device wasnt found, set it to the first viable output */
  device = firstDevice;
}

std::string CAEFactory::GetDefaultDevice(bool passthrough)
{
  if(AE)
    return AE->GetDefaultDevice(passthrough);

  return "default";
}

bool CAEFactory::SupportsRaw(AEDataFormat format, int samplerate)
{
  // check if passthrough is enabled
  if (!CSettings::Get().GetBool("audiooutput.passthrough"))
    return false;

  // fixed config disabled passthrough
  if (CSettings::Get().GetInt("audiooutput.config") == AE_CONFIG_FIXED)
    return false;

  // check if the format is enabled in settings
  if (format == AE_FMT_AC3 && !CSettings::Get().GetBool("audiooutput.ac3passthrough"))
    return false;
  if (format == AE_FMT_DTS && !CSettings::Get().GetBool("audiooutput.dtspassthrough"))
    return false;
  if (format == AE_FMT_EAC3 && !CSettings::Get().GetBool("audiooutput.eac3passthrough"))
    return false;
  if (format == AE_FMT_TRUEHD && !CSettings::Get().GetBool("audiooutput.truehdpassthrough"))
    return false;
  if (format == AE_FMT_DTSHD && !CSettings::Get().GetBool("audiooutput.dtshdpassthrough"))
    return false;

  if(AE)
    return AE->SupportsRaw(format, samplerate);

  return false;
}

bool CAEFactory::SupportsSilenceTimeout()
{
  if(AE)
    return AE->SupportsSilenceTimeout();

  return false;
}

bool CAEFactory::HasStereoAudioChannelCount()
{
  if(AE)
    return AE->HasStereoAudioChannelCount();
  return false;
}

bool CAEFactory::HasHDAudioChannelCount()
{
  if(AE)
    return AE->HasHDAudioChannelCount();
  return false;
}

/**
  * Returns true if current AudioEngine supports at lest two basic quality levels
  * @return true if quality setting is supported, otherwise false
  */
bool CAEFactory::SupportsQualitySetting(void) 
{
  if (!AE)
    return false;

  return ((AE->SupportsQualityLevel(AE_QUALITY_LOW)? 1 : 0) + 
          (AE->SupportsQualityLevel(AE_QUALITY_MID)? 1 : 0) +
          (AE->SupportsQualityLevel(AE_QUALITY_HIGH)? 1 : 0)) >= 2; 
}
  
void CAEFactory::SetMute(const bool enabled)
{
  if(AE)
    AE->SetMute(enabled);

  g_bMute = enabled;
}

bool CAEFactory::IsMuted()
{
  if(AE)
    return AE->IsMuted();

  return g_bMute || (g_fVolume == 0.0f);
}

float CAEFactory::GetVolume()
{
  if(AE)
    return AE->GetVolume();

  return g_fVolume;
}

void CAEFactory::SetVolume(const float volume)
{
  if(AE)
    AE->SetVolume(volume);
  else
    g_fVolume = volume;
}

void CAEFactory::Shutdown()
{
  if(AE)
    AE->Shutdown();
}

IAEStream *CAEFactory::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, 
  unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options)
{
  if(AE)
    return AE->MakeStream(dataFormat, sampleRate, encodedSampleRate, channelLayout, options);

  return NULL;
}

IAEStream *CAEFactory::FreeStream(IAEStream *stream)
{
  if(AE)
    return AE->FreeStream(stream);

  return NULL;
}

void CAEFactory::GarbageCollect()
{
  if(AE)
    AE->GarbageCollect();
}

void CAEFactory::SettingOptionsAudioDevicesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, false);
}

void CAEFactory::SettingOptionsAudioDevicesPassthroughFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, true);
}

void CAEFactory::SettingOptionsAudioQualityLevelsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (!AE)
    return;

  if(AE->SupportsQualityLevel(AE_QUALITY_LOW))
    list.push_back(std::make_pair(g_localizeStrings.Get(13506), AE_QUALITY_LOW));
  if(AE->SupportsQualityLevel(AE_QUALITY_MID))
    list.push_back(std::make_pair(g_localizeStrings.Get(13507), AE_QUALITY_MID));
  if(AE->SupportsQualityLevel(AE_QUALITY_HIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13508), AE_QUALITY_HIGH));
  if(AE->SupportsQualityLevel(AE_QUALITY_REALLYHIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13509), AE_QUALITY_REALLYHIGH));
  if(AE->SupportsQualityLevel(AE_QUALITY_GPU))
    list.push_back(std::make_pair(g_localizeStrings.Get(38010), AE_QUALITY_GPU));
}

void CAEFactory::SettingOptionsAudioStreamsilenceFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (!AE)
    return;

  list.push_back(std::make_pair(g_localizeStrings.Get(20422), XbmcThreads::EndTime::InfiniteValue));
  list.push_back(std::make_pair(g_localizeStrings.Get(13551), 0));

  if (AE->SupportsSilenceTimeout())
  {
    list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13554).c_str(), 1), 1));
    for (int i = 2; i <= 10; i++)
    {
      list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13555).c_str(), i), i));
    }
  }
}

void CAEFactory::SettingOptionsAudioDevicesFillerGeneral(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, bool passthrough)
{
  current = ((const CSettingString*)setting)->GetValue();
  std::string firstDevice;

  bool foundValue = false;
  AEDeviceList sinkList;
  EnumerateOutputDevices(sinkList, passthrough);
  if (sinkList.size() == 0)
    list.push_back(std::make_pair("Error - no devices found", "error"));
  else
  {
    for (AEDeviceList::const_iterator sink = sinkList.begin(); sink != sinkList.end(); ++sink)
    {
      if (sink == sinkList.begin())
        firstDevice = sink->second;

      list.push_back(std::make_pair(sink->first, sink->second));

      if (StringUtils::EqualsNoCase(current, sink->second))
        foundValue = true;
    }
  }

  if (!foundValue)
    current = firstDevice;
}

void CAEFactory::RegisterAudioCallback(IAudioCallback* pCallback)
{
  if (AE)
    AE->RegisterAudioCallback(pCallback);
}

void CAEFactory::UnregisterAudioCallback()
{
  if (AE)
    AE->UnregisterAudioCallback();
}

bool CAEFactory::IsSettingVisible(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL || value.empty() || !AE)
    return false;

  return AE->IsSettingVisible(value);
}

void CAEFactory::KeepConfiguration(unsigned int millis)
{
  if (AE)
    AE->KeepConfiguration(millis);
}

void CAEFactory::DeviceChange()
{
  if (AE)
    AE->DeviceChange();
}
