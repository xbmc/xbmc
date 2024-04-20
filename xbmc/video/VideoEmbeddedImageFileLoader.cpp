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
#include "utils/EmbeddedArt.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"
#include "video/tags/IVideoInfoTagLoader.h"
#include "video/tags/VideoInfoTagLoaderFactory.h"

namespace KODI::VIDEO
{

bool CVideoEmbeddedImageFileLoader::CanLoad(const std::string& specialType) const
{
  return StringUtils::StartsWith(specialType, "video_");
}

namespace
{
bool GetEmbeddedThumb(const std::string& path, const std::string& type, EmbeddedArt& art)
{
  CFileItem item(path, false);
  std::unique_ptr<IVideoInfoTagLoader> loader(
      CVideoInfoTagLoaderFactory::CreateLoader(item, ADDON::ScraperPtr(), false));
  CVideoInfoTag tag;
  std::vector<EmbeddedArt> artv;
  if (loader)
    loader->Load(tag, false, &artv);

  for (const auto& it : artv)
  {
    if (it.m_type == type)
    {
      art = it;
      break;
    }
  }
  return !art.Empty();
}
} // namespace

std::unique_ptr<CTexture> VIDEO::CVideoEmbeddedImageFileLoader::Load(
    const std::string& specialType,
    const std::string& filePath,
    unsigned int preferredWidth,
    unsigned int preferredHeight) const
{
  EmbeddedArt art;
  if (GetEmbeddedThumb(filePath, specialType.substr(6), art))
    return CTexture::LoadFromFileInMemory(art.m_data.data(), art.m_size, art.m_mime, preferredWidth,
                                          preferredHeight);
  return {};
}

} // namespace KODI::VIDEO
