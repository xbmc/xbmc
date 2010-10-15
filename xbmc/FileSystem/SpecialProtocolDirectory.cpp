/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SpecialProtocolDirectory.h"
#include "SpecialProtocol.h"
#include "Directory.h"
#include "Util.h"
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
  if (CDirectory::GetDirectory(translatedPath, items, m_strFileMask, m_useFileDirectories, m_allowPrompting, m_cacheDirectory, m_extFileInfo, false, true))
  { // replace our paths as necessary
    items.m_strPath = untranslatedPath;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      if (strnicmp(item->m_strPath.c_str(), translatedPath.c_str(), translatedPath.GetLength()) == 0)
        item->m_strPath = CUtil::AddFileToFolder(untranslatedPath, item->m_strPath.Mid(translatedPath.GetLength()));
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
