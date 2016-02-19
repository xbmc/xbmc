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

#include "AddonCB_Audio.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSPMode.h"
#include "cores/AudioEngine/Interfaces/AESound.h"

using namespace ActiveAE;
using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

CAddOnAudio::CAddOnAudio()
{

}

void CAddOnAudio::Init(V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->Audio.soundplay_get_handle       = CAddOnAudio::soundplay_get_handle;
  callbacks->Audio.soundplay_release_handle   = CAddOnAudio::soundplay_release_handle;
  callbacks->Audio.soundplay_play             = CAddOnAudio::soundplay_play;
  callbacks->Audio.soundplay_stop             = CAddOnAudio::soundplay_stop;
  callbacks->Audio.soundplay_is_playing       = CAddOnAudio::soundplay_is_playing;
  callbacks->Audio.soundplay_set_channel      = CAddOnAudio::soundplay_set_channel;
  callbacks->Audio.soundplay_get_channel      = CAddOnAudio::soundplay_get_channel;
  callbacks->Audio.soundplay_set_volume       = CAddOnAudio::soundplay_set_volume;
  callbacks->Audio.soundplay_get_volume       = CAddOnAudio::soundplay_get_volume;
}

KODI_HANDLE CAddOnAudio::soundplay_get_handle(
      void*                     hdl,
      const char*               filename)
{
  try
  {
    if (!hdl || !filename)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', filename='%p')",
                                        __FUNCTION__, hdl, filename);

    IAESound *sound = CAEFactory::MakeSound(filename);
    if (!sound)
    {
      CLog::Log(LOGERROR, "CAddOnAudio - %s - failed to make sound play data", __FUNCTION__);
      return nullptr;
    }

    return sound;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnAudio::soundplay_release_handle(
      void*                     hdl,
      KODI_HANDLE               sndHandle)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);

    CAEFactory::FreeSound(static_cast<IAESound*>(sndHandle));
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnAudio::soundplay_play(
      void*                     hdl,
      KODI_HANDLE               sndHandle,
      bool                      waitUntilEnd)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);
    static_cast<IAESound*>(sndHandle)->Play();
    if (waitUntilEnd)
    {
//      Sleep(50);
      while (static_cast<IAESound*>(sndHandle)->IsPlaying())
      {
        Sleep(50);
      }
    }
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnAudio::soundplay_stop(
      void*                     hdl,
      KODI_HANDLE               sndHandle)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);
    static_cast<IAESound*>(sndHandle)->Stop();
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnAudio::soundplay_is_playing(
      void*                     hdl,
      KODI_HANDLE               sndHandle)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);
    return static_cast<IAESound*>(sndHandle)->IsPlaying();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnAudio::soundplay_set_channel(
      void*                     hdl,
      KODI_HANDLE               sndHandle,
      audio_channel             channel)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);

    static_cast<IAESound*>(sndHandle)->SetChannel(CAddOnAudio::GetKODIChannel(channel));
  }
  HANDLE_ADDON_EXCEPTION
}

audio_channel CAddOnAudio::soundplay_get_channel(
      void*                     hdl,
      KODI_HANDLE               sndHandle)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);

    return CAddOnAudio::GetAddonChannel(static_cast<IAESound*>(sndHandle)->GetChannel());
  }
  HANDLE_ADDON_EXCEPTION

  return AUDIO_CH_INVALID;
}

void CAddOnAudio::soundplay_set_volume(
      void*                     hdl,
      KODI_HANDLE               sndHandle,
      float                     volume)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);
    static_cast<IAESound*>(sndHandle)->SetVolume(volume);
  }
  HANDLE_ADDON_EXCEPTION
}

float CAddOnAudio::soundplay_get_volume(
      void*                     hdl,
      KODI_HANDLE               sndHandle)
{
  try
  {
    if (!hdl || !sndHandle)
      throw ADDON::WrongValueException("CAddOnAudio - %s - invalid data (handle='%p', sndHandle='%p')",
                                        __FUNCTION__, hdl, sndHandle);
    return static_cast<IAESound*>(sndHandle)->GetVolume();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0f;
}

enum AEChannel CAddOnAudio::GetKODIChannel(audio_channel channel)
{
  switch (channel)
  {
  case AUDIO_CH_FL:   return AE_CH_FL;
  case AUDIO_CH_FR:   return AE_CH_FR;
  case AUDIO_CH_FC:   return AE_CH_FC;
  case AUDIO_CH_LFE:  return AE_CH_LFE;
  case AUDIO_CH_BL:   return AE_CH_BL;
  case AUDIO_CH_BR:   return AE_CH_BR;
  case AUDIO_CH_FLOC: return AE_CH_FLOC;
  case AUDIO_CH_FROC: return AE_CH_FROC;
  case AUDIO_CH_BC:   return AE_CH_BC;
  case AUDIO_CH_SL:   return AE_CH_SL;
  case AUDIO_CH_SR:   return AE_CH_SR;
  case AUDIO_CH_TC:   return AE_CH_TC;
  case AUDIO_CH_TFL:  return AE_CH_TFL;
  case AUDIO_CH_TFC:  return AE_CH_TFC;
  case AUDIO_CH_TFR:  return AE_CH_TFR;
  case AUDIO_CH_TBL:  return AE_CH_TBL;
  case AUDIO_CH_TBC:  return AE_CH_TBC;
  case AUDIO_CH_TBR:  return AE_CH_TBR;
  case AUDIO_CH_BLOC: return AE_CH_BLOC;
  case AUDIO_CH_BROC: return AE_CH_BROC;
  default:
    return AE_CH_NULL;
  }
}

audio_channel CAddOnAudio::GetAddonChannel(enum AEChannel channel)
{
  switch (channel)
  {
  case AE_CH_FL:   return AUDIO_CH_FL;
  case AE_CH_FR:   return AUDIO_CH_FR;
  case AE_CH_FC:   return AUDIO_CH_FC;
  case AE_CH_LFE:  return AUDIO_CH_LFE;
  case AE_CH_BL:   return AUDIO_CH_BL;
  case AE_CH_BR:   return AUDIO_CH_BR;
  case AE_CH_FLOC: return AUDIO_CH_FLOC;
  case AE_CH_FROC: return AUDIO_CH_FROC;
  case AE_CH_BC:   return AUDIO_CH_BC;
  case AE_CH_SL:   return AUDIO_CH_SL;
  case AE_CH_SR:   return AUDIO_CH_SR;
  case AE_CH_TC:   return AUDIO_CH_TC;
  case AE_CH_TFL:  return AUDIO_CH_TFL;
  case AE_CH_TFC:  return AUDIO_CH_TFC;
  case AE_CH_TFR:  return AUDIO_CH_TFR;
  case AE_CH_TBL:  return AUDIO_CH_TBL;
  case AE_CH_TBC:  return AUDIO_CH_TBC;
  case AE_CH_TBR:  return AUDIO_CH_TBR;
  case AE_CH_BLOC: return AUDIO_CH_BLOC;
  case AE_CH_BROC: return AUDIO_CH_BROC;
  default:
    return AUDIO_CH_INVALID;
  }
}

}; /* extern "C" */
}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
