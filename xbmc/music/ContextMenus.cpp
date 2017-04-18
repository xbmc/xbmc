/*
 *      Copyright (C) 2016 Team Kodi
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

#include "ContextMenus.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "tags/MusicInfoTag.h"
#include "PartyModeManager.h"
#include "settings/Settings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "ServiceBroker.h"


namespace CONTEXTMENU
{

static bool IsMusicLibraryItem(const CFileItem& item)
{
  if (!item.IsParentFolder() && URIUtils::IsMusicLibraryContent(item.GetPath()))
  {
    XFILE::CMusicDatabaseDirectory dir;
    XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE nodetype = dir.GetDirectoryType(item.GetPath());
    if (nodetype == XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_ROOT ||
        nodetype == XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_OVERVIEW ||
        nodetype == XFILE::MUSICDATABASEDIRECTORY::NODE_TYPE_TOP100)
    {
      return true;
    }
  }
  return false;
}

CMusicInfo::CMusicInfo(MediaType mediaType)
      : CStaticContextMenuAction(19033), m_mediaType(mediaType) {}

bool CMusicInfo::IsVisible(const CFileItem& item) const
{
  return item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == m_mediaType;
}

bool CMusicInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogMusicInfo::ShowFor(*item);
  return true;
}

bool CPlayPartymode::IsVisible(const CFileItem& item) const
{
  return !item.IsParentFolder() && item.CanQueue() && !item.IsAddonsPath() && !item.IsScript() && item.IsSmartPlayList();
}

bool CPlayPartymode::Execute(const CFileItemPtr& item) const
{
  g_partyModeManager.Enable(PARTYMODECONTEXT_MUSIC, item->GetPath());
  return true;
}

bool CSetDefault::IsVisible(const CFileItem& item) const
{
  return (!item.IsPath(CServiceBroker::GetSettings().GetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW)) &&
          IsMusicLibraryItem(item));
}

std::string CSetDefault::GetQuickpathName(const std::string& strPath) const
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  StringUtils::ToLower(path);
  if (path == "musicdb://genres/")
    return "Genres";
  else if (path == "musicdb://artists/")
    return "Artists";
  else if (path == "musicdb://albums/")
    return "Albums";
  else if (path == "musicdb://songs/")
    return "Songs";
  else if (path == "musicdb://top100/")
    return "Top100";
  else if (path == "musicdb://top100/songs/")
    return "Top100Songs";
  else if (path == "musicdb://top100/albums/")
    return "Top100Albums";
  else if (path == "musicdb://recentlyaddedalbums/")
    return "RecentlyAddedAlbums";
  else if (path == "musicdb://recentlyplayedalbums/")
    return "RecentlyPlayedAlbums";
  else if (path == "musicdb://compilations/")
    return "Compilations";
  else if (path == "musicdb://years/")
    return "Years";
  else if (path == "musicdb://singles/")
    return "Singles";
  else if (path == "special://musicplaylists/")
    return "Playlists";
  else
  {
    CLog::Log(LOGERROR, "CSetDefault::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CSetDefault::Execute(const CFileItemPtr& item) const
{
  CServiceBroker::GetSettings().SetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW, GetQuickpathName(item->GetPath()));
  CServiceBroker::GetSettings().Save();
  return true;
}

bool CClearDefault::IsVisible(const CFileItem& item) const
{
  return (!CServiceBroker::GetSettings().GetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW).empty() &&
          IsMusicLibraryItem(item));
}

bool CClearDefault::Execute(const CFileItemPtr& item) const
{
  CServiceBroker::GetSettings().SetString(CSettings::SETTING_MYMUSIC_DEFAULTLIBVIEW, "");
  CServiceBroker::GetSettings().Save();
  return true;
}

}
