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

#include "DirectoryNodeAlbum.h"
#include "QueryParams.h"
#include "pictures/PictureDatabase.h"

using namespace XFILE::PICTUREDATABASEDIRECTORY;

CDirectoryNodeAlbum::CDirectoryNodeAlbum(const CStdString& strName, CDirectoryNode* pParent)
: CDirectoryNode(NODE_TYPE_ALBUM, strName, pParent)
{
    
}

NODE_TYPE CDirectoryNodeAlbum::GetChildType() const
{
    return NODE_TYPE_PICTURE;
}

CStdString CDirectoryNodeAlbum::GetLocalizedName() const
{
    if (GetID() == -1)
        return g_localizeStrings.Get(15102); // All Albums
    CPictureDatabase db;
    if (db.Open())
        return db.GetPictureAlbumById(GetID());
    return "";
}

bool CDirectoryNodeAlbum::GetContent(CFileItemList& items) const
{
    CPictureDatabase picturedatabase;
    if (!picturedatabase.Open())
        return false;
    
    CQueryParams params;
    CollectQueryParams(params);
    
    bool bSuccess=picturedatabase.GetPictureAlbumsNav(BuildPath(), items, params.GetLocationId(), params.GetFaceId());
    
    picturedatabase.Close();
    
    return bSuccess;
}
