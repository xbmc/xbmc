/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DirectoryNodeSingles.h"
#include "QueryParams.h"
#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeSingles::CDirectoryNodeSingles(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_SINGLES, strName, pParent)
{

}

bool CDirectoryNodeSingles::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CStdString strBaseDir=BuildPath();
  bool bSuccess=musicdatabase.GetSongsByWhere(strBaseDir, "where idAlbum in (select idAlbum from album where strAlbum='')", items);

  musicdatabase.Close();

  return bSuccess;
}
