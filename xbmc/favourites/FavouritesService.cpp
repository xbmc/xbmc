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

#include "FavouritesService.h"
#include "filesystem/File.h"
#include "Util.h"
#include "interfaces/AnnouncementManager.h"
#include "profiles/ProfilesManager.h"
#include "FileItem.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "URL.h"


static bool LoadFromFile(const std::string& strPath, CFileItemList& items)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Unable to load %s (row %i column %i)", strPath.c_str(), doc.Row(), doc.Column());
    return false;
  }
  TiXmlElement *root = doc.RootElement();
  if (!root || strcmp(root->Value(), "favourites"))
  {
    CLog::Log(LOGERROR, "Favourites.xml doesn't contain the <favourites> root element");
    return false;
  }

  TiXmlElement *favourite = root->FirstChildElement("favourite");
  while (favourite)
  {
    // format:
    // <favourite name="Cool Video" thumb="foo.jpg">PlayMedia(c:\videos\cool_video.avi)</favourite>
    // <favourite name="My Album" thumb="bar.tbn">ActivateWindow(MyMusic,c:\music\my album)</favourite>
    // <favourite name="Apple Movie Trailers" thumb="path_to_thumb.png">RunScript(special://xbmc/scripts/apple movie trailers/default.py)</favourite>
    const char *name = favourite->Attribute("name");
    const char *thumb = favourite->Attribute("thumb");
    if (name && favourite->FirstChild())
    {
      CURL url;
      url.SetProtocol("favourites");
      url.SetHostName(CURL::Encode(favourite->FirstChild()->Value()));
      const std::string favURL(url.Get());
      if (!items.Contains(favURL))
      {
        const CFileItemPtr item(std::make_shared<CFileItem>(name));
        item->SetPath(favURL);
        if (thumb)
          item->SetArt("thumb", thumb);
        items.Add(item);
      }
    }
    favourite = favourite->NextSiblingElement("favourite");
  }
  return true;
}

CFavouritesService::CFavouritesService(std::string userDataFolder) : m_userDataFolder(std::move(userDataFolder))
{
  CFileItemList items;
  std::string favourites = "special://xbmc/system/favourites.xml";
  if(XFILE::CFile::Exists(favourites))
    LoadFromFile(favourites, m_favourites);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no system favourites found, skipping");

  favourites = URIUtils::AddFileToFolder(m_userDataFolder, "favourites.xml");
  if(XFILE::CFile::Exists(favourites))
    LoadFromFile(favourites, m_favourites);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no userdata favourites found, skipping");
}

bool CFavouritesService::Persist()
{
  CXBMCTinyXML doc;
  TiXmlElement xmlRootElement("favourites");
  TiXmlNode *rootNode = doc.InsertEndChild(xmlRootElement);
  if (!rootNode)
    return false;

  for (const auto& item : m_favourites)
  {
    TiXmlElement favNode("favourite");
    favNode.SetAttribute("name", item->GetLabel().c_str());
    if (item->HasArt("thumb"))
      favNode.SetAttribute("thumb", item->GetArt("thumb").c_str());

    const CURL url(item->GetPath());
    TiXmlText execute(CURL::Decode(url.GetHostName()));
    favNode.InsertEndChild(execute);
    rootNode->InsertEndChild(favNode);
  }

  auto path = URIUtils::AddFileToFolder(m_userDataFolder, "favourites.xml");
  return doc.SaveFile(path);
}

bool CFavouritesService::Save(const CFileItemList& items)
{
  {
    CSingleLock lock(m_criticalSection);
    m_favourites.Clear();
    m_favourites.Copy(items);
    Persist();
  }
  OnUpdated();
  return true;
}

void CFavouritesService::OnUpdated()
{
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::GUI, "xbmc", "OnFavouritesUpdated");
}

std::string CFavouritesService::GetFavouritesUrl(const CFileItem& item, int contextWindow) const
{
  CURL url;
  url.SetProtocol("favourites");
  url.SetHostName(CURL::Encode(GetExecutePath(item, contextWindow)));
  return url.Get();
}

bool CFavouritesService::AddOrRemove(const CFileItem& item, int contextWindow)
{
  auto favUrl = GetFavouritesUrl(item, contextWindow);
  {
    CSingleLock lock(m_criticalSection);
    CFileItemPtr match = m_favourites.Get(favUrl);
    if (match)
    { // remove the item
      m_favourites.Remove(match.get());
    }
    else
    { // create our new favourite item
      const CFileItemPtr favourite(std::make_shared<CFileItem>(item.GetLabel()));
      if (item.GetLabel().empty())
        favourite->SetLabel(CUtil::GetTitleFromPath(item.GetPath(), item.m_bIsFolder));
      favourite->SetArt("thumb", item.GetArt("thumb"));
      favourite->SetPath(favUrl);
      m_favourites.Add(favourite);
    }
    Persist();
  }
  OnUpdated();
  return true;
}

bool CFavouritesService::IsFavourited(const CFileItem& item, int contextWindow) const
{
  CSingleLock lock(m_criticalSection);
  return m_favourites.Contains(GetFavouritesUrl(item, contextWindow));
}

std::string CFavouritesService::GetExecutePath(const CFileItem &item, int contextWindow) const
{
  return GetExecutePath(item, StringUtils::Format("%i", contextWindow));
}

std::string CFavouritesService::GetExecutePath(const CFileItem &item, const std::string &contextWindow) const
{
  std::string execute;
  if (URIUtils::IsProtocol(item.GetPath(), "favourites"))
  {
    const CURL url(item.GetPath());
    execute = CURL::Decode(url.GetHostName());
  }
  else if (item.m_bIsFolder && (g_advancedSettings.m_playlistAsFolders ||
                                !(item.IsSmartPlayList() || item.IsPlayList())))
  {
    if (!contextWindow.empty())
      execute = StringUtils::Format("ActivateWindow(%s,%s,return)", contextWindow.c_str(), StringUtils::Paramify(item.GetPath()).c_str());
  }
  //! @todo STRING_CLEANUP
  else if (item.IsScript() && item.GetPath().size() > 9) // script://<foo>
    execute = StringUtils::Format("RunScript(%s)", StringUtils::Paramify(item.GetPath().substr(9)).c_str());
  else if (item.IsAddonsPath() && item.GetPath().size() > 9) // addons://<foo>
  {
    CURL url(item.GetPath());
    execute = StringUtils::Format("RunAddon(%s)", url.GetFileName().c_str());
  }
  else if (item.IsAndroidApp() && item.GetPath().size() > 26) // androidapp://sources/apps/<foo>
    execute = StringUtils::Format("StartAndroidActivity(%s)", StringUtils::Paramify(item.GetPath().substr(26)).c_str());
  else  // assume a media file
  {
    if (item.IsVideoDb() && item.HasVideoInfoTag())
      execute = StringUtils::Format("PlayMedia(%s)", StringUtils::Paramify(item.GetVideoInfoTag()->m_strFileNameAndPath).c_str());
    else if (item.IsMusicDb() && item.HasMusicInfoTag())
      execute = StringUtils::Format("PlayMedia(%s)", StringUtils::Paramify(item.GetMusicInfoTag()->GetURL()).c_str());
    else if (item.IsPicture())
      execute = StringUtils::Format("ShowPicture(%s)", StringUtils::Paramify(item.GetPath()).c_str());
    else
      execute = StringUtils::Format("PlayMedia(%s)", StringUtils::Paramify(item.GetPath()).c_str());
  }
  return execute;
}

void CFavouritesService::GetAll(CFileItemList& items) const
{
  CSingleLock lock(m_criticalSection);
  items.Clear();
  items.Copy(m_favourites);
}
