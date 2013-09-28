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

#include "DirectoryNodeContactRecentlyAdded.h"
#include "contacts/ContactDatabase.h"
#include "FileItem.h"

using namespace XFILE::CONTACTDATABASEDIRECTORY;

CDirectoryNodeContactRecentlyAdded::CDirectoryNodeContactRecentlyAdded(const CStdString& strName, CDirectoryNode* pParent)
: CDirectoryNode(NODE_TYPE_CONTACT_RECENTLY_ADDED, strName, pParent)
{
    
}

NODE_TYPE CDirectoryNodeContactRecentlyAdded::GetChildType() const
{
    if (GetName()=="-1")
        return NODE_TYPE_CONTACT_RECENTLY_ADDED_CONTACTS;
    
    return NODE_TYPE_CONTACT;
}

CStdString CDirectoryNodeContactRecentlyAdded::GetLocalizedName() const
{
    if (GetID() == -1)
        return g_localizeStrings.Get(15102); // All Contacts
    CContactDatabase db;
//    if (db.Open())
//        return db.GetContactById(GetID());
    return "";
}

bool CDirectoryNodeContactRecentlyAdded::GetContent(CFileItemList& items) const
{
    CContactDatabase contactdatabase;
    if (!contactdatabase.Open())
        return false;
  /*
    VECCONTACTS albums;
    if (!contactdatabase.GetRecentlyAddedContacts(albums))
    {
        contactdatabase.Close();
        return false;
    }
    
    for (int i=0; i<(int)albums.size(); ++i)
    {
        CContact& album=albums[i];
        CStdString strDir;
        strDir.Format("%s%ld/", BuildPath().c_str(), album.idContact);
        CFileItemPtr pItem(new CFileItem(strDir, album));
        items.Add(pItem);
    }
    */
    contactdatabase.Close();
    return true;
}
