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

#include "PictureDatabaseDirectory.h"
#include "utils/URIUtils.h"
#include "PictureDatabaseDirectory/QueryParams.h"
#include "pictures/PictureDatabase.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "utils/Crc32.h"
#include "guilib/TextureManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;
using namespace PICTUREDATABASEDIRECTORY;


#define _ENABLE_PICTURE_SCAN_

CPictureDatabaseDirectory::CPictureDatabaseDirectory(void)
{
}

CPictureDatabaseDirectory::~CPictureDatabaseDirectory(void)
{
}

bool CPictureDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    bool bResult = false;
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strPath);
    items.SetPath(path);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return false;
    
    bResult = pNode->GetChilds(items);
    for (int i=0;i<items.Size();++i)
    {
        CFileItemPtr item = items[i];
        if (item->m_bIsFolder && !item->HasIcon() && !item->HasArt("thumb"))
        {
            CStdString strImage = GetIcon(item->GetPath());
            if (!strImage.IsEmpty() && g_TextureManager.HasTexture(strImage))
                item->SetIconImage(strImage);
        }
    }
    items.SetLabel(pNode->GetLocalizedName());
#endif
    return bResult;
}

NODE_TYPE CPictureDatabaseDirectory::GetDirectoryChildType(const CStdString& strPath)
{
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return NODE_TYPE_NONE;
    
    return pNode->GetChildType();
#endif
    return NODE_TYPE_NONE;
}

NODE_TYPE CPictureDatabaseDirectory::GetDirectoryType(const CStdString& strPath)
{
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return NODE_TYPE_NONE;
    
    return pNode->GetType();
#endif
    return NODE_TYPE_NONE;
}

NODE_TYPE CPictureDatabaseDirectory::GetDirectoryParentType(const CStdString& strPath)
{
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return NODE_TYPE_NONE;
    
    CDirectoryNode* pParentNode=pNode->GetParent();
    
    if (!pParentNode)
        return NODE_TYPE_NONE;
    
    return pParentNode->GetChildType();
#endif
    return NODE_TYPE_NONE;
}

bool CPictureDatabaseDirectory::IsFaceDir(const CStdString& strDirectory)
{
    NODE_TYPE node=GetDirectoryType(strDirectory);
    return (node==NODE_TYPE_FACE);
}

bool CPictureDatabaseDirectory::HasAlbumInfo(const CStdString& strDirectory)
{
    NODE_TYPE node=GetDirectoryType(strDirectory);
    return (node!=NODE_TYPE_OVERVIEW && node!=NODE_TYPE_TOP100 &&
            node!=NODE_TYPE_LOCATION && node!=NODE_TYPE_FACE && node!=NODE_TYPE_YEAR);
}

void CPictureDatabaseDirectory::ClearDirectoryCache(const CStdString& strDirectory)
{
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strDirectory);
    URIUtils::RemoveSlashAtEnd(path);
    
    Crc32 crc;
    crc.ComputeFromLowerCase(path);
    
    CStdString strFileName;
    strFileName.Format("special://temp/%08x.fi", (unsigned __int32) crc);
    CFile::Delete(strFileName);
#endif
}

bool CPictureDatabaseDirectory::IsAllItem(const CStdString& strDirectory)
{
    if (strDirectory.Right(4).Equals("/-1/"))
        return true;
    return false;
}

bool CPictureDatabaseDirectory::GetLabel(const CStdString& strDirectory, CStdString& strLabel)
{
    strLabel = "";
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strDirectory);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    if (!pNode.get())
        return false;
    
    // first see if there's any filter criteria
    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(path, params);
    
    CPictureDatabase picturedatabase;
    if (!picturedatabase.Open())
        return false;
    
    // get genre
    if (params.GetLocationId() >= 0)
        strLabel += picturedatabase.GetLocationById(params.GetLocationId());
    
    // get artist
    if (params.GetFaceId() >= 0)
    {
        if (!strLabel.IsEmpty())
            strLabel += " / ";
        strLabel += picturedatabase.GetFaceById(params.GetFaceId());
    }
    
    // get album
    if (params.GetPictureAlbumId() >= 0)
    {
        if (!strLabel.IsEmpty())
            strLabel += " / ";
        strLabel += picturedatabase.GetPictureAlbumById(params.GetPictureAlbumId());
    }
    
    if (strLabel.IsEmpty())
    {
        switch (pNode->GetChildType())
        {
            case NODE_TYPE_TOP100:
                strLabel = g_localizeStrings.Get(271); // Top 100
                break;
            case NODE_TYPE_LOCATION:
                strLabel = g_localizeStrings.Get(135); // Locations
                break;
            case NODE_TYPE_FACE:
                strLabel = g_localizeStrings.Get(133); // Faces
                break;
            case NODE_TYPE_ALBUM:
                strLabel = g_localizeStrings.Get(132); // Albums
                break;
            case NODE_TYPE_ALBUM_RECENTLY_ADDED:
            case NODE_TYPE_ALBUM_RECENTLY_ADDED_PICTURES:
                strLabel = g_localizeStrings.Get(359); // Recently Added Albums
                break;
            case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
            case NODE_TYPE_ALBUM_RECENTLY_PLAYED_PICTURES:
                strLabel = g_localizeStrings.Get(517); // Recently Played Albums
                break;
            case NODE_TYPE_ALBUM_TOP100:
            case NODE_TYPE_ALBUM_TOP100_PICTURES:
                strLabel = g_localizeStrings.Get(10505); // Top 100 Albums
                break;
            case NODE_TYPE_SINGLES:
                strLabel = g_localizeStrings.Get(1050); // Singles
                break;
            case NODE_TYPE_PICTURE:
                strLabel = g_localizeStrings.Get(134); // Pictures
                break;
            case NODE_TYPE_PICTURE_TOP100:
                strLabel = g_localizeStrings.Get(10504); // Top 100 Pictures
                break;
            case NODE_TYPE_YEAR:
            case NODE_TYPE_YEAR_ALBUM:
            case NODE_TYPE_YEAR_PICTURE:
                strLabel = g_localizeStrings.Get(652);  // Years
                break;
            case NODE_TYPE_ALBUM_COMPILATIONS:
            case NODE_TYPE_ALBUM_COMPILATIONS_PICTURES:
                strLabel = g_localizeStrings.Get(521);
                break;
            case NODE_TYPE_OVERVIEW:
                strLabel = "";
                break;
            default:
                CLog::Log(LOGWARNING, "%s - Unknown nodetype requested %d", __FUNCTION__, pNode->GetChildType());
                return false;
        }
    }
#endif
    return true;
}

bool CPictureDatabaseDirectory::ContainsPictures(const CStdString &path)
{
    PICTUREDATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_PICTURE) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_SINGLES) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_ADDED_PICTURES) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_PLAYED_PICTURES) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_ALBUM_COMPILATIONS_PICTURES) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_ALBUM_TOP100_PICTURES) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_PICTURE_TOP100) return true;
    if (type == PICTUREDATABASEDIRECTORY::NODE_TYPE_YEAR_PICTURE) return true;
    return false;
}

bool CPictureDatabaseDirectory::Exists(const char* strPath)
{
#ifdef _ENABLE_PICTURE_SCAN_
    
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return false;
    
    if (pNode->GetChildType() == PICTUREDATABASEDIRECTORY::NODE_TYPE_NONE)
        return false;
#endif
    return true;
}

bool CPictureDatabaseDirectory::CanCache(const CStdString& strPath)
{
#ifdef _ENABLE_PICTURE_SCAN_
    CStdString path = CLegacyPathTranslation::TranslatePictureDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    if (!pNode.get())
        return false;
    return pNode->CanCache();
#endif
    return false;
}

CStdString CPictureDatabaseDirectory::GetIcon(const CStdString &strDirectory)
{
    switch (GetDirectoryChildType(strDirectory))
    {
        case NODE_TYPE_FACE:
            return "DefaultPictureFaces.png";
        case NODE_TYPE_LOCATION:
            return "DefaultPictureLocations.png";
        case NODE_TYPE_TOP100:
            return "DefaultPictureTop100.png";
        case NODE_TYPE_ALBUM:
        case NODE_TYPE_YEAR_ALBUM:
            return "DefaultPictureAlbums.png";
        case NODE_TYPE_ALBUM_RECENTLY_ADDED:
        case NODE_TYPE_ALBUM_RECENTLY_ADDED_PICTURES:
            return "DefaultPictureRecentlyAdded.png";
        case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
        case NODE_TYPE_ALBUM_RECENTLY_PLAYED_PICTURES:
            return "DefaultPictureRecentlyPlayed.png";
        case NODE_TYPE_SINGLES:
        case NODE_TYPE_PICTURE:
        case NODE_TYPE_YEAR_PICTURE:
        case NODE_TYPE_ALBUM_COMPILATIONS_PICTURES:
            return "DefaultPicturePictures.png";
        case NODE_TYPE_ALBUM_TOP100:
        case NODE_TYPE_ALBUM_TOP100_PICTURES:
            return "DefaultPictureTop100Albums.png";
        case NODE_TYPE_PICTURE_TOP100:
            return "DefaultPictureTop100Pictures.png";
        case NODE_TYPE_YEAR:
            return "DefaultPictureYears.png";
        case NODE_TYPE_ALBUM_COMPILATIONS:
            return "DefaultPictureCompilations.png";
        default:
            CLog::Log(LOGWARNING, "%s - Unknown nodetype requested %s", __FUNCTION__, strDirectory.c_str());
            break;
    }
    
    return "";
}
