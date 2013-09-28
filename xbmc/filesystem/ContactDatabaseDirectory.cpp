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

#include "ContactDatabaseDirectory.h"
#include "utils/URIUtils.h"
#include "ContactDatabaseDirectory/QueryParams.h"
#include "contacts/ContactDatabase.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "utils/Crc32.h"
#include "guilib/TextureManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"

using namespace std;
using namespace XFILE;
using namespace CONTACTDATABASEDIRECTORY;


#define _ENABLE_CONTACT_SCAN_

CContactDatabaseDirectory::CContactDatabaseDirectory(void)
{
}

CContactDatabaseDirectory::~CContactDatabaseDirectory(void)
{
}

bool CContactDatabaseDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
    bool bResult = false;
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strPath);
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

NODE_TYPE CContactDatabaseDirectory::GetDirectoryChildType(const CStdString& strPath)
{
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return NODE_TYPE_NONE;
    
    return pNode->GetChildType();
#endif
    return NODE_TYPE_NONE;
}

NODE_TYPE CContactDatabaseDirectory::GetDirectoryType(const CStdString& strPath)
{
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return NODE_TYPE_NONE;
    
    return pNode->GetType();
#endif
    return NODE_TYPE_NONE;
}

NODE_TYPE CContactDatabaseDirectory::GetDirectoryParentType(const CStdString& strPath)
{
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strPath);
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

bool CContactDatabaseDirectory::IsFaceDir(const CStdString& strDirectory)
{
    NODE_TYPE node=GetDirectoryType(strDirectory);
    return (node==NODE_TYPE_FACE);
}

bool CContactDatabaseDirectory::HasAlbumInfo(const CStdString& strDirectory)
{
    NODE_TYPE node=GetDirectoryType(strDirectory);
    return (node!=NODE_TYPE_OVERVIEW && node!=NODE_TYPE_TOP100 &&
            node!=NODE_TYPE_LOCATION && node!=NODE_TYPE_FACE );
}

void CContactDatabaseDirectory::ClearDirectoryCache(const CStdString& strDirectory)
{
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strDirectory);
    URIUtils::RemoveSlashAtEnd(path);
    
    Crc32 crc;
    crc.ComputeFromLowerCase(path);
    
    CStdString strFileName;
    strFileName.Format("special://temp/%08x.fi", (unsigned __int32) crc);
    CFile::Delete(strFileName);
#endif
}

bool CContactDatabaseDirectory::IsAllItem(const CStdString& strDirectory)
{
    if (strDirectory.Right(4).Equals("/-1/"))
        return true;
    return false;
}

bool CContactDatabaseDirectory::GetLabel(const CStdString& strDirectory, CStdString& strLabel)
{
    strLabel = "";
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strDirectory);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    if (!pNode.get())
        return false;
    
    // first see if there's any filter criteria
    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(path, params);
    
    CContactDatabase contactdatabase;
    if (!contactdatabase.Open())
        return false;
    
    // get genre
//    if (params.GetLocationId() >= 0)
//        strLabel += contactdatabase.GetLocationById(params.GetLocationId());
    
    // get artist
    if (params.GetFaceId() >= 0)
    {
        if (!strLabel.IsEmpty())
            strLabel += " / ";
//        strLabel += contactdatabase.GetFaceById(params.GetFaceId());
    }
    
    // get album
    if (params.GetContactAlbumId() >= 0)
    {
        if (!strLabel.IsEmpty())
            strLabel += " / ";
  //      strLabel += contactdatabase.GetContactAlbumById(params.GetContactAlbumId());
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
            case NODE_TYPE_CONTACT_RECENTLY_ADDED:
            case NODE_TYPE_CONTACT_RECENTLY_ADDED_CONTACTS:
                strLabel = g_localizeStrings.Get(359); // Recently Added Albums
                break;
            case NODE_TYPE_CONTACT:
                strLabel = g_localizeStrings.Get(134); // Contacts
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

bool CContactDatabaseDirectory::ContainsContacts(const CStdString &path)
{
    CONTACTDATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
    if (type == CONTACTDATABASEDIRECTORY::NODE_TYPE_CONTACT) return true;
    if (type == CONTACTDATABASEDIRECTORY::NODE_TYPE_CONTACT_RECENTLY_ADDED_CONTACTS) return true;
    return false;
}

bool CContactDatabaseDirectory::Exists(const char* strPath)
{
#ifdef _ENABLE_CONTACT_SCAN_
    
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    
    if (!pNode.get())
        return false;
    
    if (pNode->GetChildType() == CONTACTDATABASEDIRECTORY::NODE_TYPE_NONE)
        return false;
#endif
    return true;
}

bool CContactDatabaseDirectory::CanCache(const CStdString& strPath)
{
#ifdef _ENABLE_CONTACT_SCAN_
    CStdString path = CLegacyPathTranslation::TranslateContactDbPath(strPath);
    auto_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
    if (!pNode.get())
        return false;
    return pNode->CanCache();
#endif
    return false;
}

CStdString CContactDatabaseDirectory::GetIcon(const CStdString &strDirectory)
{
    switch (GetDirectoryChildType(strDirectory))
    {
        case NODE_TYPE_FACE:
            return "DefaultContactFaces.png";
        case NODE_TYPE_LOCATION:
            return "DefaultContactLocations.png";
        case NODE_TYPE_TOP100:
            return "DefaultContactTop100.png";
        case NODE_TYPE_CONTACT:
            return "DefaultContactAlbums.png";
        case NODE_TYPE_CONTACT_RECENTLY_ADDED:
        case NODE_TYPE_CONTACT_RECENTLY_ADDED_CONTACTS:
            return "DefaultContactRecentlyAdded.png";
        default:
            CLog::Log(LOGWARNING, "%s - Unknown nodetype requested %s", __FUNCTION__, strDirectory.c_str());
            break;
    }
    
    return "";
}
