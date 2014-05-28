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

#ifndef PLAYLISTS_PLAYLISTURL_H_INCLUDED
#define PLAYLISTS_PLAYLISTURL_H_INCLUDED
#include "PlayListURL.h"
#endif

#ifndef PLAYLISTS_FILESYSTEM_FILE_H_INCLUDED
#define PLAYLISTS_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif

#ifndef PLAYLISTS_UTILS_URIUTILS_H_INCLUDED
#define PLAYLISTS_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef PLAYLISTS_UTILS_STRINGUTILS_H_INCLUDED
#define PLAYLISTS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif


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

  m_strPlayListName = URIUtils::GetFileName(strFileName);
  URIUtils::GetParentPath(strFileName, m_strBasePath);

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

    if (StringUtils::StartsWith(strLine, "[InternetShortcut]"))
    {
      if (file.ReadString(szLine,1024))
      {
        strLine  = szLine;
        StringUtils::RemoveCRLF(strLine);
        if (StringUtils::StartsWith(strLine, "URL="))
        {
          CFileItemPtr newItem(new CFileItem(strLine.substr(4), false));
          Add(newItem);
        }
      }
    }
  }

  file.Close();
  return true;
}

