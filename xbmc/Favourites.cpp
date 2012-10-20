/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Favourites.h"
#include "filesystem/File.h"
#include "Util.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "settings/AdvancedSettings.h"
#include "video/VideoInfoTag.h"

bool CFavourites::Load(CFileItemList &items)
{
  items.Clear();
  CStdString favourites;

  favourites = "special://xbmc/system/favourites.xml";
  if(XFILE::CFile::Exists(favourites))
    CFavourites::LoadFavourites(favourites, items);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no system favourites found, skipping");
  URIUtils::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "favourites.xml", favourites);
  if(XFILE::CFile::Exists(favourites))
    CFavourites::LoadFavourites(favourites, items);
  else
    CLog::Log(LOGDEBUG, "CFavourites::Load - no userdata favourites found, skipping");

  return true;
}

bool CFavourites::LoadFavourites(CStdString& strPath, CFileItemList& items)
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
      if(!items.Contains(favourite->FirstChild()->Value()))
      {
        CFileItemPtr item(new CFileItem(name));
        item->SetPath(favourite->FirstChild()->Value());
        if (thumb) item->SetArt("thumb", thumb);
        items.Add(item);
      }
    }
    favourite = favourite->NextSiblingElement("favourite");
  }
  return true;
}

bool CFavourites::Save(const CFileItemList &items)
{
  CStdString favourites;
  CXBMCTinyXML doc;
  TiXmlElement xmlRootElement("favourites");
  TiXmlNode *rootNode = doc.InsertEndChild(xmlRootElement);
  if (!rootNode) return false;

  for (int i = 0; i < items.Size(); i++)
  {
    const CFileItemPtr item = items[i];
    TiXmlElement favNode("favourite");
    favNode.SetAttribute("name", item->GetLabel().c_str());
    if (item->HasArt("thumb"))
      favNode.SetAttribute("thumb", item->GetArt("thumb").c_str());
    TiXmlText execute(item->GetPath());
    favNode.InsertEndChild(execute);
    rootNode->InsertEndChild(favNode);
  }

  URIUtils::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "favourites.xml", favourites);
  return doc.SaveFile(favourites);
}

bool CFavourites::AddOrRemove(CFileItem *item, int contextWindow)
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
      favourite->SetLabel(CUtil::GetTitleFromPath(item->GetPath(), item->m_bIsFolder));
    favourite->SetArt("thumb", item->GetArt("thumb"));
    favourite->SetPath(executePath);
    items.Add(favourite);
  }

  // and save our list again
  return Save(items);
}

bool CFavourites::IsFavourite(CFileItem *item, int contextWindow)
{
  CFileItemList items;
  if (!Load(items)) return false;

  return items.Contains(GetExecutePath(item, contextWindow));
}

static CStdString Paramify(const CStdString& param)
{
  CStdString result(param);
  result.Replace("\\", "\\\\");
  result.Replace("\"", "\\\"");
  return "\"" + result + "\"";
}

#ifdef UNIT_TESTING
bool CFavourites::TestParamify()
{
  return (Paramify("test") == "\"test\"" &&
          Paramify("test\"foo\"test") == "\"test\\\"foo\\\"test\"" &&
          Paramify("C:\\foo\\bar\\") == "\"C:\\\\foo\\\\bar\\\\\"");
}
#endif

CStdString CFavourites::GetExecutePath(const CFileItem *item, int contextWindow)
{
  CStdString execute;
  if (item->m_bIsFolder && (g_advancedSettings.m_playlistAsFolders ||
                            !(item->IsSmartPlayList() || item->IsPlayList())))
    execute.Format("ActivateWindow(%i,%s)", contextWindow, Paramify(item->GetPath()));
  else if (item->IsScript())
    execute.Format("RunScript(%s)", Paramify(item->GetPath().Mid(9)));
  else  // assume a media file
  {
    if (item->IsVideoDb() && item->HasVideoInfoTag())
      execute.Format("PlayMedia(%s)", Paramify(item->GetVideoInfoTag()->m_strFileNameAndPath));
    else
      execute.Format("PlayMedia(%s)", Paramify(item->GetPath()));
  }
  return execute;
}
