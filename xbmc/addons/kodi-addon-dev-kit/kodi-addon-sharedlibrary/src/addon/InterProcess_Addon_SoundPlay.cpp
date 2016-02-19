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

#include "InterProcess_Addon_SoundPlay.h"
#include "InterProcess.h"

extern "C"
{

  void* CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_GetHandle(const std::string& filename)
  {
    return g_interProcess.m_Callbacks->Audio.soundplay_get_handle(g_interProcess.m_Handle, filename.c_str());
  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_ReleaseHandle(void* hdl)
  {
    g_interProcess.m_Callbacks->Audio.soundplay_release_handle(g_interProcess.m_Handle, hdl);
  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_Play(void* hdl, bool waitUntilEnd)
  {
    g_interProcess.m_Callbacks->Audio.soundplay_play(g_interProcess.m_Handle, hdl, waitUntilEnd);
  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_Stop(void* hdl)
  {
    g_interProcess.m_Callbacks->Audio.soundplay_stop(g_interProcess.m_Handle, hdl);
  }

  bool CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_IsPlaying(void* hdl)
  {
    return g_interProcess.m_Callbacks->Audio.soundplay_is_playing(g_interProcess.m_Handle, hdl);
  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_SetChannel(void* hdl, audio_channel channel)
  {
    g_interProcess.m_Callbacks->Audio.soundplay_set_channel(g_interProcess.m_Handle, hdl, channel);
  }

  audio_channel CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_GetChannel(void* hdl)
  {
    return g_interProcess.m_Callbacks->Audio.soundplay_get_channel(g_interProcess.m_Handle, hdl);
  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_SetVolume(void* hdl, float volume)
  {
    g_interProcess.m_Callbacks->Audio.soundplay_set_volume(g_interProcess.m_Handle, hdl, volume);
  }

  float CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_GetVolume(void* hdl)
  {
    return g_interProcess.m_Callbacks->Audio.soundplay_get_volume(g_interProcess.m_Handle, hdl);
  }

}; /* extern "C" */
