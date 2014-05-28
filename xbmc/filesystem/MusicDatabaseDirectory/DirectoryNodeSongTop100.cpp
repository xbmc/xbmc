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

#ifndef MUSICDATABASEDIRECTORY_DIRECTORYNODESONGTOP100_H_INCLUDED
#define MUSICDATABASEDIRECTORY_DIRECTORYNODESONGTOP100_H_INCLUDED
#include "DirectoryNodeSongTop100.h"
#endif

#ifndef MUSICDATABASEDIRECTORY_MUSIC_MUSICDATABASE_H_INCLUDED
#define MUSICDATABASEDIRECTORY_MUSIC_MUSICDATABASE_H_INCLUDED
#include "music/MusicDatabase.h"
#endif


using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeSongTop100::CDirectoryNodeSongTop100(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SONG_TOP100, strName, pParent)
{

}

bool CDirectoryNodeSongTop100::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CStdString strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetTop100(strBaseDir, items);

  musicdatabase.Close();

  return bSuccess;
}
