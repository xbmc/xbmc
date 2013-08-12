/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "PictureInfoTagLoaderDatabase.h"
#include "pictures/PictureDatabase.h"
#include "filesystem/PictureDatabaseDirectory.h"
#include "filesystem/PictureDatabaseDirectory/DirectoryNode.h"
#include "PictureInfoTag.h"

using namespace PICTURE_INFO;

CPictureInfoTagLoaderDatabase::CPictureInfoTagLoaderDatabase(void)
{
}

CPictureInfoTagLoaderDatabase::~CPictureInfoTagLoaderDatabase()
{
}

bool CPictureInfoTagLoaderDatabase::Load(const CStdString& strFileName, CPictureInfoTag& tag, EmbeddedArt *art)
{
  tag.SetLoaded(false);
  CPictureDatabase database;
  database.Open();
  XFILE::PICTUREDATABASEDIRECTORY::CQueryParams param;
  XFILE::PICTUREDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strFileName,param);
  
  CPicture song;
  if (database.GetPicture(param.GetPictureId(),song))
    tag.SetPicture(song);
  
  database.Close();
  
  return tag.Loaded();
}

