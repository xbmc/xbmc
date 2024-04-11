/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicEmbeddedImageFileLoader.h"

#include "FileItem.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "music/tags/ImusicInfoTagLoader.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "utils/EmbeddedArt.h"

using namespace MUSIC_INFO;

bool CMusicEmbeddedImageFileLoader::CanLoad(const std::string& specialType) const
{
  return specialType == "music";
}

namespace
{
bool GetEmbeddedThumb(const std::string& path, EmbeddedArt& art)
{
  CFileItem item(path, false);
  std::unique_ptr<IMusicInfoTagLoader> loader(CMusicInfoTagLoaderFactory::CreateLoader(item));
  CMusicInfoTag tag;
  if (loader)
    loader->Load(path, tag, &art);

  return !art.Empty();
}
} // namespace

std::unique_ptr<CTexture> CMusicEmbeddedImageFileLoader::Load(
    const IMAGE_FILES::CImageFileURL& imageFile) const
{
  EmbeddedArt art;
  if (GetEmbeddedThumb(imageFile.GetTargetFile(), art))
    return CTexture::LoadFromFileInMemory(art.m_data.data(), art.m_size, art.m_mime);
  return nullptr;
}
