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
#include KITINCLUDE(ADDON_API_LEVEL, addon/SoundPlay.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{

  CSoundPlay::CSoundPlay(const std::string& filename)
   : m_PlayHandle(nullptr)
  {
    m_PlayHandle = g_interProcess.m_Callbacks->Audio.soundplay_get_handle(g_interProcess.m_Handle, filename.c_str());
    if (!m_PlayHandle)
      fprintf(stderr, "libKODI_addon-ERROR: CSoundPlay can't get callback table from KODI !!!\n");
  }

  CSoundPlay::~CSoundPlay()
  {
    if (m_PlayHandle)
      g_interProcess.m_Callbacks->Audio.soundplay_release_handle(g_interProcess.m_Handle, m_PlayHandle);
  }

  void CSoundPlay::Play()
  {
    if (m_PlayHandle)
      g_interProcess.m_Callbacks->Audio.soundplay_play(g_interProcess.m_Handle, m_PlayHandle);
  }

  void CSoundPlay::Stop()
  {
    if (m_PlayHandle)
      g_interProcess.m_Callbacks->Audio.soundplay_stop(g_interProcess.m_Handle, m_PlayHandle);
  }

  void CSoundPlay::SetChannel(audio_channel channel)
  {
    if (m_PlayHandle)
      g_interProcess.m_Callbacks->Audio.soundplay_set_channel(g_interProcess.m_Handle, m_PlayHandle, channel);
  }

  audio_channel CSoundPlay::GetChannel()
  {
    if (!m_PlayHandle)
      return AUDIO_CH_INVALID;
    return (audio_channel)g_interProcess.m_Callbacks->Audio.soundplay_get_channel(g_interProcess.m_Handle, m_PlayHandle);
  }

  void CSoundPlay::SetVolume(float volume)
  {
    if (m_PlayHandle)
      g_interProcess.m_Callbacks->Audio.soundplay_set_volume(g_interProcess.m_Handle, m_PlayHandle, volume);
  }

  float CSoundPlay::GetVolume()
  {
    if (!m_PlayHandle)
      return 0.0f;
    return g_interProcess.m_Callbacks->Audio.soundplay_get_volume(g_interProcess.m_Handle, m_PlayHandle);
  }

} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()
