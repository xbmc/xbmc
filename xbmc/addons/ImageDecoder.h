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
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_imagedec_types.h"
#include "guilib/iimage.h"

namespace ADDON
{
  class CImageDecoder : public CAddonDll,
                        public IImage
  {
  public:
    static std::unique_ptr<CImageDecoder> FromExtension(CAddonInfo&&,
                                                        const cp_extension_t* ext);
    explicit CImageDecoder(CAddonInfo addonInfo) :
      CAddonDll(std::move(addonInfo))
    {}

    CImageDecoder(CAddonInfo&& addonInfo, std::string mimetypes, std::string extensions);
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

    const std::string& GetMimetypes() const { return m_mimetype; }
    const std::string& GetExtensions() const { return m_extension; }
  protected:
    void* m_image = nullptr;
    std::string m_mimetype;
    std::string m_extension;
    AddonInstance_ImageDecoder m_struct = {};
  };

} /*namespace ADDON*/
