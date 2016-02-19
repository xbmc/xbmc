#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include "PlatformDefs.h" // for __stat64, ssize_t
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"
#include "cores/AudioEngine/Utils/AEChannelData.h"

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

  struct CB_AddOnLib;

  class CAddOnAudio
  {
  public:
    CAddOnAudio();

    static void Init(V2::KodiAPI::CB_AddOnLib *callbacks);

    static KODI_HANDLE soundplay_get_handle(
        void*                     hdl,
        const char*               filename);

    static void soundplay_release_handle(
        void*                     hdl,
        KODI_HANDLE               sndHandle);

    static void soundplay_play(
        void*                     hdl,
        KODI_HANDLE               sndHandle,
        bool                      waitUntilEnd);

    static void soundplay_stop(
        void*                     hdl,
        KODI_HANDLE               sndHandle);

    static bool soundplay_is_playing(
        void*                     hdl,
        KODI_HANDLE               sndHandle);

    static void soundplay_set_channel(
        void*                     hdl,
        KODI_HANDLE               sndHandle,
        audio_channel           channel);

    static audio_channel soundplay_get_channel(
        void*                     hdl,
        KODI_HANDLE               sndHandle);

    static void soundplay_set_volume(
        void*                     hdl,
        KODI_HANDLE               sndHandle,
        float                     volume);

    static float soundplay_get_volume(
        void*                     hdl,
        KODI_HANDLE               sndHandle);

  private:
    static enum AEChannel GetKODIChannel(audio_channel channel);
    static audio_channel GetAddonChannel(enum AEChannel channel);
  };

}; /* extern "C" */
}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
