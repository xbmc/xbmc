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

#include "PiAudioAE.h"

using namespace PiAudioAE;
#include "Utils/AEUtil.h"

#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "windowing/WindowingFactory.h"

#if defined(TARGET_RASPBERRY_PI)
#include "linux/RBP.h"
#endif

CPiAudioAE::CPiAudioAE()
{
}

CPiAudioAE::~CPiAudioAE()
{
}

bool CPiAudioAE::Initialize()
{
  UpdateStreamSilence();
  return true;
}

void CPiAudioAE::UpdateStreamSilence()
{
#if defined(TARGET_RASPBERRY_PI)
  bool enable = CSettings::Get().GetString("audiooutput.audiodevice") == "HDMI" &&
                CSettings::Get().GetBool("audiooutput.streamsilence");
  char response[80] = "";
  char command[80] = "";
  sprintf(command, "force_audio hdmi %d", enable);
  vc_gencmd(response, sizeof response, command);
#endif
}

bool CPiAudioAE::Suspend()
{
  return true;
}

bool CPiAudioAE::Resume()
{
  return true;
}

float CPiAudioAE::GetVolume()
{
  return m_aeVolume;
}

void CPiAudioAE::SetVolume(const float volume)
{
  m_aeVolume = std::max( 0.0f, std::min(1.0f, volume));
}

void CPiAudioAE::SetMute(const bool enabled)
{
  m_aeMuted = enabled;
}

bool CPiAudioAE::IsMuted()
{
  return m_aeMuted;
}

IAEStream *CPiAudioAE::MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options)
{
  return NULL;
}

IAEStream *CPiAudioAE::FreeStream(IAEStream *stream)
{
  return NULL;
}

IAESound *CPiAudioAE::MakeSound(const std::string& file)
{
  return NULL;
}

void CPiAudioAE::FreeSound(IAESound *sound)
{
}

bool CPiAudioAE::SupportsRaw(AEDataFormat format)
{
  bool supported = false;
#if defined(TARGET_RASPBERRY_PI)
  if (CSettings::Get().GetString("audiooutput.audiodevice") == "HDMI")
  {
    if (!CSettings::Get().GetBool("audiooutput.dualaudio"))
    {
      DllBcmHost m_DllBcmHost;
      m_DllBcmHost.Load();
      if (format == AE_FMT_AC3 && CSettings::Get().GetBool("audiooutput.ac3passthrough") &&
          m_DllBcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_eAC3, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) == 0)
        supported = true;
      if (format == AE_FMT_DTS && CSettings::Get().GetBool("audiooutput.dtspassthrough") &&
          m_DllBcmHost.vc_tv_hdmi_audio_supported(EDID_AudioFormat_eDTS, 2, EDID_AudioSampleRate_e44KHz, EDID_AudioSampleSize_16bit ) == 0)
        supported = true;
      m_DllBcmHost.Unload();
    }
  }
#endif
  return supported;
}

bool CPiAudioAE::SupportsDrain()
{
  return true;
}

void CPiAudioAE::OnSettingsChange(const std::string& setting)
{
  if (setting == "audiooutput.streamsilence" || setting == "audiooutput.audiodevice")
    UpdateStreamSilence();
}

void CPiAudioAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
   if (!passthrough)
   {
     devices.push_back(AEDevice("Analogue", "Analogue"));
     devices.push_back(AEDevice("HDMI", "HDMI"));
   }
}

std::string CPiAudioAE::GetDefaultDevice(bool passthrough)
{
  return "HDMI";
}

bool CPiAudioAE::IsSettingVisible(const std::string &settingId)
{
  if (settingId == "audiooutput.samplerate")
    return true;

  if (CSettings::Get().GetString("audiooutput.audiodevice") == "HDMI")
  {
    if (settingId == "audiooutput.passthrough")
      return true;
    if (settingId == "audiooutput.dtspassthrough")
      return true;
    if (settingId == "audiooutput.ac3passthrough")
      return true;
    if (settingId == "audiooutput.channels")
      return true;
    if (settingId == "audiooutput.dualaudio")
      return true;
    if (settingId == "audiooutput.streamsilence")
      return true;
  }
  return false;
}
