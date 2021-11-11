/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddonSupportCheck.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/ImageDecoder.h"
#include "guilib/iimage.h"

class CPictureInfoTag;

namespace KODI
{
namespace ADDONS
{

class CImageDecoder : public ADDON::IAddonInstanceHandler,
                      public KODI::ADDONS::IAddonSupportCheck,
                      public IImage
{
public:
  explicit CImageDecoder(const ADDON::AddonInfoPtr& addonInfo, const std::string& mimetype);
  ~CImageDecoder() override;

  bool Create();

  bool CreateThumbnailFromSurface(unsigned char*,
                                  unsigned int,
                                  unsigned int,
                                  unsigned int,
                                  unsigned int,
                                  const std::string&,
                                  unsigned char*&,
                                  unsigned int&) override
  {
    return false;
  }

  bool LoadImageFromMemory(unsigned char* buffer,
                           unsigned int bufSize,
                           unsigned int width,
                           unsigned int height) override;
  bool Decode(unsigned char* const pixels,
              unsigned int width,
              unsigned int height,
              unsigned int pitch,
              unsigned int format) override;

  bool LoadInfoTag(const std::string& fileName, CPictureInfoTag* tag);

  // KODI::ADDONS::IAddonSupportCheck related function
  bool SupportsFile(const std::string& filename) override;

private:
  /*! @note m_mimetype not set in all cases, only available if @ref LoadImageFromMemory is used. */
  const std::string m_mimetype;
};

} /* namespace ADDONS */
} /* namespace KODI */
