/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpecialImageLoaderFactory.h"

#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "music/MusicEmbeddedImageFileLoader.h"
#include "pictures/PictureFolderImageFileLoader.h"
#include "pvr/PVRChannelGroupImageFileLoader.h"
#include "video/VideoEmbeddedImageFileLoader.h"
#include "video/VideoGeneratedImageFileLoader.h"

using namespace IMAGE_FILES;
using namespace KODI;

CSpecialImageLoaderFactory::CSpecialImageLoaderFactory()
{
  m_specialImageLoaders[0] = std::make_unique<VIDEO::CVideoEmbeddedImageFileLoader>();
  m_specialImageLoaders[1] = std::make_unique<MUSIC_INFO::CMusicEmbeddedImageFileLoader>();
  m_specialImageLoaders[2] = std::make_unique<VIDEO::CVideoGeneratedImageFileLoader>();
  m_specialImageLoaders[3] = std::make_unique<CPictureFolderImageFileLoader>();
  m_specialImageLoaders[4] = std::make_unique<PVR::CPVRChannelGroupImageFileLoader>();
}

std::unique_ptr<CTexture> CSpecialImageLoaderFactory::Load(const CImageFileURL& imageFile) const
{
  if (!imageFile.IsSpecialImage())
    return {};
  for (auto& loader : m_specialImageLoaders)
  {
    if (loader->CanLoad(imageFile.GetSpecialType()))
    {
      auto val = loader->Load(imageFile);
      if (val)
        return val;
    }
  }
  return {};
}
