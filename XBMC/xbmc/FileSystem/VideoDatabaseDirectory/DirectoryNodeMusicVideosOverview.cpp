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

#include "DirectoryNodeMusicVideosOverview.h"
#include "FileItem.h"
#include "LocalizeStrings.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeMusicVideosOverview::CDirectoryNodeMusicVideosOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MUSICVIDEOS_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMusicVideosOverview::GetChildType()
{
  if (GetName()=="1")
    return NODE_TYPE_GENRE;
  else if (GetName()=="2")
    return NODE_TYPE_TITLE_MUSICVIDEOS;
  else if (GetName()=="3")
    return NODE_TYPE_YEAR;
  else if (GetName()=="4")
    return NODE_TYPE_ACTOR;
  else if (GetName()=="5")
    return NODE_TYPE_MUSICVIDEOS_ALBUM;
  else if (GetName()=="6")
    return NODE_TYPE_DIRECTOR;
  else if (GetName()=="7")
    return NODE_TYPE_STUDIO;

  return NODE_TYPE_NONE;
}

bool CDirectoryNodeMusicVideosOverview::GetContent(CFileItemList& items)
{
  CStdStringArray vecRoot;
  vecRoot.push_back(g_localizeStrings.Get(135));  // Genres
  vecRoot.push_back(g_localizeStrings.Get(369));  // Title
  vecRoot.push_back(g_localizeStrings.Get(562));  // Year
  vecRoot.push_back(g_localizeStrings.Get(133));  // Artists
  vecRoot.push_back(g_localizeStrings.Get(132));  // Albums
  vecRoot.push_back(g_localizeStrings.Get(20348));  // Directors
  vecRoot.push_back(g_localizeStrings.Get(20388));  // Studios

  for (int i = 0; i < (int)vecRoot.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(vecRoot[i]));
    CStdString strDir;
    strDir.Format("%i/", i+1);
    pItem->m_strPath = BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}

