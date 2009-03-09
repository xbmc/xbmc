/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "stdafx.h"
#include "HTTPDirectory.h"
#include "URL.h"
#include "Util.h"
#include "FileCurl.h"
#include "FileItem.h"
#include "utils/RegExp.h"
#include "Settings.h"

using namespace XFILE;
using namespace DIRECTORY;

CHTTPDirectory::CHTTPDirectory(void){}
CHTTPDirectory::~CHTTPDirectory(void){}

bool CHTTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CFileCurl http;
  CURL url(strPath);

  CStdString strName, strLink;
  CStdString strBasePath = url.GetFileName();

  if(!http.Open(url, false))
  {
    CLog::Log(LOGERROR, "%s - Unable to get http directory", __FUNCTION__);
    return false;
  }

  CRegExp reItem;
  reItem.RegComp("<a href=\"(.*)\">(.*)</a>");

  /* read response from server into string buffer */
  char buffer[MAX_PATH + 1024];
  while(http.ReadString(buffer, sizeof(buffer)-1))
  {
    CStdString strBuffer = buffer;
    StringUtils::RemoveCRLF(strBuffer);

    if (reItem.RegFind(strBuffer.c_str()) >= 0)
    {
      strLink = reItem.GetReplaceString("\\1");
      strName = reItem.GetReplaceString("\\2");

      if(strLink[0] == '/')
        strLink = strLink.Mid(1);

      CStdString strNameTemp = strName.Trim();
      CStdString strLinkTemp = strLink;
      CUtil::RemoveSlashAtEnd(strLinkTemp);
      CUtil::RemoveSlashAtEnd(strNameTemp);
      CUtil::UrlDecode(strLinkTemp);

      if (strNameTemp == strLinkTemp)
      {
        g_charsetConverter.stringCharsetToUtf8(strName);
        CUtil::RemoveSlashAtEnd(strName);

        CFileItemPtr pItem(new CFileItem(strName));
        pItem->m_strPath = strBasePath + strLink;

        if(CUtil::HasSlashAtEnd(pItem->m_strPath))
          pItem->m_bIsFolder = true;

        url.SetFileName(pItem->m_strPath);
        url.GetURL(pItem->m_strPath);

        if (!pItem->m_bIsFolder && g_advancedSettings.m_bHTTPDirectoryStatFilesize)
        {
          CFileCurl file;
          file.Open(url, false);
          pItem->m_dwSize= file.GetLength();
          file.Close();
        }

        if (!pItem->m_bIsFolder && pItem->m_dwSize == 0)
        {
          CRegExp reSize;
          reSize.RegComp(">([0-9.]+)(K|M|G)</td>");
          if (reSize.RegFind(strBuffer.c_str()) >= 0)
          {
            double Size = atof(reSize.GetReplaceString("\\1"));
            CStdString strUnit = reSize.GetReplaceString("\\2");

            if (strUnit == "M")
              Size = Size * 1024;
            else if (strUnit == "G")
              Size = Size * 1000 * 1024;

            pItem->m_dwSize = (__int64)(Size * 1024);
          }
        }

        items.Add(pItem);
      }
    }
  }
  http.Close();

  return true;
}
