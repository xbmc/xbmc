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

#include "DirectoryNodeFace.h"
#include "QueryParams.h"
#include "contacts/ContactDatabase.h"
#include "settings/Settings.h"

using namespace XFILE::CONTACTDATABASEDIRECTORY;

CDirectoryNodeFace::CDirectoryNodeFace(const CStdString& strName, CDirectoryNode* pParent)
: CDirectoryNode(NODE_TYPE_FACE, strName, pParent)
{
    
}

NODE_TYPE CDirectoryNodeFace::GetChildType() const
{
    return NODE_TYPE_CONTACT;
}

CStdString CDirectoryNodeFace::GetLocalizedName() const
{
    if (GetID() == -1)
        return g_localizeStrings.Get(15103); // All Faces
    CContactDatabase db;
//    if (db.Open())
//        return db.GetFaceById(GetID());
    return "";
}

bool CDirectoryNodeFace::GetContent(CFileItemList& items) const
{
    CContactDatabase contactdatabase;
    if (!contactdatabase.Open())
        return false;
    
    CQueryParams params;
    CollectQueryParams(params);
    
//    bool bSuccess = contactdatabase.GetFacesNav(BuildPath(), items, !CSettings::Get().GetBool("contactlibrary.showcompilationartists"), params.GetLocationId());
    
    contactdatabase.Close();
  return false;
  //  return bSuccess;
}
