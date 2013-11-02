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

#include "DirectoryNodeAlbumRecentlyAddedPicture.h"
#include "pictures/PictureDatabase.h"

using namespace XFILE::PICTUREDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyAddedPicture::CDirectoryNodeAlbumRecentlyAddedPicture(const CStdString& strName, CDirectoryNode* pParent)
: CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_ADDED_PICTURES, strName, pParent)
{
    
}

bool CDirectoryNodeAlbumRecentlyAddedPicture::GetContent(CFileItemList& items) const
{
    CPictureDatabase picturedatabase;
    if (!picturedatabase.Open())
        return false;
    
    CStdString strBaseDir=BuildPath();
    CStdString pictureType = "Picture";
    bool bSuccess=picturedatabase.GetRecentlyPlayedPictureAlbumPictures(strBaseDir, items, pictureType);
    
    picturedatabase.Close();
    
    return bSuccess;
}
