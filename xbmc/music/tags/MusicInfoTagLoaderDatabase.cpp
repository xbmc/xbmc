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

#include "MusicInfoTagLoaderDatabase.h"
#include "music/MusicDatabase.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "MusicInfoTag.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderDatabase::CMusicInfoTagLoaderDatabase(void)
{
}

CMusicInfoTagLoaderDatabase::~CMusicInfoTagLoaderDatabase()
{
}

bool CMusicInfoTagLoaderDatabase::Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
{
  tag.SetLoaded(false);
  CMusicDatabase database;
  database.Open();
  XFILE::MUSICDATABASEDIRECTORY::CQueryParams param;
  XFILE::MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strFileName,param);

  CSong song;
  if (database.GetSong(param.GetSongId(),song))
    tag.SetSong(song);

  database.Close();

  return tag.Loaded();
}

