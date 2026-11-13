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
#include "VideoInfoScannerLookups.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/IFileTypes.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <charconv>

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
  if (!opts.empty())
    opts += "&flags=8";
  else
    opts = "?flags=8";
  url.SetOptions(opts);
  if (IsVideoDb(item) && item.HasVideoInfoTag())
    item.GetVideoInfoTag()->m_strFileNameAndPath = url.Get();
  else
    item.SetPath(url.Get());
  g_directoryCache.ClearDirectory(CURL(url.GetWithoutFilename()));
}
} // namespace

std::unique_ptr<CTexture> CVideoGeneratedImageFileLoader::Load(
    const IMAGE_FILES::CImageFileURL& imageFile) const
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MYVIDEOS_EXTRACTTHUMB))
  {
    return {};
  }

  const std::string& filePath = imageFile.GetTargetFile();
  CFileItem item{filePath, false};

  if (URIUtils::IsInRAR(filePath))
    SetupRarOptions(item, filePath);

  std::string chapterOption = imageFile.GetOption("chapter");
  int chapter = 0;
  std::from_chars(chapterOption.data(), chapterOption.data() + chapterOption.size(), chapter);

  if (!CScanProbe::Scanning())
    return CDVDFileInfo::ExtractThumbToTexture(item, chapter);

  // Decoding a frame from a point in the file reads little of it, so it is read without the file
  // cache. As a scan's art is cached while it runs, a frame is taken in turn with the scan's probes
  // where the file's client makes one network call at a time.
  std::unique_ptr<CTexture> texture;
  CScanProbe::Run(item.GetPath(),
                  [&]
                  {
                    texture =
                        CDVDFileInfo::ExtractThumbToTexture(item, chapter, XFILE::READ_NO_CACHE);
                    return texture != nullptr;
                  });
  return texture;
}

} // namespace KODI::VIDEO
