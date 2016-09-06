/*
 *      Copyright (C) 2013-2014 Team KODI
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_adsp.h"
#include "addons/binary/interfaces/api1/AudioDSP/AddonCallbacksAudioDSP.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;
using namespace V1::KodiAPI::AudioDSP;

extern "C"
{

DLLEXPORT void* ADSP_register_me(void *hdl)
{
  CB_ADSPLib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "libKODI_adsp-ERROR: ADSPLib_register_me is called with NULL handle !!!\n");
  else
  {
    cb = (CB_ADSPLib*)((AddonCB*)hdl)->ADSPLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "libKODI_adsp-ERROR: ADSPLib_register_me can't get callback table from KODI !!!\n");
  }
  return cb;
}

DLLEXPORT void ADSP_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->ADSPLib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_ADSPLib*)cb);
}

DLLEXPORT void ADSP_add_menu_hook(void *hdl, void* cb, AE_DSP_MENUHOOK *hook)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->AddMenuHook(((AddonCB*)hdl)->addonData, hook);
}

DLLEXPORT void ADSP_remove_menu_hook(void *hdl, void* cb, AE_DSP_MENUHOOK *hook)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->RemoveMenuHook(((AddonCB*)hdl)->addonData, hook);
}

DLLEXPORT void ADSP_register_mode(void *hdl, void* cb, AE_DSP_MODES::AE_DSP_MODE *mode)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->RegisterMode(((AddonCB*)hdl)->addonData, mode);
}

DLLEXPORT void ADSP_unregister_mode(void *hdl, void* cb, AE_DSP_MODES::AE_DSP_MODE *mode)
{
  if (cb == NULL)
    return;

  ((CB_ADSPLib*)cb)->UnregisterMode(((AddonCB*)hdl)->addonData, mode);
}

///-------------------------------------
/// CAddonSoundPlay

DLLEXPORT CAddonSoundPlay* ADSP_get_sound_play(void *hdl, void *cb, const char *filename)
{
  return new CAddonSoundPlay(hdl, cb, filename);
}

DLLEXPORT void ADSP_release_sound_play(CAddonSoundPlay* p)
{
  delete p;
}

CAddonSoundPlay::CAddonSoundPlay(void *hdl, void *cb, const char *filename)
 : m_Filename(filename),
   m_Handle(hdl),
   m_cb(cb)
{
  m_PlayHandle = NULL;
  if (!hdl || !cb)
    fprintf(stderr, "libKODI_adsp-ERROR: ADSP_get_sound_play is called with NULL handle !!!\n");
  else
  {
    m_PlayHandle = ((CB_ADSPLib*)m_cb)->SoundPlay_GetHandle(((AddonCB*)m_Handle)->addonData, m_Filename.c_str());
    if (!m_PlayHandle)
      fprintf(stderr, "libKODI_adsp-ERROR: ADSP_get_sound_play can't get callback table from KODI !!!\n");
  }
}

CAddonSoundPlay::~CAddonSoundPlay()
{
  if (m_PlayHandle)
    ((CB_ADSPLib*)m_cb)->SoundPlay_ReleaseHandle(((AddonCB*)m_Handle)->addonData, m_PlayHandle);
}

void CAddonSoundPlay::Play()
{
  if (m_PlayHandle)
    ((CB_ADSPLib*)m_cb)->SoundPlay_Play(((AddonCB*)m_Handle)->addonData, m_PlayHandle);
}

void CAddonSoundPlay::Stop()
{
  if (m_PlayHandle)
    ((CB_ADSPLib*)m_cb)->SoundPlay_Stop(((AddonCB*)m_Handle)->addonData, m_PlayHandle);
}

bool CAddonSoundPlay::IsPlaying()
{
  if (!m_PlayHandle)
    return false;

  return ((CB_ADSPLib*)m_cb)->SoundPlay_IsPlaying(((AddonCB*)m_Handle)->addonData, m_PlayHandle);
}

void CAddonSoundPlay::SetChannel(AE_DSP_CHANNEL channel)
{
  if (m_PlayHandle)
    ((CB_ADSPLib*)m_cb)->SoundPlay_SetChannel(((AddonCB*)m_Handle)->addonData, m_PlayHandle, channel);
}

AE_DSP_CHANNEL CAddonSoundPlay::GetChannel()
{
  if (!m_PlayHandle)
    return AE_DSP_CH_INVALID;
  return ((CB_ADSPLib*)m_cb)->SoundPlay_GetChannel(((AddonCB*)m_Handle)->addonData, m_PlayHandle);
}

void CAddonSoundPlay::SetVolume(float volume)
{
  if (m_PlayHandle)
    ((CB_ADSPLib*)m_cb)->SoundPlay_SetVolume(((AddonCB*)m_Handle)->addonData, m_PlayHandle, volume);
}

float CAddonSoundPlay::GetVolume()
{
  if (!m_PlayHandle)
    return 0.0f;

  return ((CB_ADSPLib*)m_cb)->SoundPlay_GetVolume(((AddonCB*)m_Handle)->addonData, m_PlayHandle);
}

};
