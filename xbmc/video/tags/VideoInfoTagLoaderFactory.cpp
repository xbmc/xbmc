/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoInfoTagLoaderFactory.h"

#include "FileItem.h"
#include "VideoTagLoaderFFmpeg.h"
#include "VideoTagLoaderNFO.h"
#include "VideoTagLoaderPlugin.h"
#include "video/tags/VideoTagExtractionHelper.h"

namespace KODI::VIDEO
{

IVideoInfoTagLoader* CVideoInfoTagLoaderFactory::CreateLoader(const CFileItem& item,
                                                              const ADDON::ScraperPtr& info,
                                                              bool lookInFolder,
                                                              bool forceRefresh)
{
  if (item.IsPlugin() && info && info->ID() == "metadata.local")
  {
    // Direct loading from plugin source with metadata.local scraper
    CVideoTagLoaderPlugin* plugin = new CVideoTagLoaderPlugin(item, forceRefresh);
    if (plugin->HasInfo())
      return plugin;
    delete plugin;
  }

  CVideoTagLoaderNFO* nfo = new CVideoTagLoaderNFO(item, info, lookInFolder);
  if (nfo->HasInfo())
    return nfo;
  delete nfo;

  if (TAGS::CVideoTagExtractionHelper::IsExtractionSupportedFor(item))
  {
    CVideoTagLoaderFFmpeg* ff = new CVideoTagLoaderFFmpeg(item, info, lookInFolder);
    if (ff->HasInfo())
      return ff;
    delete ff;
  }

  return nullptr;
}

} // namespace KODI::VIDEO
