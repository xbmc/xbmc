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

#include "DirectoryNodeMusicVideosOverview.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node MusicVideoChildren[] = {
                              { NODE_TYPE_GENRE,             1, 135 },
                              { NODE_TYPE_TITLE_MUSICVIDEOS, 2, 369 },
                              { NODE_TYPE_YEAR,              3, 562 },
                              { NODE_TYPE_ACTOR,             4, 133 },
                              { NODE_TYPE_MUSICVIDEOS_ALBUM, 5, 132 },
                              { NODE_TYPE_DIRECTOR,          6, 20348 },
                              { NODE_TYPE_STUDIO,            7, 20388 },
                              { NODE_TYPE_TAGS,              9, 20459 }
                            };

CDirectoryNodeMusicVideosOverview::CDirectoryNodeMusicVideosOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MUSICVIDEOS_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMusicVideosOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(MusicVideoChildren) / sizeof(Node); ++i)
    if (GetID() == MusicVideoChildren[i].id)
      return MusicVideoChildren[i].node;

  return NODE_TYPE_NONE;
}

CStdString CDirectoryNodeMusicVideosOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(MusicVideoChildren) / sizeof(Node); ++i)
    if (GetID() == MusicVideoChildren[i].id)
      return g_localizeStrings.Get(MusicVideoChildren[i].label);
  return "";
}

bool CDirectoryNodeMusicVideosOverview::GetContent(CFileItemList& items) const
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;
  
  for (unsigned int i = 0; i < sizeof(MusicVideoChildren) / sizeof(Node); ++i)
  {
    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(MusicVideoChildren[i].label)));

    CVideoDbUrl itemUrl = videoUrl;
    CStdString strDir; strDir.Format("%ld/", MusicVideoChildren[i].id);
    itemUrl.AppendPath(strDir);
    pItem->SetPath(itemUrl.ToString());

    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}

