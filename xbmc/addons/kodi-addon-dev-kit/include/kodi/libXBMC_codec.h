#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "xbmc_codec_types.h"
#include "libXBMC_addon.h"

#define CODEC_HELPER_DLL_NAME XBMC_DLL_NAME("codec")
#define CODEC_HELPER_DLL XBMC_DLL("codec")

class CHelper_libXBMC_codec
{
public:
  CHelper_libXBMC_codec(void)
  {
    m_libXBMC_codec = NULL;
    m_Handle        = NULL;
  }

  ~CHelper_libXBMC_codec(void)
  {
    if (m_libXBMC_codec)
    {
      CODEC_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libXBMC_codec);
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
    libBasePath += CODEC_HELPER_DLL;

    m_libXBMC_codec = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libXBMC_codec == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    CODEC_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libXBMC_codec, "CODEC_register_me");
    if (CODEC_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    CODEC_unregister_me = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libXBMC_codec, "CODEC_unregister_me");
    if (CODEC_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    CODEC_get_codec_by_name = (xbmc_codec_t (*)(void* HANDLE, void* CB, const char* strCodecName))
        dlsym(m_libXBMC_codec, "CODEC_get_codec_by_name");
    if (CODEC_get_codec_by_name == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    m_Callbacks = CODEC_register_me(m_Handle);
    return m_Callbacks != NULL;
  }

  /*!
   * @brief Get the codec id used by XBMC
   * @param strCodecName The name of the codec
   * @return The codec_id, or a codec_id with 0 values when not supported
   */
  xbmc_codec_t GetCodecByName(const char* strCodecName)
  {
    return CODEC_get_codec_by_name(m_Handle, m_Callbacks, strCodecName);
  }

protected:
  void* (*CODEC_register_me)(void*);
  void (*CODEC_unregister_me)(void*, void*);
  xbmc_codec_t (*CODEC_get_codec_by_name)(void *HANDLE, void* CB, const char* strCodecName);

private:
  void* m_libXBMC_codec;
  void* m_Handle;
  void* m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};

