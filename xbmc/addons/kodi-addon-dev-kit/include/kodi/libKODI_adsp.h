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

typedef void* ADSPHANDLE;

#define ADSP_HELPER_DLL KODI_DLL("adsp")
#define ADSP_HELPER_DLL_NAME KODI_DLL_NAME("adsp")

class CAddonSoundPlay;

class CHelper_libKODI_adsp
{
public:
  CHelper_libKODI_adsp(void)
  {
    m_libKODI_adsp = NULL;
    m_Handle      = NULL;
  }

  ~CHelper_libKODI_adsp(void)
  {
    if (m_libKODI_adsp)
    {
      ADSP_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libKODI_adsp);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += ADSP_HELPER_DLL;

    m_libKODI_adsp = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_adsp == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    ADSP_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libKODI_adsp, "ADSP_register_me");
    if (ADSP_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_unregister_me = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libKODI_adsp, "ADSP_unregister_me");
    if (ADSP_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_add_menu_hook = (void (*)(void* HANDLE, void* CB, AE_DSP_MENUHOOK *hook))
      dlsym(m_libKODI_adsp, "ADSP_add_menu_hook");
    if (ADSP_add_menu_hook == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_remove_menu_hook = (void (*)(void* HANDLE, void* CB, AE_DSP_MENUHOOK *hook))
      dlsym(m_libKODI_adsp, "ADSP_remove_menu_hook");
    if (ADSP_remove_menu_hook == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_register_mode = (void (*)(void *HANDLE, void* CB, AE_DSP_MODES::AE_DSP_MODE *modes))
      dlsym(m_libKODI_adsp, "ADSP_register_mode");
    if (ADSP_register_mode == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_unregister_mode = (void (*)(void* HANDLE, void* CB, AE_DSP_MODES::AE_DSP_MODE *modes))
      dlsym(m_libKODI_adsp, "ADSP_unregister_mode");
    if (ADSP_unregister_mode == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_get_sound_play = (CAddonSoundPlay* (*)(void *HANDLE, void *CB, const char *filename))
      dlsym(m_libKODI_adsp, "ADSP_get_sound_play");
    if (ADSP_get_sound_play == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    ADSP_release_sound_play = (void (*)(CAddonSoundPlay* p))
      dlsym(m_libKODI_adsp, "ADSP_release_sound_play");
    if (ADSP_release_sound_play == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    m_Callbacks = ADSP_register_me(m_Handle);
    return m_Callbacks != NULL;
  }

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param hook The hook to add
   */
  void AddMenuHook(AE_DSP_MENUHOOK* hook)
  {
    return ADSP_add_menu_hook(m_Handle, m_Callbacks, hook);
  }

  /*!
   * @brief Remove a menu hook for the context menu for this add-on
   * @param hook The hook to remove
   */
  void RemoveMenuHook(AE_DSP_MENUHOOK* hook)
  {
    return ADSP_remove_menu_hook(m_Handle, m_Callbacks, hook);
  }

  /*!
   * @brief Add or replace master mode information inside audio dsp database.
   * Becomes identifier written inside mode to iModeID if it was 0 (undefined)
   * @param mode The master mode to add or update inside database
   */
  void RegisterMode(AE_DSP_MODES::AE_DSP_MODE* mode)
  {
    return ADSP_register_mode(m_Handle, m_Callbacks, mode);
  }

  /*!
   * @brief Remove a master mode from audio dsp database
   * @param mode The Mode to remove
   */
  void UnregisterMode(AE_DSP_MODES::AE_DSP_MODE* mode)
  {
    return ADSP_unregister_mode(m_Handle, m_Callbacks, mode);
  }

  /*!
   * @brief Open a sound playing class
   * @param filename The wav filename to open
   */
  CAddonSoundPlay* GetSoundPlay(const char *filename)
  {
    return ADSP_get_sound_play(m_Handle, m_Callbacks, filename);
  }

  /*!
   * @brief Remove a played file class
   * @param p The playback to remove
   */
  void ReleaseSoundPlay(CAddonSoundPlay* p)
  {
    return ADSP_release_sound_play(p);
  }

protected:
  void* (*ADSP_register_me)(void*);

  void (*ADSP_unregister_me)(void*, void*);
  void (*ADSP_add_menu_hook)(void*, void*, AE_DSP_MENUHOOK*);
  void (*ADSP_remove_menu_hook)(void*, void*, AE_DSP_MENUHOOK*);
  void (*ADSP_register_mode)(void*, void*, AE_DSP_MODES::AE_DSP_MODE*);
  void (*ADSP_unregister_mode)(void*, void*, AE_DSP_MODES::AE_DSP_MODE*);
  CAddonSoundPlay* (*ADSP_get_sound_play)(void*, void*, const char *);
  void (*ADSP_release_sound_play)(CAddonSoundPlay*);

private:
  void* m_libKODI_adsp;
  void* m_Handle;
  void* m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};

class CAddonSoundPlay
{
public:
  CAddonSoundPlay(void *hdl, void *cb, const char *filename);
  virtual ~CAddonSoundPlay();

  /*! play the sound this object represents */
  virtual void Play();

  /*! stop playing the sound this object represents */
  virtual void Stop();

  /*! return true if the sound is currently playing */
  virtual bool IsPlaying();

  /*! set the playback channel position of this sound, AE_DSP_CH_INVALID for all */
  virtual void SetChannel(AE_DSP_CHANNEL channel);

  /*! get the current playback volume of this sound, AE_DSP_CH_INVALID for all */
  virtual AE_DSP_CHANNEL GetChannel();

  /*! set the playback volume of this sound */
  virtual void SetVolume(float volume);

  /*! get the current playback volume of this sound */
  virtual float GetVolume();

private:
  std::string m_Filename;
  void       *m_Handle;
  void       *m_cb;
  ADSPHANDLE  m_PlayHandle;
};
