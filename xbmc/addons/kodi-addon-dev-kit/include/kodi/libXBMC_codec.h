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

extern "C"
{
namespace KodiAPI
{
namespace Codec
{

typedef struct CB_CODEC
{
  xbmc_codec_t (*GetCodecByName)(const void* addonData, const char* strCodecName);
} CB_CodecLib;

} /* namespace Codec */
} /* namespace KodiAPI */
} /* extern "C" */

class CHelper_libXBMC_codec
{
public:
  CHelper_libXBMC_codec(void)
  {
    m_Handle = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libXBMC_codec(void)
  {
    if (m_Handle && m_Callbacks)
    {
      m_Handle->CodecLib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
    }
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
      m_Callbacks = (KodiAPI::Codec::CB_CODEC*)m_Handle->CodecLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libXBMC_codec-ERROR: CodecLib_RegisterMe can't get callback table from Kodi !!!\n");

    return m_Callbacks != nullptr;
  }

  /*!
   * @brief Get the codec id used by XBMC
   * @param strCodecName The name of the codec
   * @return The codec_id, or a codec_id with 0 values when not supported
   */
  xbmc_codec_t GetCodecByName(const char* strCodecName)
  {
    return m_Callbacks->GetCodecByName(m_Handle->addonData, strCodecName);
  }

private:
  AddonCB* m_Handle;
  KodiAPI::Codec::CB_CODEC *m_Callbacks;
};
