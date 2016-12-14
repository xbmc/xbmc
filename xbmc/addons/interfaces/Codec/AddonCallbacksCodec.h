#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
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

#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libXBMC_codec.h"

namespace KodiAPI
{
namespace V1
{
namespace Codec
{

class CAddonCallbacksCodec : public ADDON::IAddonInterface
{
public:
  CAddonCallbacksCodec(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksCodec();

  /*!
   * @return The callback table.
   */
  CB_CodecLib *GetCallbacks() { return m_callbacks; }

  static xbmc_codec_t GetCodecByName(const void* addonData, const char* strCodecName);

private:
  CB_CodecLib*                           m_callbacks; /*!< callback addresses */
};

} /* namespace Codec */
} /* namespace V1 */
} /* namespace KodiAPI */
