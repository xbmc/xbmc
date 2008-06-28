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
#include "DirectoryNodeSeasons.h"
#include "QueryParams.h"
#include "VideoDatabase.h"
#include "GUISettings.h"
#include "FileItem.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeSeasons::CDirectoryNodeSeasons(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SEASONS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeSeasons::GetChildType()
{
  return NODE_TYPE_EPISODES;
}

bool CDirectoryNodeSeasons::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetSeasonsNav(BuildPath(), items, params.GetActorId(), params.GetDirectorId(), params.GetGenreId(), params.GetYear(), params.GetTvShowId());
  if (items.GetObjectCount() == 1 && g_guiSettings.GetBool("videolibrary.singleseason"))
  {
    items.Clear();
    bSuccess=videodatabase.GetEpisodesNav(BuildPath()+"-1/",items,params.GetGenreId(),params.GetYear(),params.GetActorId(),params.GetDirectorId(),params.GetTvShowId());
    items.m_strPath = BuildPath()+"-1/";
  }

  videodatabase.Close();

  return bSuccess;
}
