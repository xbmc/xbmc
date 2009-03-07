/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "File.h"
#include "Favourites.h"
#include "Util.h"
#include "Key.h"
#include "Settings.h"
#include "FileItem.h"

bool CFavourites::Load(CFileItemList &items)
{
  items.Clear();
  CStdString favourites;

  favourites = "special://xbmc/system/favourites.xml";
  if(XFILE::CFile::Exists(favourites))
    CFavourites::LoadFavourites(favourites, items);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no system favourites found, skipping");
  CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "favourites.xml", favourites);
  if(XFILE::CFile::Exists(favourites))
    CFavourites::LoadFavourites(favourites, items);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no userdata favourites found, skipping");

  return true;
}

bool CFavourites::LoadFavourites(CStdString& strPath, CFileItemList& items)
{
  TiXmlDocument doc;
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
      CFileItemPtr item(new CFileItem(name));
      item->m_strPath = favourite->FirstChild()->Value();
      if (thumb) item->SetThumbnailImage(thumb);
      items.Add(item);
    }
    favourite = favourite->NextSiblingElement("favourite");
  }
  return true;
}

bool CFavourites::Save(const CFileItemList &items)
{
  CStdString favourites;
  TiXmlDocument doc;
  TiXmlElement xmlRootElement("favourites");
  TiXmlNode *rootNode = doc.InsertEndChild(xmlRootElement);
  if (!rootNode) return false;

  for (int i = 0; i < items.Size(); i++)
  {
    const CFileItemPtr item = items[i];
    TiXmlElement favNode("favourite");
    favNode.SetAttribute("name", item->GetLabel().c_str());
    if (item->HasThumbnail())
      favNode.SetAttribute("thumb", item->GetThumbnailImage().c_str());
    TiXmlText execute(item->m_strPath);
    favNode.InsertEndChild(execute);
    rootNode->InsertEndChild(favNode);
  }

  CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "favourites.xml", favourites);
  return doc.SaveFile(favourites);
}

bool CFavourites::AddOrRemove(CFileItem *item, DWORD contextWindow)
{
  if (!item) return false;

  // load our list
  CFileItemList items;
  Load(items);

  CStdString executePath(GetExecutePath(item, contextWindow));

  CFileItemPtr match = items.Get(executePath);
  if (match)
  { // remove the item
    items.Remove(match.get());
  }
  else
  { // create our new favourite item
    CFileItemPtr favourite(new CFileItem(item->GetLabel()));
    if (item->GetLabel().IsEmpty())
      favourite->SetLabel(CUtil::GetTitleFromPath(item->m_strPath, item->m_bIsFolder));
    favourite->SetThumbnailImage(item->GetThumbnailImage());
    favourite->m_strPath = executePath;
    items.Add(favourite);
  }

  // and save our list again
  return Save(items);
}

bool CFavourites::IsFavourite(CFileItem *item, DWORD contextWindow)
{
  CFileItemList items;
  if (!Load(items)) return false;

  return items.Contains(GetExecutePath(item, contextWindow));
}

CStdString CFavourites::GetExecutePath(const CFileItem *item, DWORD contextWindow)
{
  CStdString execute;
  if (item->m_bIsFolder && !(item->IsSmartPlayList() || item->IsPlayList()))
    execute.Format("ActivateWindow(%i,%s)", contextWindow, item->m_strPath);
  else if (item->m_strPath.Left(9).Equals("plugin://"))
    execute.Format("RunPlugin(%s)", item->m_strPath);
  else if (contextWindow == WINDOW_SCRIPTS)
    execute.Format("RunScript(%s)", item->m_strPath);
  else if (contextWindow == WINDOW_PROGRAMS)
    execute.Format("RunXBE(%s)", item->m_strPath);
  else  // assume a media file
    execute.Format("PlayMedia(%s)", item->m_strPath);
  return execute;
}
