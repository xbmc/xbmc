#pragma once
/*
 *      Copyright (C) 2005-2014 Team KODI
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "kodi_adsp_types.h"
#include "libXBMC_addon.h"

class CAddonSoundPlay
{
public:
  CAddonSoundPlay(AddonCB* hdl, AddonInstance_AudioDSP* cb, const char* filename)
    : m_Filename(filename),
      m_Handle(hdl),
      m_cb(cb)
  {
    m_PlayHandle = nullptr;
    if (!hdl || !cb)
      fprintf(stderr, "libKODI_adsp-ERROR: ADSP_get_sound_play is called with NULL handle !!!\n");
    else
    {
      m_PlayHandle = m_cb->toKodi.SoundPlay_GetHandle(m_cb->toKodi.kodiInstance, m_Filename.c_str());
      if (!m_PlayHandle)
        fprintf(stderr, "libKODI_adsp-ERROR: ADSP_get_sound_play can't get callback table from KODI !!!\n");
    }
  }

  ~CAddonSoundPlay()
  {
    if (m_PlayHandle)
      m_cb->toKodi.SoundPlay_ReleaseHandle(m_cb->toKodi.kodiInstance, m_PlayHandle);
  }

  /*! play the sound this object represents */
  void Play()
  {
    if (m_PlayHandle)
      m_cb->toKodi.SoundPlay_Play(m_cb->toKodi.kodiInstance, m_PlayHandle);
  }


  /*! stop playing the sound this object represents */
  void Stop()
  {
    if (m_PlayHandle)
      m_cb->toKodi.SoundPlay_Stop(m_cb->toKodi.kodiInstance, m_PlayHandle);
  }

  /*! return true if the sound is currently playing */
  bool IsPlaying()
  {
    if (!m_PlayHandle)
      return false;

    return m_cb->toKodi.SoundPlay_IsPlaying(m_cb->toKodi.kodiInstance, m_PlayHandle);
  }

  /*! set the playback channel position of this sound, AE_DSP_CH_INVALID for all */
  void SetChannel(AE_DSP_CHANNEL channel)
  {
    if (m_PlayHandle)
      m_cb->toKodi.SoundPlay_SetChannel(m_cb->toKodi.kodiInstance, m_PlayHandle, channel);
  }

  /*! get the current playback volume of this sound, AE_DSP_CH_INVALID for all */
  AE_DSP_CHANNEL GetChannel()
  {
    if (!m_PlayHandle)
      return AE_DSP_CH_INVALID;
    return m_cb->toKodi.SoundPlay_GetChannel(m_cb->toKodi.kodiInstance, m_PlayHandle);
  }

  /*! set the playback volume of this sound */
  void SetVolume(float volume)
  {
    if (m_PlayHandle)
      m_cb->toKodi.SoundPlay_SetVolume(m_cb->toKodi.kodiInstance, m_PlayHandle, volume);
  }

  /*! get the current playback volume of this sound */
  float GetVolume()
  {
    if (!m_PlayHandle)
      return 0.0f;

    return m_cb->toKodi.SoundPlay_GetVolume(m_cb->toKodi.kodiInstance, m_PlayHandle);
  }

private:
  std::string m_Filename;
  AddonCB* m_Handle;
  AddonInstance_AudioDSP *m_cb;
  ADSPHANDLE  m_PlayHandle;
};

class CHelper_libKODI_adsp
{
public:
  CHelper_libKODI_adsp(void)
  {
    m_Handle    = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libKODI_adsp(void)
  {
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = static_cast<AddonCB*>(handle);
    if (m_Handle)
      m_Callbacks = (AddonInstance_AudioDSP*)m_Handle->ADSPLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libKODI_adsp-ERROR: ADSLib_RegisterMe can't get callback table from Kodi !!!\n");

    return m_Callbacks != nullptr;
  }

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param hook The hook to add
   */
  void AddMenuHook(AE_DSP_MENUHOOK* hook)
  {
    return m_Callbacks->toKodi.AddMenuHook(m_Callbacks->toKodi.kodiInstance, hook);
  }

  /*!
   * @brief Remove a menu hook for the context menu for this add-on
   * @param hook The hook to remove
   */
  void RemoveMenuHook(AE_DSP_MENUHOOK* hook)
  {
    return m_Callbacks->toKodi.RemoveMenuHook(m_Callbacks->toKodi.kodiInstance, hook);
  }

  /*!
   * @brief Add or replace master mode information inside audio dsp database.
   * Becomes identifier written inside mode to iModeID if it was 0 (undefined)
   * @param mode The master mode to add or update inside database
   */
  void RegisterMode(AE_DSP_MODES::AE_DSP_MODE* mode)
  {
    return m_Callbacks->toKodi.RegisterMode(m_Callbacks->toKodi.kodiInstance, mode);
  }

  /*!
   * @brief Remove a master mode from audio dsp database
   * @param mode The Mode to remove
   */
  void UnregisterMode(AE_DSP_MODES::AE_DSP_MODE* mode)
  {
    return m_Callbacks->toKodi.UnregisterMode(m_Callbacks->toKodi.kodiInstance, mode);
  }

  /*!
   * @brief Open a sound playing class
   * @param filename The wav filename to open
   */
  CAddonSoundPlay* GetSoundPlay(const char *filename)
  {
    return new CAddonSoundPlay(m_Handle, m_Callbacks, filename);
  }

  /*!
   * @brief Remove a played file class
   * @param p The playback to remove
   */
  void ReleaseSoundPlay(CAddonSoundPlay* p)
  {
    delete p;
  }

private:
  AddonCB* m_Handle;
  AddonInstance_AudioDSP *m_Callbacks;
};
