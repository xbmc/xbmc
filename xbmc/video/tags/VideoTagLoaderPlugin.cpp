/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoTagLoaderPlugin.h"
#include "FileItem.h"
#include "URL.h"
#include "filesystem/PluginDirectory.h"

using namespace XFILE;

CVideoTagLoaderPlugin::CVideoTagLoaderPlugin(const CFileItem& item, bool forceRefresh)
  : IVideoInfoTagLoader(item, nullptr, false), m_force_refresh(forceRefresh)
{
  if (forceRefresh)
    return;
  // Preserve CFileItem video info and art to avoid info loss between creating VideoInfoTagLoaderFactory and calling Load()
  if (m_item.HasVideoInfoTag())
    m_tag.reset(new CVideoInfoTag(*m_item.GetVideoInfoTag()));
  auto& art = item.GetArt();
  if (!art.empty())
    m_art.reset(new CGUIListItem::ArtMap(art));
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
      m_art.reset(new CGUIListItem::ArtMap(item->GetArt()));
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
