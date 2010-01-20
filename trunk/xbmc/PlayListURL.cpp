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

#include "PlayListURL.h"
#include "FileSystem/File.h"
#include "Util.h"
#include "StringUtils.h"

using namespace PLAYLIST;
using namespace XFILE;

// example url file
//[DEFAULT]
//BASEURL=http://msdn2.microsoft.com/en-us/library/ms812698.aspx
//[InternetShortcut]
//URL=http://msdn2.microsoft.com/en-us/library/ms812698.aspx

CPlayListURL::CPlayListURL(void)
{}

CPlayListURL::~CPlayListURL(void)
{}

bool CPlayListURL::Load(const CStdString& strFileName)
{
  char szLine[4096];
  CStdString strLine;

  Clear();

  m_strPlayListName = CUtil::GetFileName(strFileName);
  CUtil::GetParentPath(strFileName, m_strBasePath);

  CFile file;
  if (!file.Open(strFileName) )
  {
    file.Close();
    return false;
  }

  while (file.ReadString(szLine, 1024))
  {
    strLine = szLine;
    StringUtils::RemoveCRLF(strLine);

    if (strLine.Left(18) == "[InternetShortcut]")
    {
      file.ReadString(szLine,1024);
      strLine  = szLine;
      StringUtils::RemoveCRLF(strLine);
      if (strLine.Left(4) == "URL=")
      {
        CFileItemPtr newItem(new CFileItem(strLine.Mid(4),false));
        Add(newItem);
      }
    }
  }

  file.Close();
  return true;
}

