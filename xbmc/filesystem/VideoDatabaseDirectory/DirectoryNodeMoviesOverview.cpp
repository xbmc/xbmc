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

#include "DirectoryNodeMoviesOverview.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;
using namespace std;

CDirectoryNodeMoviesOverview::CDirectoryNodeMoviesOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MOVIES_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMoviesOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_TITLE_MOVIES;
  else if (GetName()=="3")
    return NODE_TYPE_YEAR;
  else if (GetName()=="4")
    return NODE_TYPE_ACTOR;
  else if (GetName()=="5")
    return NODE_TYPE_DIRECTOR;
  else if (GetName()=="6")
    return NODE_TYPE_STUDIO;
  else if (GetName()=="7")
    return NODE_TYPE_SETS;
  else if (GetName()=="8")
    return NODE_TYPE_COUNTRY;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeMoviesOverview::GetContent(CFileItemList& items)
{
  vector<pair<const char*, int> > vecRoot;
  vecRoot.push_back(make_pair("1", 135));  // Genres
  vecRoot.push_back(make_pair("2", 369));  // Title
  vecRoot.push_back(make_pair("3", 562));  // Year
  vecRoot.push_back(make_pair("4", 344));  // Actors
  vecRoot.push_back(make_pair("5", 20348));  // Directors
  vecRoot.push_back(make_pair("6", 20388));  // Studios
  CVideoDatabase db;
  if (db.Open())
  {
    if (db.HasSets())
      vecRoot.push_back(make_pair("7", 20434));  // Sets
    db.Close();
  }
  vecRoot.push_back(make_pair("8", 20451));  // Countries

  CStdString path = BuildPath();
  for (unsigned int i = 0; i < vecRoot.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vecRoot[i].first + "/", true));
    pItem->SetLabel(g_localizeStrings.Get(vecRoot[i].second));
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
