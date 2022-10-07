/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesService.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "filesystem/File.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <mutex>

namespace
{
bool IsMediasourceOfFavItemUnlocked(const std::shared_ptr<CFileItem>& item)
{
  if (!item)
  {
    CLog::Log(LOGERROR, "{}: No item passed (nullptr).", __func__);
    return true;
  }

  std::string execString = CURL::Decode(item->GetPath());
  std::string execute;
  std::vector<std::string> params;

  CUtil::SplitExecFunction(execString, execute, params);

  FavAction favAction;
  if (StringUtils::EqualsNoCase(execute, "Favourites://PlayMedia"))
    favAction = FavAction::PLAYMEDIA;
  else if (StringUtils::EqualsNoCase(execute, "Favourites://ShowPicture"))
    favAction = FavAction::SHOWPICTURE;
  else
    return true;

  const auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
  {
    CLog::Log(LOGERROR, "{}: returned nullptr.", __func__);
    return true;
  }

  const auto profileManager = settingsComponent->GetProfileManager();
  if (!profileManager)
  {
    CLog::Log(LOGERROR, "{}: returned nullptr.", __func__);
    return true;
  }

  bool isFolder{false};
  CFileItem itemToCheck(params[0], isFolder);

  if (favAction == FavAction::PLAYMEDIA)
  {
    if (itemToCheck.IsVideo())
    {
      if (!profileManager->GetCurrentProfile().videoLocked())
        return g_passwordManager.IsMediaFileUnlocked("video", itemToCheck.GetPath());

      return false;
    }
    else if (itemToCheck.IsAudio())
    {
      if (!profileManager->GetCurrentProfile().musicLocked())
        return g_passwordManager.IsMediaFileUnlocked("music", itemToCheck.GetPath());

      return false;
    }
  }
  else if (favAction == FavAction::SHOWPICTURE && itemToCheck.IsPicture())
  {
    if (!profileManager->GetCurrentProfile().picturesLocked())
      return g_passwordManager.IsMediaFileUnlocked("pictures", itemToCheck.GetPath());

    return false;
  }

  return true;
}

bool LoadFromFile(const std::string& strPath, CFileItemList& items)
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
} // unnamed namespace

CFavouritesService::CFavouritesService(std::string userDataFolder) : m_favourites("favourites://")
{
  ReInit(std::move(userDataFolder));
}

void CFavouritesService::ReInit(std::string userDataFolder)
{
  m_userDataFolder = std::move(userDataFolder);
  m_favourites.Clear();
  m_favourites.SetContent("favourites");

  std::string favourites = "special://xbmc/system/favourites.xml";
  if (XFILE::CFile::Exists(favourites))
    LoadFromFile(favourites, m_favourites);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no system favourites found, skipping");

  favourites = URIUtils::AddFileToFolder(m_userDataFolder, "favourites.xml");
  if (XFILE::CFile::Exists(favourites))
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
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
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
  url.SetHostName(CURL::Encode(CUtil::GetExecPath(item, std::to_string(contextWindow))));
  return url.Get();
}

bool CFavouritesService::AddOrRemove(const CFileItem& item, int contextWindow)
{
  auto favUrl = GetFavouritesUrl(item, contextWindow);
  {
    std::unique_lock<CCriticalSection> lock(m_criticalSection);
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
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  return m_favourites.Contains(GetFavouritesUrl(item, contextWindow));
}

void CFavouritesService::GetAll(CFileItemList& items) const
{
  std::unique_lock<CCriticalSection> lock(m_criticalSection);
  items.Clear();
  if (g_passwordManager.IsMasterLockUnlocked(false)) // don't prompt
  {
    items.Copy(m_favourites, true); // copy items
  }
  else
  {
    for (const auto& fav : m_favourites)
    {
      if (IsMediasourceOfFavItemUnlocked(fav))
        items.Add(fav);
    }
  }

  int index = 0;
  for (const auto& item : items)
  {
    item->SetProperty("favourite.action", CUtil::GetExecActionLabelFromPath(item->GetPath()));
    item->SetProperty("favourite.provider", CUtil::GetExecProviderLabelFromPath(item->GetPath()));
    item->SetProperty("favourite.index", index++);
  }
}

void CFavouritesService::RefreshFavourites()
{
  m_events.Publish(FavouritesUpdated{});
}
