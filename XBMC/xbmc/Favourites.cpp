/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Favourites.h"
#include "Util.h"

bool CFavourites::Load(CFileItemList &items)
{
  items.Clear();
  CStdString favourites;
  CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "favourites.xml", favourites);
  TiXmlDocument doc;
  if (!doc.LoadFile(favourites))
  {
    CLog::Log(LOGERROR, "Unable to load %s (row %i column %i)", favourites.c_str(), doc.Row(), doc.Column());
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
    // <favourite name="Cool Video" thumb="q:\userdata\thumbnails\video\04385918.tbn">PlayMedia(c:\videos\cool_video.avi)</favourite>
    // <favourite name="My Album" thumb="q:\userdata\thumbnails\music\0\04385918.tbn">ActivateWindow(MyMusic,c:\music\my album)</favourite>
    // <favourite name="Apple Movie Trailers" thumb="q:\userdata\thumbnails\programs\04385918.tbn">RunScript(q:\scripts\apple movie trailers\default.py)</favourite>
    const char *name = favourite->Attribute("name");
    const char *thumb = favourite->Attribute("thumb");
    if (name && favourite->FirstChild())
    {
      CFileItem *item = new CFileItem(name);
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
    const CFileItem *item = items[i];
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

  CFileItem *match = items.Get(executePath);
  if (match)
  { // remove the item
    items.Remove(match);
  }
  else
  { // create our new favourite item
    CFileItem *favourite = new CFileItem(item->GetLabel());
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
  if (item->m_bIsFolder)
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
