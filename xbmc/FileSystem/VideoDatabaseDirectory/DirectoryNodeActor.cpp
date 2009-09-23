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

#include "DirectoryNodeActor.h"
#include "QueryParams.h"
#include "VideoDatabase.h"

using namespace DIRECTORY::VIDEODATABASEDIRECTORY;

CDirectoryNodeActor::CDirectoryNodeActor(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ACTOR, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeActor::GetChildType()
{
  CQueryParams params;
  CollectQueryParams(params);
  if (params.GetContentType() == VIDEODB_CONTENT_MOVIES)
    return NODE_TYPE_TITLE_MOVIES;
  if (params.GetContentType() == VIDEODB_CONTENT_MUSICVIDEOS)
    return NODE_TYPE_MUSICVIDEOS_ALBUM;

  return NODE_TYPE_TITLE_TVSHOWS;
}

bool CDirectoryNodeActor::GetContent(CFileItemList& items)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetActorsNav(BuildPath(), items, params.GetContentType());

  videodatabase.Close();

  return bSuccess;
}

