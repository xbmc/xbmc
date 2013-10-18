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
#include "Utils/AEUtil.h"

#if defined(TARGET_DARWIN)
  #include "Engines/CoreAudio/CoreAudioAE.h"
  #include "settings/SettingsManager.h"
#else
  #include "Engines/SoftAE/SoftAE.h"
  #include "Engines/ActiveAE/ActiveAE.h"
#endif

#if defined(HAS_PULSEAUDIO)
  #include "Engines/PulseAE/PulseAE.h"
#endif

#if defined(TARGET_RASPBERRY_PI)
  #include "Engines/PiAudio/PiAudioAE.h"
#endif

#include "guilib/LocalizeStrings.h"
#include "settings/Setting.h"
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
  bool loaded = false;

#if defined(TARGET_RASPBERRY_PI)
  return CAEFactory::LoadEngine(AE_ENGINE_PIAUDIO);
#elif defined(TARGET_DARWIN)
  return CAEFactory::LoadEngine(AE_ENGINE_COREAUDIO);
#endif

  std::string engine;
  if (getenv("AE_ENGINE"))
  {
    engine = (std::string)getenv("AE_ENGINE");
    std::transform(engine.begin(), engine.end(), engine.begin(), ::toupper);

    #if defined(HAS_PULSEAUDIO)
    if (!loaded && engine == "PULSE")
      loaded = CAEFactory::LoadEngine(AE_ENGINE_PULSE);
    #endif
    
    if (!loaded && engine == "SOFT" )
      loaded = CAEFactory::LoadEngine(AE_ENGINE_SOFT);

    if (!loaded && engine == "ACTIVE")
      loaded = CAEFactory::LoadEngine(AE_ENGINE_ACTIVE);
  }

#if defined(HAS_PULSEAUDIO)
  if (!loaded)
    loaded = CAEFactory::LoadEngine(AE_ENGINE_PULSE);
#endif

  if (!loaded)
    loaded = CAEFactory::LoadEngine(AE_ENGINE_ACTIVE);

  return loaded;
}

bool CAEFactory::LoadEngine(enum AEEngine engine)
{
  /* can only load the engine once, XBMC restart is required to change it */
  if (AE)
    return false;

  switch(engine)
  {
    case AE_ENGINE_NULL     :
#if defined(TARGET_DARWIN)
    case AE_ENGINE_COREAUDIO: AE = new CCoreAudioAE(); break;
#else
    case AE_ENGINE_SOFT     : AE = new CSoftAE(); break;
    case AE_ENGINE_ACTIVE   : AE = new ActiveAE::CActiveAE(); break;
#endif
#if defined(HAS_PULSEAUDIO)
    case AE_ENGINE_PULSE    : AE = new CPulseAE(); break;
#endif
#if defined(TARGET_RASPBERRY_PI)
    case AE_ENGINE_PIAUDIO  : AE = new PiAudioAE::CPiAudioAE(); break;
#endif
    default:
      return false;
  }

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

bool CAEFactory::SupportsRaw(AEDataFormat format)
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
  if (format == AE_FMT_AAC && !CSettings::Get().GetBool("audiooutput.passthroughaac"))
    return false;
  if (format == AE_FMT_TRUEHD && !CSettings::Get().GetBool("audiooutput.truehdpassthrough"))
    return false;
  if (format == AE_FMT_DTSHD && !CSettings::Get().GetBool("audiooutput.dtshdpassthrough"))
    return false;

  if(AE)
    return AE->SupportsRaw(format);

  return false;
}

bool CAEFactory::SupportsDrain()
{
  if(AE)
    return AE->SupportsDrain();

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

void CAEFactory::SettingOptionsAudioDevicesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, false);
}

void CAEFactory::SettingOptionsAudioDevicesPassthroughFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, true);
}

void CAEFactory::SettingOptionsAudioQualityLevelsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
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
}

void CAEFactory::SettingOptionsAudioDevicesFillerGeneral(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, bool passthrough)
{
  current = ((const CSettingString*)setting)->GetValue();
  std::string firstDevice;

  bool foundValue = false;
  AEDeviceList sinkList;
  EnumerateOutputDevices(sinkList, passthrough);
#if !defined(TARGET_DARWIN)
  if (sinkList.size() == 0)
    list.push_back(std::make_pair("Error - no devices found", "error"));
  else
  {
#endif
    for (AEDeviceList::const_iterator sink = sinkList.begin(); sink != sinkList.end(); ++sink)
    {
      if (sink == sinkList.begin())
        firstDevice = sink->second;

#if defined(TARGET_DARWIN)
      list.push_back(std::make_pair(sink->first, sink->first));
#else
      list.push_back(std::make_pair(sink->first, sink->second));
#endif

      if (StringUtils::EqualsNoCase(current, sink->second))
        foundValue = true;
    }
#if !defined(TARGET_DARWIN)
  }
#endif

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

bool CAEFactory::IsSettingVisible(const std::string &condition, const std::string &value, const std::string &settingId)
{
  if (settingId.empty() || value.empty() || !AE)
    return false;

  return AE->IsSettingVisible(value);
}
