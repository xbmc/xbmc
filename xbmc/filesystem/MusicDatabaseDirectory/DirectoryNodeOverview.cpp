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
#include "music/MusicDatabase.h"
#include "guilib/LocalizeStrings.h"

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
    Node OverviewChildren[] = {
                                { NODE_TYPE_GENRE,                 1, 135 },
                                { NODE_TYPE_ARTIST,                2, 133 },
                                { NODE_TYPE_ALBUM,                 3, 132 },
                                { NODE_TYPE_SINGLES,              10, 1050 },
                                { NODE_TYPE_SONG,                  4, 134 },
                                { NODE_TYPE_YEAR,                  9, 652 },
                                { NODE_TYPE_TOP100,                5, 271 },
                                { NODE_TYPE_ALBUM_RECENTLY_ADDED,  6, 359 },
                                { NODE_TYPE_ALBUM_RECENTLY_PLAYED, 7, 517 },
                                { NODE_TYPE_ALBUM_COMPILATIONS,    8, 521 },
                              };
  };
};

using namespace std;
using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetID() == OverviewChildren[i].id)
      return OverviewChildren[i].node;
  return NODE_TYPE_NONE;
}

CStdString CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetID() == OverviewChildren[i].id)
      return g_localizeStrings.Get(OverviewChildren[i].label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicDatabase;
  bool showSingles = false;
  if (musicDatabase.Open())
  {
    if (musicDatabase.GetSongsCount("where idAlbum in (select idAlbum from album where strAlbum='')") > 0)
      showSingles = true;
  }

  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
  {
    if (i == 3 && !showSingles) // singles
      continue;
    if (i == 9 && musicDatabase.GetVariousArtistsAlbumsCount() == 0) // compilations
      continue;

    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(OverviewChildren[i].label)));
    CStdString strDir;
    strDir.Format("%ld/", OverviewChildren[i].id);
    pItem->m_strPath = BuildPath() + strDir;
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
