/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoGeneratedImageFileLoader.h"

#include "DVDFileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/DirectoryCache.h"
#include "guilib/Texture.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

namespace KODI::VIDEO
{

bool CVideoGeneratedImageFileLoader::CanLoad(const std::string& specialType) const
{
  return specialType == "video";
}

namespace
{
void SetupRarOptions(CFileItem& item, const std::string& path)
{
  std::string path2(path);
  if (IsVideoDb(item) && item.HasVideoInfoTag())
    path2 = item.GetVideoInfoTag()->m_strFileNameAndPath;
  CURL url(path2);
  std::string opts = url.GetOptions();
  if (opts.find("flags") != std::string::npos)
    return;
  if (opts.size())
    opts += "&flags=8";
  else
    opts = "?flags=8";
  url.SetOptions(opts);
  if (IsVideoDb(item) && item.HasVideoInfoTag())
    item.GetVideoInfoTag()->m_strFileNameAndPath = url.Get();
  else
    item.SetPath(url.Get());
  g_directoryCache.ClearDirectory(url.GetWithoutFilename());
}
} // namespace

std::unique_ptr<CTexture> VIDEO::CVideoGeneratedImageFileLoader::Load(
    const std::string& specialType, const std::string& filePath, unsigned int, unsigned int) const
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB))
  {
    return {};
  }

  CFileItem item{filePath, false};

  if (URIUtils::IsInRAR(filePath))
    SetupRarOptions(item, filePath);

  return CDVDFileInfo::ExtractThumbToTexture(item);
}

} // namespace KODI::VIDEO
