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

#include "ContactInfoTagLoaderDatabase.h"
#include "contacts/ContactDatabase.h"
#include "filesystem/ContactDatabaseDirectory.h"
#include "filesystem/ContactDatabaseDirectory/DirectoryNode.h"
#include "ContactInfoTag.h"

using namespace CONTACT_INFO;

CContactInfoTagLoaderDatabase::CContactInfoTagLoaderDatabase(void)
{
}

CContactInfoTagLoaderDatabase::~CContactInfoTagLoaderDatabase()
{
}

bool CContactInfoTagLoaderDatabase::Load(const CStdString& strFileName, CContactInfoTag& tag)
{
  tag.SetLoaded(false);
  CContactDatabase database;
  database.Open();
  XFILE::CONTACTDATABASEDIRECTORY::CQueryParams param;
  XFILE::CONTACTDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(strFileName,param);
  
  CContact contact;
  if (database.GetContact(param.GetContactId(),contact))
    tag.SetContact(contact);
  
  database.Close();
  
  return tag.Loaded();
}

