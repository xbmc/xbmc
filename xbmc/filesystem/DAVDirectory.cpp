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

#include "DAVDirectory.h"

#include "DAVCommon.h"
#include "DAVFile.h"
#include "URL.h"
#include "CurlFile.h"
#include "FileItem.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CDAVDirectory::CDAVDirectory(void) {}
CDAVDirectory::~CDAVDirectory(void) {}

/*
 * Parses a <response>
 *
 * <!ELEMENT response (href, ((href*, status)|(propstat+)), responsedescription?) >
 * <!ELEMENT propstat (prop, status, responsedescription?) >
 *
 */
void CDAVDirectory::ParseResponse(const TiXmlElement *pElement, CFileItem &item)
{
  const TiXmlNode *pResponseChild;
  const TiXmlNode *pPropstatChild;
  const TiXmlNode *pPropChild;

  /* Iterate response children elements */
  for (pResponseChild = pElement->FirstChild(); pResponseChild != 0; pResponseChild = pResponseChild->NextSibling())
  {
    if (CDAVCommon::ValueWithoutNamespace(pResponseChild, "href"))
    {
      CStdString path(pResponseChild->ToElement()->GetText());
      URIUtils::RemoveSlashAtEnd(path);
      item.SetPath(path);
    }
    else 
    if (CDAVCommon::ValueWithoutNamespace(pResponseChild, "propstat"))
    {
      if (CDAVCommon::GetStatusTag(pResponseChild->ToElement()) == "HTTP/1.1 200 OK")
      {
        /* Iterate propstat children elements */
        for (pPropstatChild = pResponseChild->FirstChild(); pPropstatChild != 0; pPropstatChild = pPropstatChild->NextSibling())
        {
          if (CDAVCommon::ValueWithoutNamespace(pPropstatChild, "prop"))
          {
            /* Iterate all properties available */
            for (pPropChild = pPropstatChild->FirstChild(); pPropChild != 0; pPropChild = pPropChild->NextSibling())
            {
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "getcontentlength"))
              {
                item.m_dwSize = strtoll(pPropChild->ToElement()->GetText(), NULL, 10);
              }
              else
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "getlastmodified"))
              {
                struct tm timeDate = {0};
                strptime(pPropChild->ToElement()->GetText(), "%a, %d %b %Y %T", &timeDate);
                item.m_dateTime = mktime(&timeDate);
              }
              else
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "displayname"))
              {
                item.SetLabel(pPropChild->ToElement()->GetText());
              }
              else
              if (!item.m_dateTime.IsValid() && CDAVCommon::ValueWithoutNamespace(pPropChild, "creationdate"))
              {
                struct tm timeDate = {0};
                strptime(pPropChild->ToElement()->GetText(), "%Y-%m-%dT%T", &timeDate);
                item.m_dateTime = mktime(&timeDate);
              }
              else 
              if (CDAVCommon::ValueWithoutNamespace(pPropChild, "resourcetype"))
              {
                if (CDAVCommon::ValueWithoutNamespace(pPropChild->FirstChild(), "collection"))
                {
                  item.m_bIsFolder = true;
                }
              }
            }
          }
        }
      }
    }
  }
}

bool CDAVDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CCurlFile dav;
  CURL url(strPath);
  CStdString strRequest = "PROPFIND";

  dav.SetCustomRequest(strRequest);
  dav.SetMimeType("text/xml; charset=\"utf-8\"");
  dav.SetRequestHeader("depth", 1);
  dav.SetPostData(
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
    " <D:propfind xmlns:D=\"DAV:\">"
    "   <D:prop>"
    "     <D:resourcetype/>"
    "     <D:getcontentlength/>"
    "     <D:getlastmodified/>"
    "     <D:creationdate/>"
    "     <D:displayname/>"
    "    </D:prop>"
    "  </D:propfind>");

  if (!dav.Open(url))
  {
    CLog::Log(LOGERROR, "%s - Unable to get dav directory (%s)", __FUNCTION__, CURL::GetRedacted(strPath).c_str());
    return false;
  }

  CStdString strResponse;
  dav.ReadData(strResponse);

  std::string fileCharset(dav.GetServerReportedCharset());
  CXBMCTinyXML davResponse;
  davResponse.Parse(strResponse, fileCharset);

  if (!davResponse.Parse(strResponse))
  {
    CLog::Log(LOGERROR, "%s - Unable to process dav directory (%s)", __FUNCTION__, CURL::GetRedacted(strPath).c_str());
    dav.Close();
    return false;
  }

  TiXmlNode *pChild;
  // Iterate over all responses
  for (pChild = davResponse.RootElement()->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
  {
    if (CDAVCommon::ValueWithoutNamespace(pChild, "response"))
    {
      CFileItem item;
      ParseResponse(pChild->ToElement(), item);
      CURL url2(strPath);
      CURL url3(item.GetPath());

      CStdString itemPath(URIUtils::AddFileToFolder(url2.GetWithoutFilename(), url3.GetFileName()));

      if (item.GetLabel().empty())
      {
        CStdString name(itemPath);
        URIUtils::RemoveSlashAtEnd(name);
        item.SetLabel(URIUtils::GetFileName(CURL::Decode(name)));
      }

      if (item.m_bIsFolder)
        URIUtils::AddSlashAtEnd(itemPath);

      // Add back protocol options
      if (!url2.GetProtocolOptions().empty())
        itemPath += "|" + url2.GetProtocolOptions();
      item.SetPath(itemPath);

      if (!item.GetPath().Equals(strPath))
      {
        CFileItemPtr pItem(new CFileItem(item));
        items.Add(pItem);
      }
    }
  }

  dav.Close();

  return true;
}

bool CDAVDirectory::Create(const char* strPath)
{
  CDAVFile dav;
  CURL url(strPath);
  CStdString strRequest = "MKCOL";

  dav.SetCustomRequest(strRequest);
 
  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "%s - Unable to create dav directory (%s) - %d", __FUNCTION__, url.GetRedacted().c_str(), dav.GetLastResponseCode());
    return false;
  }

  dav.Close();

  return true;
}

bool CDAVDirectory::Exists(const char* strPath)
{
  CCurlFile dav;

  // Set the PROPFIND custom request else we may not find folders, depending
  // on the server's configuration
  CStdString strRequest = "PROPFIND";
  dav.SetCustomRequest(strRequest);
  dav.SetRequestHeader("depth", 0);

  CURL url(strPath);
  return dav.Exists(url);
}

bool CDAVDirectory::Remove(const char* strPath)
{
  CDAVFile dav;
  CURL url(strPath);
  CStdString strRequest = "DELETE";

  dav.SetCustomRequest(strRequest);
 
  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "%s - Unable to delete dav directory (%s) - %d", __FUNCTION__, url.GetRedacted().c_str(), dav.GetLastResponseCode());
    return false;
  }

  dav.Close();

  return true;
}
