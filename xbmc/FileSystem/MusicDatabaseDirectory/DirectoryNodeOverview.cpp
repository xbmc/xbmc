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

#include "DirectoryNodeOverview.h"
#include "FileItem.h"
#include "MusicDatabase.h"
#include "LocalizeStrings.h"

using namespace std;
using namespace DIRECTORY::MUSICDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_ARTIST;
  else if (GetName()=="3")
    return NODE_TYPE_ALBUM;
  else if (GetName()=="4")
    return NODE_TYPE_SONG;
  else if (GetName()=="5")
    return NODE_TYPE_TOP100;
  else if (GetName()=="6")
    return NODE_TYPE_ALBUM_RECENTLY_ADDED;
  else if (GetName()=="7")
    return NODE_TYPE_ALBUM_RECENTLY_PLAYED;
  else if (GetName()=="8")
    return NODE_TYPE_ALBUM_COMPILATIONS;
  else if (GetName()=="9")
    return NODE_TYPE_YEAR;
  else if (GetName()=="10")
    return NODE_TYPE_SINGLES;
  return NODE_TYPE_NONE;
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items)
{
  vector< pair<int, int> > rootItems;
  CMusicDatabase musicDatabase;
  bool showSingles = false;
  if (musicDatabase.Open())
  {
    if (musicDatabase.GetSongsCount("where strAlbum=''") > 0)
      showSingles = true;
    musicDatabase.Close();
  }

  rootItems.push_back(make_pair(1, 135));
  rootItems.push_back(make_pair(2, 133));
  rootItems.push_back(make_pair(3, 132));
  if (showSingles)
    rootItems.push_back(make_pair(10, 1050));
  rootItems.push_back(make_pair(4, 134));
  rootItems.push_back(make_pair(9, 652));
  rootItems.push_back(make_pair(5, 271));
  rootItems.push_back(make_pair(6, 359));
  rootItems.push_back(make_pair(7, 517));
  rootItems.push_back(make_pair(8, 521));

  for (unsigned int i = 0; i < rootItems.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(rootItems[i].second)));
    CStdString strDir;
    strDir.Format("%i/", rootItems[i].first);
    pItem->m_strPath = BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  items.m_strPath = "";
  return true;
}
