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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SpecialProtocolDirectory.h"
#include "SpecialProtocol.h"
#include "Directory.h"
#include "utils/URIUtils.h"
#include "FileItem.h"

using namespace XFILE;

CSpecialProtocolDirectory::CSpecialProtocolDirectory(void)
{
}

CSpecialProtocolDirectory::~CSpecialProtocolDirectory(void)
{
}

bool CSpecialProtocolDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString untranslatedPath = strPath;  // Why do I need a copy??? - the GetDirectory() call below will override strPath???
  CStdString translatedPath = CSpecialProtocol::TranslatePath(strPath);
  if (CDirectory::GetDirectory(translatedPath, items, m_strFileMask, m_flags | DIR_FLAG_GET_HIDDEN))
  { // replace our paths as necessary
    items.SetPath(untranslatedPath);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (strnicmp(item->GetPath().c_str(), translatedPath.c_str(), translatedPath.GetLength()) == 0)
        item->SetPath(URIUtils::AddFileToFolder(untranslatedPath, item->GetPath().Mid(translatedPath.GetLength())));
    }
    return true;
  }
  return false;
}

bool CSpecialProtocolDirectory::Create(const char* strPath)
{
  CStdString translatedPath = CSpecialProtocol::TranslatePath(strPath);
  return CDirectory::Create(translatedPath.c_str());
}

bool CSpecialProtocolDirectory::Remove(const char* strPath)
{
  CStdString translatedPath = CSpecialProtocol::TranslatePath(strPath);
  return CDirectory::Remove(translatedPath.c_str());
}

bool CSpecialProtocolDirectory::Exists(const char* strPath)
{
  CStdString translatedPath = CSpecialProtocol::TranslatePath(strPath);
  return CDirectory::Exists(translatedPath.c_str());
}
