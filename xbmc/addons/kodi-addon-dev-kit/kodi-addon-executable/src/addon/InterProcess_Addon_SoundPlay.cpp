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
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

  void* CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_GetHandle(const std::string& filename)
  {

  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_ReleaseHandle(void* hdl)
  {

  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_Play(void* hdl, bool waitUntilEnd)
  {

  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_Stop(void* hdl)
  {

  }

  bool CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_IsPlaying(void* hdl)
  {

  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_SetChannel(void* hdl, audio_channel channel)
  {

  }

  audio_channel CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_GetChannel(void* hdl)
  {

  }

  void CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_SetVolume(void* hdl, float volume)
  {

  }

  float CKODIAddon_InterProcess_Addon_SoundPlay::SoundPlay_GetVolume(void* hdl)
  {

  }

}; /* extern "C" */
