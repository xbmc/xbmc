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

#include "HTTPDirectory.h"
#include "URL.h"
#include "FileCurl.h"
#include "FileItem.h"
#include "utils/RegExp.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CHTTPDirectory::CHTTPDirectory(void){}
CHTTPDirectory::~CHTTPDirectory(void){}

bool CHTTPDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CFileCurl http;
  CURL url(strPath);

  CStdString strName, strLink;
  CStdString strBasePath = url.GetFileName();

  if(!http.Open(url))
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
      URIUtils::RemoveSlashAtEnd(strLinkTemp);
      URIUtils::RemoveSlashAtEnd(strNameTemp);
      CURL::Decode(strLinkTemp);

      if (strNameTemp == strLinkTemp)
      {
        g_charsetConverter.unknownToUTF8(strName);
        URIUtils::RemoveSlashAtEnd(strName);

        CFileItemPtr pItem(new CFileItem(strName));
        pItem->m_strPath = strBasePath + strLink;
        pItem->SetProperty("IsHTTPDirectory", true);

        if(URIUtils::HasSlashAtEnd(pItem->m_strPath))
          pItem->m_bIsFolder = true;

        url.SetFileName(pItem->m_strPath);
        pItem->m_strPath = url.Get();

        if (!pItem->m_bIsFolder && g_advancedSettings.m_bHTTPDirectoryStatFilesize)
        {
          CFileCurl file;
          file.Open(url);
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

            pItem->m_dwSize = (int64_t)(Size * 1024);
          }
        }

        items.Add(pItem);
      }
    }
  }
  http.Close();

  return true;
}

bool CHTTPDirectory::Exists(const char* strPath)
{
  CFileCurl http;
  CURL url(strPath);
  return http.Exists(url);
}
