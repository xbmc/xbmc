/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoTagLoaderPlugin.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/PluginDirectory.h"

#include <memory>

using namespace XFILE;

CVideoTagLoaderPlugin::CVideoTagLoaderPlugin(const CFileItem& item, bool forceRefresh)
  : IVideoInfoTagLoader(item, nullptr, false), m_force_refresh(forceRefresh)
{
  if (forceRefresh)
    return;
  // Preserve CFileItem video info and art to avoid info loss between creating VideoInfoTagLoaderFactory and calling Load()
  if (m_item.HasVideoInfoTag())
    m_tag = std::make_unique<CVideoInfoTag>(*m_item.GetVideoInfoTag());
  auto& art = item.GetArt();
  if (!art.empty())
    m_art = std::make_unique<CGUIListItem::ArtMap>(art);
}

bool CVideoTagLoaderPlugin::HasInfo() const
{
  return m_tag || m_force_refresh;
}

CInfoScanner::INFO_TYPE CVideoTagLoaderPlugin::Load(CVideoInfoTag& tag, bool, std::vector<EmbeddedArt>*)
{
  if (m_force_refresh)
  {
    // In case of force refresh call our plugin with option "kodi_action=refresh_info"
    // Plugin must do all refreshing work at specified path and return directory containing one ListItem with video tag and art
    // We cannot obtain all info from setResolvedUrl, because CPluginDirectory::GetPluginResult doesn't copy full art
    CURL url(m_item.GetPath());
    url.SetOption("kodi_action", "refresh_info");
    CPluginDirectory plugin;
    CFileItemList items;
    if (!plugin.GetDirectory(url, items))
      return CInfoScanner::ERROR_NFO;
    if (!items.IsEmpty())
    {
      const CFileItemPtr &item = items[0];
      m_art = std::make_unique<CGUIListItem::ArtMap>(item->GetArt());
      if (item->HasVideoInfoTag())
      {
        tag = *item->GetVideoInfoTag();
        return CInfoScanner::FULL_NFO;
      }
    }
  }
  else if (m_tag)
  {
    // Otherwise just copy CFileItem video info to tag
    tag = *m_tag;
    return CInfoScanner::FULL_NFO;
  }
  return CInfoScanner::NO_NFO;
}
