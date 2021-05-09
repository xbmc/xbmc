/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesService.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/File.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"


static bool LoadFromFile(const std::string& strPath, CFileItemList& items)
{
  CXBMCTinyXML doc;
  if (!doc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Unable to load {} (row {} column {})", strPath, doc.Row(), doc.Column());
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

CFavouritesService::CFavouritesService(std::string userDataFolder)
{
  ReInit(std::move(userDataFolder));
}

void CFavouritesService::ReInit(std::string userDataFolder)
{
  m_userDataFolder = std::move(userDataFolder);
  m_favourites.Clear();

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
  m_events.Publish(FavouritesUpdated{});
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
      favourite->SetArt("thumb", ContentUtils::GetPreferredArtImage(item));
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
  return GetExecutePath(item, std::to_string(contextWindow));
}

std::string CFavouritesService::GetExecutePath(const CFileItem &item, const std::string &contextWindow) const
{
  std::string execute;
  if (URIUtils::IsProtocol(item.GetPath(), "favourites"))
  {
    const CURL url(item.GetPath());
    execute = CURL::Decode(url.GetHostName());
  }
  else if (item.m_bIsFolder && (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders ||
                                !(item.IsSmartPlayList() || item.IsPlayList())))
  {
    if (!contextWindow.empty())
      execute = StringUtils::Format("ActivateWindow({},{},return)", contextWindow,
                                    StringUtils::Paramify(item.GetPath()));
  }
  //! @todo STRING_CLEANUP
  else if (item.IsScript() && item.GetPath().size() > 9) // script://<foo>
    execute = StringUtils::Format("RunScript({})", StringUtils::Paramify(item.GetPath().substr(9)));
  else if (item.IsAddonsPath() && item.GetPath().size() > 9) // addons://<foo>
  {
    CURL url(item.GetPath());
    if (url.GetHostName() == "install")
      execute = "installfromzip";
    else
      execute = StringUtils::Format("RunAddon({})", url.GetFileName());
  }
  else if (item.IsAndroidApp() && item.GetPath().size() > 26) // androidapp://sources/apps/<foo>
    execute = StringUtils::Format("StartAndroidActivity({})",
                                  StringUtils::Paramify(item.GetPath().substr(26)));
  else  // assume a media file
  {
    if (item.IsVideoDb() && item.HasVideoInfoTag())
      execute = StringUtils::Format(
          "PlayMedia({})", StringUtils::Paramify(item.GetVideoInfoTag()->m_strFileNameAndPath));
    else if (item.IsMusicDb() && item.HasMusicInfoTag())
      execute = StringUtils::Format("PlayMedia({})",
                                    StringUtils::Paramify(item.GetMusicInfoTag()->GetURL()));
    else if (item.IsPicture())
      execute = StringUtils::Format("ShowPicture({})", StringUtils::Paramify(item.GetPath()));
    else
      execute = StringUtils::Format("PlayMedia({})", StringUtils::Paramify(item.GetPath()));
  }
  return execute;
}

void CFavouritesService::GetAll(CFileItemList& items) const
{
  CSingleLock lock(m_criticalSection);
  items.Clear();
  items.Copy(m_favourites);
}
