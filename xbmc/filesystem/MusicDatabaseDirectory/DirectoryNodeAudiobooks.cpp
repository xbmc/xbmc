/*
 *      Copyright (C) 2014 Arne Morten Kvarving
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

#include "DirectoryNodeAudiobooks.h"
#include "QueryParams.h"
#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAudiobooks::CDirectoryNodeAudiobooks(const std::string& strName,
                                                   CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_AUDIOBOOKS, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAudiobooks::GetChildType() const
{
  return NODE_TYPE_SONG;
}

std::string CDirectoryNodeAudiobooks::GetLocalizedName() const
{
  return "";
}

bool CDirectoryNodeAudiobooks::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  bool bSuccess=musicdatabase.GetAudioBooks(items);

  musicdatabase.Close();

  return bSuccess;
}
