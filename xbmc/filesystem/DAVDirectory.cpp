/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DAVDirectory.h"

#include "CurlFile.h"
#include "DAVCommon.h"
#include "DAVFile.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

using namespace XFILE;

CDAVDirectory::CDAVDirectory(void) = default;
CDAVDirectory::~CDAVDirectory(void) = default;

bool CDAVDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CCurlFile dav;
  std::string strRequest = "PROPFIND";

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
    CLog::LogF(LOGERROR, "Unable to get dav directory ({})", url.GetRedacted());
    return false;
  }

  std::string strResponse;
  dav.ReadData(strResponse);

  std::string fileCharset(dav.GetProperty(XFILE::FileProperty::CONTENT_CHARSET));
  CXBMCTinyXML2 davResponse;
  davResponse.Parse(strResponse);

  if (!davResponse.Parse(strResponse))
  {
    CLog::LogF(LOGERROR, "Unable to process dav directory ({})", url.GetRedacted());
    dav.Close();
    return false;
  }

  // Iterate over all responses
  for (auto* child = davResponse.RootElement()->FirstChild(); child; child = child->NextSibling())
  {
    if (CDAVCommon::ValueWithoutNamespace(child, "response"))
    {
      CFileItem item;
      CDAVCommon::ParseDavDirectoryResponse(child->ToElement(), item);
      const CURL& url2(url);
      CURL url3(item.GetPath());

      std::string itemPath(URIUtils::AddFileToFolder(url2.GetWithoutFilename(), url3.GetFileName()));

      if (item.GetLabel().empty())
      {
        std::string name(itemPath);
        URIUtils::RemoveSlashAtEnd(name);
        item.SetLabel(URIUtils::GetDecodedFileName(name));
      }

      if (item.IsFolder())
        URIUtils::AddSlashAtEnd(itemPath);

      // Add back protocol options
      if (!url2.GetProtocolOptions().empty())
        itemPath += "|" + url2.GetProtocolOptions();
      item.SetPath(itemPath);

      if (!item.IsURL(url))
      {
        CFileItemPtr pItem(new CFileItem(item));
        items.Add(pItem);
      }
    }
  }

  dav.Close();

  return true;
}

bool CDAVDirectory::Create(const CURL& url)
{
  CDAVFile dav;
  std::string strRequest = "MKCOL";

  dav.SetCustomRequest(strRequest);

  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "{} - Unable to create dav directory ({}) - {}", __FUNCTION__,
              url.GetRedacted(), dav.GetLastResponseCode());
    return false;
  }

  dav.Close();

  return true;
}

bool CDAVDirectory::Exists(const CURL& url)
{
  CCurlFile dav;

  // Set the PROPFIND custom request else we may not find folders, depending
  // on the server's configuration
  std::string strRequest = "PROPFIND";
  dav.SetCustomRequest(strRequest);
  dav.SetRequestHeader("depth", 0);

  return dav.Exists(url);
}

bool CDAVDirectory::Remove(const CURL& url)
{
  CDAVFile dav;
  std::string strRequest = "DELETE";

  dav.SetCustomRequest(strRequest);

  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "{} - Unable to delete dav directory ({}) - {}", __FUNCTION__,
              url.GetRedacted(), dav.GetLastResponseCode());
    return false;
  }

  dav.Close();

  return true;
}
