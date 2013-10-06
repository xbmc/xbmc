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
  bool enable = CSettings::Get().GetBool("audiooutput.streamsilence");
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

bool CPiAudioAE::SupportsRaw()
{
  return true;
}

bool CPiAudioAE::SupportsDrain()
{
  return true;
}

void CPiAudioAE::OnSettingsChange(const std::string& setting)
{
  if (setting == "audiooutput.streamsilence")
    UpdateStreamSilence();
}

void CPiAudioAE::EnumerateOutputDevices(AEDeviceList &devices, bool passthrough)
{
}

