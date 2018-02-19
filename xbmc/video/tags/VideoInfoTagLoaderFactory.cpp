/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#include "VideoInfoTagLoaderFactory.h"
#include "VideoTagLoaderFFmpeg.h"
#include "VideoTagLoaderNFO.h"
#include "VideoTagLoaderPlugin.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"

using namespace VIDEO;

IVideoInfoTagLoader* CVideoInfoTagLoaderFactory::CreateLoader(const CFileItem& item,
                                                              ADDON::ScraperPtr info,
                                                              bool lookInFolder,
                                                              bool forceRefresh)
{
  // don't try to read tags for streams
  if (item.IsInternetStream())
    return nullptr;

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

  if (CServiceBroker::GetSettings().GetBool(CSettings::SETTING_MYVIDEOS_USETAGS) &&
      (item.IsType(".mkv") || item.IsType(".mp4") || item.IsType(".avi")))
  {
    CVideoTagLoaderFFmpeg* ff = new CVideoTagLoaderFFmpeg(item, info, lookInFolder);
    if (ff->HasInfo())
      return ff;
    delete ff;
  }

  return nullptr;
}
