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

#ifndef TAGS_MUSICINFOTAGLOADERDATABASE_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERDATABASE_H_INCLUDED
#include "MusicInfoTagLoaderDatabase.h"
#endif

#ifndef TAGS_MUSIC_MUSICDATABASE_H_INCLUDED
#define TAGS_MUSIC_MUSICDATABASE_H_INCLUDED
#include "music/MusicDatabase.h"
#endif

#ifndef TAGS_FILESYSTEM_MUSICDATABASEDIRECTORY_H_INCLUDED
#define TAGS_FILESYSTEM_MUSICDATABASEDIRECTORY_H_INCLUDED
#include "filesystem/MusicDatabaseDirectory.h"
#endif

#ifndef TAGS_FILESYSTEM_MUSICDATABASEDIRECTORY_DIRECTORYNODE_H_INCLUDED
#define TAGS_FILESYSTEM_MUSICDATABASEDIRECTORY_DIRECTORYNODE_H_INCLUDED
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#endif

#ifndef TAGS_MUSICINFOTAG_H_INCLUDED
#define TAGS_MUSICINFOTAG_H_INCLUDED
#include "MusicInfoTag.h"
#endif


using namespace MUSIC_INFO;

CMusicInfoTagLoaderDatabase::CMusicInfoTagLoaderDatabase(void)
{
}

CMusicInfoTagLoaderDatabase::~CMusicInfoTagLoaderDatabase()
{
}

bool CMusicInfoTagLoaderDatabase::Load(const CStdString& strFileName, CMusicInfoTag& tag, EmbeddedArt *art)
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

