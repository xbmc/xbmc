/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef FILESYSTEM_SPECIALPROTOCOLDIRECTORY_H_INCLUDED
#define FILESYSTEM_SPECIALPROTOCOLDIRECTORY_H_INCLUDED
#include "SpecialProtocolDirectory.h"
#endif

#ifndef FILESYSTEM_SPECIALPROTOCOL_H_INCLUDED
#define FILESYSTEM_SPECIALPROTOCOL_H_INCLUDED
#include "SpecialProtocol.h"
#endif

#ifndef FILESYSTEM_DIRECTORY_H_INCLUDED
#define FILESYSTEM_DIRECTORY_H_INCLUDED
#include "Directory.h"
#endif

#ifndef FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif


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
      if (StringUtils::StartsWith(item->GetPath(), translatedPath))
        item->SetPath(URIUtils::AddFileToFolder(untranslatedPath, item->GetPath().substr(translatedPath.size())));
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
