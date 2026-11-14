/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoEmbeddedImageFileLoader.h"

#include "FileItem.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "utils/EmbeddedArt.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"
#include "video/tags/VideoTagLoaderFFmpeg.h"

namespace KODI::VIDEO
{

bool CVideoEmbeddedImageFileLoader::CanLoad(const std::string& specialType) const
{
  return StringUtils::StartsWith(specialType, "video_");
}

bool CVideoEmbeddedImageFileLoader::GetEmbeddedArt(const std::string& path,
                                                   const std::string& type,
                                                   EmbeddedArt& art)
{
  CFileItem item(path, false);
  std::vector<EmbeddedArt> artv;

  // The file's own loader, as the factory would give an nfo beside the file, which holds no art
  CVideoTagLoaderFFmpeg loader{item, nullptr, false};
  CVideoInfoTag tag;
  loader.Load(tag, false, &artv);

  for (const auto& it : artv)
  {
    if (it.m_type == type)
    {
      art = it;
      return !art.Empty();
    }
  }
  return false;
}

std::unique_ptr<CTexture> CVideoEmbeddedImageFileLoader::Load(
    const IMAGE_FILES::CImageFileURL& imageFile) const
{
  EmbeddedArt art;
  if (GetEmbeddedArt(imageFile.GetTargetFile(), imageFile.GetSpecialType().substr(6), art))
    return CTexture::LoadFromFileInMemory(art.m_data.data(), art.m_size, art.m_mime);
  return {};
}

} // namespace KODI::VIDEO
