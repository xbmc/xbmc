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

#include "FTPDirectory.h"
#include "FTPParse.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "CurlFile.h"
#include "FileItem.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "climits"

using namespace XFILE;

CFTPDirectory::CFTPDirectory(void){}
CFTPDirectory::~CFTPDirectory(void){}

bool CFTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CCurlFile reader;

  CURL url(strPath);

  CStdString path = url.GetFileName();
  if( !path.IsEmpty() && !path.Right(1).Equals("/") )
  {
    path += "/";
    url.SetFileName(path);
  }

  if (!reader.Open(url))
    return false;


  char buffer[MAX_PATH + 1024];
  while( reader.ReadString(buffer, sizeof(buffer)) )
  {
    CStdString strBuffer = buffer;

    StringUtils::RemoveCRLF(strBuffer);

    CFTPParse parse;
    if (parse.FTPParse(strBuffer))
    {
      if( parse.getName().length() == 0 )
        continue;

      if( parse.getFlagtrycwd() == 0 && parse.getFlagtryretr() == 0 )
        continue;

      /* buffer name */
      CStdString name;
      name.assign(parse.getName());

      if( name.Equals("..") || name.Equals(".") )
        continue;

      /* this should be conditional if we ever add    */
      /* support for the utf8 extension in ftp client */
      g_charsetConverter.unknownToUTF8(name);

      CFileItemPtr pItem(new CFileItem(name));

      pItem->m_bIsFolder = (bool)(parse.getFlagtrycwd() != 0);
      CStdString filePath = path + name;
      if (pItem->m_bIsFolder)
        URIUtils::AddSlashAtEnd(filePath);

      /* qualify the url with host and all */
      url.SetFileName(filePath);
      pItem->SetPath(url.Get());

      pItem->m_dwSize = parse.getSize();
      pItem->m_dateTime=parse.getTime();

      items.Add(pItem);
    }
  }

  return true;
}

bool CFTPDirectory::Exists(const char* strPath)
{
  CCurlFile ftp;
  CURL url(strPath);
  return ftp.Exists(url);
}
