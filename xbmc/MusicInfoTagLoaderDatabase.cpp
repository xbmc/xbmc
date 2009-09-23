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

#include "MusicInfoTagLoaderDatabase.h"
#include "MusicDatabase.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "MusicInfoTag.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderDatabase::CMusicInfoTagLoaderDatabase(void)
{
}

CMusicInfoTagLoaderDatabase::~CMusicInfoTagLoaderDatabase()
{
}

bool CMusicInfoTagLoaderDatabase::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
  tag.SetLoaded(false);
  CMusicDatabase database;
  database.Open();
  DIRECTORY::MUSICDATABASEDIRECTORY::CQueryParams param;
  DIRECTORY::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strFileName,param);

  CSong song;
  if (database.GetSongById(param.GetSongId(),song))
    tag.SetSong(song);

  database.Close();

  return tag.Loaded();
}

