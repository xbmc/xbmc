/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess.h"
#include "kodi/api2/addon/SoundPlay.hpp"

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{

  CSoundPlay::CSoundPlay(const std::string& filename)
   : m_PlayHandle(nullptr)
  {
    m_PlayHandle = g_interProcess.SoundPlay_GetHandle(filename);
    if (!m_PlayHandle)
      fprintf(stderr, "libKODI_addon-ERROR: CSoundPlay can't get callback table from KODI !!!\n");
  }

  CSoundPlay::~CSoundPlay()
  {
    if (m_PlayHandle)
      g_interProcess.SoundPlay_ReleaseHandle(m_PlayHandle);
  }

  void CSoundPlay::Play(bool waitUntilEnd)
  {
    if (m_PlayHandle)
      g_interProcess.SoundPlay_Play(m_PlayHandle, waitUntilEnd);
  }

  void CSoundPlay::Stop()
  {
    if (m_PlayHandle)
      g_interProcess.SoundPlay_Stop(m_PlayHandle);
  }

  bool CSoundPlay::IsPlaying()
  {
    if (!m_PlayHandle)
      return false;
    return g_interProcess.SoundPlay_IsPlaying(m_PlayHandle);
  }

  void CSoundPlay::SetChannel(audio_channel channel)
  {
    if (m_PlayHandle)
      g_interProcess.SoundPlay_SetChannel(m_PlayHandle, channel);
  }

  audio_channel CSoundPlay::GetChannel()
  {
    if (!m_PlayHandle)
      return AUDIO_CH_INVALID;
    return g_interProcess.SoundPlay_GetChannel(m_PlayHandle);
  }

  void CSoundPlay::SetVolume(float volume)
  {
    if (m_PlayHandle)
      g_interProcess.SoundPlay_SetVolume(m_PlayHandle, volume);
  }

  float CSoundPlay::GetVolume()
  {
    if (!m_PlayHandle)
      return 0.0f;
    return g_interProcess.SoundPlay_GetVolume(m_PlayHandle);
  }

}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
