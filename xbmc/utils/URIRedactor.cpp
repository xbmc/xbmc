/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URIRedactor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/StackDirectory.h"
#include "utils/StringUtils.h"

#include <vector>

std::string CURIRedactor::GetWithoutUserDetails(const CURL& url, bool redact)
{
  std::string strURL;

  if (url.IsProtocol("stack"))
  {
    CFileItemList items;
    XFILE::CStackDirectory dir;
    dir.GetDirectory(url, items);
    std::vector<std::string> newItems;
    for (int i = 0; i < items.Size(); ++i)
    {
      CURL member(items[i]->GetPath());
      items[i]->SetPath(GetWithoutUserDetails(member, redact));
      newItems.push_back(items[i]->GetPath());
    }
    dir.ConstructStackPath(newItems, strURL);
    return strURL;
  }

  unsigned int sizeneed = url.GetProtocol().length() + url.GetHostName().length() +
                          url.GetFileName().length() + url.GetOptions().length() +
                          url.GetProtocolOptions().length() + 10;

  if (redact && !url.GetUserName().empty())
  {
    sizeneed += sizeof("USERNAME");
    if (!url.GetPassWord().empty())
      sizeneed += sizeof(":PASSWORD@");
    if (!url.GetDomain().empty())
      sizeneed += sizeof("DOMAIN;");
  }

  strURL.reserve(sizeneed);

  if (url.GetProtocol().empty())
    return url.GetFileName();

  strURL = url.GetProtocol();
  strURL += "://";

  if (redact && !url.GetUserName().empty())
  {
    if (!url.GetDomain().empty())
      strURL += "DOMAIN;";
    strURL += "USERNAME";
    if (!url.GetPassWord().empty())
      strURL += ":PASSWORD";
    strURL += "@";
  }

  if (!url.GetHostName().empty())
  {
    std::string strHostName;

    if (url.HasParentInHostname())
      strHostName = GetWithoutUserDetails(CURL(url.GetHostName()), false);
    else
      strHostName = url.GetHostName();

    if (url.HasEncodedHostname())
      strHostName = CURL::Encode(strHostName);

    if (url.HasPort())
    {
      if (strHostName.find(':') != strHostName.rfind(':'))
        strHostName = '[' + strHostName + ']';
      strURL += strHostName + StringUtils::Format(":{}", url.GetPort());
    }
    else
      strURL += strHostName;

    strURL += "/";
  }
  strURL += url.GetFileName();

  if (!url.GetOptions().empty())
    strURL += url.GetOptions();
  if (!url.GetProtocolOptions().empty())
    strURL += "|" + url.GetProtocolOptions();

  return strURL;
}

std::string CURIRedactor::GetRedacted(std::string_view path)
{
  return GetWithoutUserDetails(CURL(std::string(path)), true);
}
