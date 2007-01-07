#include "stdafx.h"
#include "MusicInfoTagLoaderDatabase.h"
#include "MusicDatabase.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "Util.h"

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