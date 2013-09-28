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

#include "DirectoryNodeContact.h"
#include "QueryParams.h"
#include "contacts/ContactDatabase.h"

using namespace XFILE::CONTACTDATABASEDIRECTORY;

CDirectoryNodeContact::CDirectoryNodeContact(const CStdString& strName, CDirectoryNode* pParent)
: CDirectoryNode(NODE_TYPE_CONTACT, strName, pParent)
{
    
}

bool CDirectoryNodeContact::GetContent(CFileItemList& items) const
{
    CContactDatabase contactdatabase;
    if (!contactdatabase.Open())
        return false;
    
    CQueryParams params;
    CollectQueryParams(params);
    
    CStdString strBaseDir=BuildPath();
//    bool bSuccess=contactdatabase.GetContactsNav(strBaseDir, items, params.GetLocationId(), params.GetFaceId(), params.GetContactAlbumId());
    
    contactdatabase.Close();
  
  return false;
//    return bSuccess;
}
