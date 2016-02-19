#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "addons/binary/callbacks/IAddonCallback.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_codec.h"

namespace V1
{
namespace KodiAPI
{

namespace Codec
{

typedef xbmc_codec_t (*CODECGetCodecByName)(const void* addonData, const char* strCodecName);

typedef struct CB_CODEC
{
  CODECGetCodecByName   GetCodecByName;
} CB_CodecLib;

class CAddonCallbacksCodec : public ADDON::IAddonCallback
{
public:
  CAddonCallbacksCodec(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksCodec();

  static int APILevel() { return m_apiLevel; }
  static std::string Version() { return m_version; }

  /*!
   * @return The callback table.
   */
  CB_CodecLib *GetCallbacks() { return m_callbacks; }

  static xbmc_codec_t GetCodecByName(const void* addonData, const char* strCodecName);

private:
  static constexpr const int   m_apiLevel = 1;
  static constexpr const char* m_version  = "0.0.1";

  CB_CodecLib*                           m_callbacks; /*!< callback addresses */
};

}; /* namespace Codec */

}; /* namespace KoidAPI */
}; /* namespace V1 */
