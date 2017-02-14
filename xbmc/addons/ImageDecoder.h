/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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
#pragma once

#include "AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/ImageDecoder.h"
#include "guilib/iimage.h"

namespace ADDON
{
  class CImageDecoder : public IImage, public IAddonInstanceHandler
  {
  public:
    CImageDecoder(const ADDON::AddonInfoPtr& addonInfo);
    virtual ~CImageDecoder();

    bool Create(const std::string& mimetype);

    bool CreateThumbnailFromSurface(unsigned char*, unsigned int, unsigned int,
                                    unsigned int, unsigned int, const std::string&,
                                    unsigned char*&, unsigned int&) { return false; }

    bool LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                             unsigned int width, unsigned int height);
    bool Decode(unsigned char* const pixels, unsigned int width,
                unsigned int height, unsigned int pitch,
                unsigned int format);

  protected:
    kodi::addon::CInstanceImageDecoder* m_addonInstance;
    AddonInstance_ImageDecoder m_struct;
  };

} /*namespace ADDON*/
