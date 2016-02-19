#pragma once
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_Addon_SoundPlay
  {
    void* SoundPlay_GetHandle(const std::string& filename);
    void SoundPlay_ReleaseHandle(void* hdl);
    void SoundPlay_Play(void* hdl, bool waitUntilEnd);
    void SoundPlay_Stop(void* hdl);
    bool SoundPlay_IsPlaying(void* hdl);
    void SoundPlay_SetChannel(void* hdl, audio_channel channel);
    audio_channel SoundPlay_GetChannel(void* hdl);
    void SoundPlay_SetVolume(void* hdl, float volume);
    float SoundPlay_GetVolume(void* hdl);
  };

}; /* extern "C" */
