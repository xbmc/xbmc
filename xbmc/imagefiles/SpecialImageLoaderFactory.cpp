/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpecialImageLoaderFactory.h"

#include "guilib/Texture.h"
#include "video/VideoEmbeddedImageFileLoader.h"

using namespace IMAGE_FILES;

CSpecialImageLoaderFactory::CSpecialImageLoaderFactory()
{
  m_specialImageLoaders[0] = std::make_unique<VIDEO::CVideoEmbeddedImageFileLoader>();
}

std::unique_ptr<CTexture> CSpecialImageLoaderFactory::Load(std::string specialType,
                                                           std::string filePath,
                                                           unsigned int preferredWidth,
                                                           unsigned int preferredHeight) const
{
  if (specialType.empty())
    return {};
  for (auto& loader : m_specialImageLoaders)
  {
    if (loader->CanLoad(specialType))
    {
      auto val = loader->Load(specialType, filePath, preferredWidth, preferredHeight);
      if (val)
        return val;
    }
  }
  return {};
}
