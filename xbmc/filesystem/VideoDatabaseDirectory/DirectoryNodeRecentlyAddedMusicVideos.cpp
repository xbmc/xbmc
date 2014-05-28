/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDMUSICVIDEOS_H_INCLUDED
#define VIDEODATABASEDIRECTORY_DIRECTORYNODERECENTLYADDEDMUSICVIDEOS_H_INCLUDED
#include "DirectoryNodeRecentlyAddedMusicVideos.h"
#endif

#ifndef VIDEODATABASEDIRECTORY_VIDEO_VIDEODATABASE_H_INCLUDED
#define VIDEODATABASEDIRECTORY_VIDEO_VIDEODATABASE_H_INCLUDED
#include "video/VideoDatabase.h"
#endif


using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedMusicVideos::CDirectoryNodeRecentlyAddedMusicVideos(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_RECENTLY_ADDED_MUSICVIDEOS, strName, pParent)
{

}

bool CDirectoryNodeRecentlyAddedMusicVideos::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;
  
  bool bSuccess=videodatabase.GetRecentlyAddedMusicVideosNav(BuildPath(), items);

  videodatabase.Close();

  return bSuccess;
}

