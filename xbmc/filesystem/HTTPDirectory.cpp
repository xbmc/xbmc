/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HTTPDirectory.h"

#include "CurlFile.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/HTMLUtil.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <climits>

using namespace XFILE;

CHTTPDirectory::CHTTPDirectory(void) = default;
CHTTPDirectory::~CHTTPDirectory(void) = default;

bool CHTTPDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CCurlFile http;

  const std::string& strBasePath = url.GetFileName();

  if(!http.Open(url))
  {
    CLog::Log(LOGERROR, "{} - Unable to get http directory ({})", __FUNCTION__, url.GetRedacted());
    return false;
  }

  CRegExp reItem(true); // HTML is case-insensitive
  reItem.RegComp("<a href=\"([^\"]*)\"[^>]*>\\s*(.*?)\\s*</a>(.+?)(?=<a|</tr|$)");

  CRegExp reDateTimeHtml(true);
  reDateTimeHtml.RegComp(
      "<td align=\"right\">([0-9]{2})-([A-Z]{3})-([0-9]{4}) ([0-9]{2}):([0-9]{2}) +</td>");

  CRegExp reDateTimeLighttp(true);
  reDateTimeLighttp.RegComp(
      "<td class=\"m\">([0-9]{4})-([A-Z]{3})-([0-9]{2}) ([0-9]{2}):([0-9]{2}):([0-9]{2})</td>");

  CRegExp reDateTimeNginx(true);
  reDateTimeNginx.RegComp("([0-9]{2})-([A-Z]{3})-([0-9]{4}) ([0-9]{2}):([0-9]{2})");

  CRegExp reDateTimeNginxFancy(true);
  reDateTimeNginxFancy.RegComp(
      "<td class=\"date\">([0-9]{4})-([A-Z]{3})-([0-9]{2}) ([0-9]{2}):([0-9]{2})</td>");

  CRegExp reDateTimeApacheNewFormat(true);
  reDateTimeApacheNewFormat.RegComp(
      "<td align=\"right\">([0-9]{4})-([0-9]{2})-([0-9]{2}) ([0-9]{2}):([0-9]{2}) +</td>");

  CRegExp reDateTime(true);
  reDateTime.RegComp("([0-9]{4})-([0-9]{2})-([0-9]{2}) ([0-9]{2}):([0-9]{2})");

  CRegExp reSizeHtml(true);
  reSizeHtml.RegComp("> *([0-9.]+) *(B|K|M|G| )(iB)?</td>");

  CRegExp reSize(true);
  reSize.RegComp(" +([0-9]+)(B|K|M|G)?(?=\\s|<|$)");

  /* read response from server into string buffer */
  std::string strBuffer;
  if (http.ReadData(strBuffer) && strBuffer.length() > 0)
  {
    /* if Content-Length is found and its not text/html, URL is pointing to file so don't treat URL as HTTPDirectory */
    if (!http.GetHttpHeader().GetValue("Content-Length").empty() &&
        !StringUtils::StartsWithNoCase(http.GetHttpHeader().GetValue("Content-type"), "text/html"))
    {
      return false;
    }

    std::string fileCharset(http.GetProperty(XFILE::FILE_PROPERTY_CONTENT_CHARSET));
    if (!fileCharset.empty() && fileCharset != "UTF-8")
    {
      std::string converted;
      if (g_charsetConverter.ToUtf8(fileCharset, strBuffer, converted) && !converted.empty())
        strBuffer = converted;
    }

    unsigned int bufferOffset = 0;
    while (bufferOffset < strBuffer.length())
    {
      int matchOffset = reItem.RegFind(strBuffer.c_str(), bufferOffset);
      if (matchOffset < 0)
        break;

      bufferOffset = matchOffset + reItem.GetSubLength(0);

      std::string strLink = reItem.GetMatch(1);
      std::string strName = reItem.GetMatch(2);
      std::string strMetadata = reItem.GetMatch(3);
      StringUtils::Trim(strMetadata);

      if(strLink[0] == '/')
        strLink = strLink.substr(1);

      std::string strNameTemp = StringUtils::Trim(strName);

      std::wstring wName, wLink, wConverted;
      if (fileCharset.empty())
        g_charsetConverter.unknownToUTF8(strNameTemp);
      g_charsetConverter.utf8ToW(strNameTemp, wName, false);
      HTML::CHTMLUtil::ConvertHTMLToW(wName, wConverted);
      g_charsetConverter.wToUTF8(wConverted, strNameTemp);
      URIUtils::RemoveSlashAtEnd(strNameTemp);

      std::string strLinkBase = strLink;
      std::string strLinkOptions;

      // split link with url options
      size_t pos = strLinkBase.find('?');
      if (pos != std::string::npos)
      {
        strLinkOptions = strLinkBase.substr(pos);
        strLinkBase.erase(pos);
      }

      // strip url fragment from the link
      pos = strLinkBase.find('#');
      if (pos != std::string::npos)
      {
        strLinkBase.erase(pos);
      }

      // Convert any HTTP character entities (e.g.: "&amp;") to percentage encoding
      // (e.g.: "%xx") as some web servers (Apache) put these in HTTP Directory Indexes
      // this is also needed as CURL objects interpret them incorrectly due to the ;
      // also being allowed as URL option separator
      if (fileCharset.empty())
        g_charsetConverter.unknownToUTF8(strLinkBase);
      g_charsetConverter.utf8ToW(strLinkBase, wLink, false);
      HTML::CHTMLUtil::ConvertHTMLToW(wLink, wConverted);
      g_charsetConverter.wToUTF8(wConverted, strLinkBase);

      // encoding + and ; to URL encode if it is not already encoded by http server used on the remote server (example: Apache)
      // more characters may be added here when required when required by certain http servers
      pos = strLinkBase.find_first_of("+;");
      while (pos != std::string::npos) 
      {
        std::stringstream convert;
        convert << '%' << std::hex << int(strLinkBase.at(pos));
        strLinkBase.replace(pos, 1, convert.str());
        pos = strLinkBase.find_first_of("+;");
      }

      std::string strLinkTemp = strLinkBase;

      URIUtils::RemoveSlashAtEnd(strLinkTemp);
      strLinkTemp = CURL::Decode(strLinkTemp);

      if (StringUtils::EndsWith(strNameTemp, "..>") &&
          StringUtils::StartsWith(strLinkTemp, strNameTemp.substr(0, strNameTemp.length() - 3)))
        strName = strNameTemp = strLinkTemp;

      /* Per RFC 1808 § 5.3, relative paths containing a colon ":" should be either prefixed with
       * "./" or escaped (as "%3A"). This handles the prefix case, the escaping should be handled by
       * the CURL::Decode above
       * - https://tools.ietf.org/html/rfc1808#section-5.3
       */
      auto NameMatchesLink([](const std::string& name, const std::string& link) -> bool
      {
        return (name == link) ||
               ((std::string::npos != name.find(':')) && (std::string{"./"}.append(name) == link));
      });

      // we detect http directory items by its display name and its stripped link
      // if same, we consider it as a valid item.
      if (strLinkTemp != ".." && strLinkTemp != "" && NameMatchesLink(strNameTemp, strLinkTemp))
      {
        CFileItemPtr pItem(new CFileItem(strNameTemp));
        pItem->SetProperty("IsHTTPDirectory", true);
        CURL url2(url);

        url2.SetFileName(strBasePath + strLinkBase);
        url2.SetOptions(strLinkOptions);
        pItem->SetURL(url2);

        if(URIUtils::HasSlashAtEnd(pItem->GetPath(), true))
          pItem->m_bIsFolder = true;

        std::string day, month, year, hour, minute;
        int monthNum = 0;

        if (reDateTimeHtml.RegFind(strMetadata.c_str()) >= 0)
        {
          day = reDateTimeHtml.GetMatch(1);
          month = reDateTimeHtml.GetMatch(2);
          year = reDateTimeHtml.GetMatch(3);
          hour = reDateTimeHtml.GetMatch(4);
          minute = reDateTimeHtml.GetMatch(5);
        }
        else if (reDateTimeNginxFancy.RegFind(strMetadata.c_str()) >= 0)
        {
          day = reDateTimeNginxFancy.GetMatch(3);
          month = reDateTimeNginxFancy.GetMatch(2);
          year = reDateTimeNginxFancy.GetMatch(1);
          hour = reDateTimeNginxFancy.GetMatch(4);
          minute = reDateTimeNginxFancy.GetMatch(5);
        }
        else if (reDateTimeNginx.RegFind(strMetadata.c_str()) >= 0)
        {
          day = reDateTimeNginx.GetMatch(1);
          month = reDateTimeNginx.GetMatch(2);
          year = reDateTimeNginx.GetMatch(3);
          hour = reDateTimeNginx.GetMatch(4);
          minute = reDateTimeNginx.GetMatch(5);
        }
        else if (reDateTimeLighttp.RegFind(strMetadata.c_str()) >= 0)
        {
          day = reDateTimeLighttp.GetMatch(3);
          month = reDateTimeLighttp.GetMatch(2);
          year = reDateTimeLighttp.GetMatch(1);
          hour = reDateTimeLighttp.GetMatch(4);
          minute = reDateTimeLighttp.GetMatch(5);
        }
        else if (reDateTimeApacheNewFormat.RegFind(strMetadata.c_str()) >= 0)
        {
          day = reDateTimeApacheNewFormat.GetMatch(3);
          monthNum = atoi(reDateTimeApacheNewFormat.GetMatch(2).c_str());
          year = reDateTimeApacheNewFormat.GetMatch(1);
          hour = reDateTimeApacheNewFormat.GetMatch(4);
          minute = reDateTimeApacheNewFormat.GetMatch(5);
        }
        else if (reDateTime.RegFind(strMetadata.c_str()) >= 0)
        {
          day = reDateTime.GetMatch(3);
          monthNum = atoi(reDateTime.GetMatch(2).c_str());
          year = reDateTime.GetMatch(1);
          hour = reDateTime.GetMatch(4);
          minute = reDateTime.GetMatch(5);
        }

        if (month.length() > 0)
          monthNum = CDateTime::MonthStringToMonthNum(month);

        if (day.length() > 0 && monthNum > 0 && year.length() > 0)
        {
          pItem->m_dateTime = CDateTime(atoi(year.c_str()), monthNum, atoi(day.c_str()), atoi(hour.c_str()), atoi(minute.c_str()), 0);
        }

        if (!pItem->m_bIsFolder)
        {
          if (reSizeHtml.RegFind(strMetadata.c_str()) >= 0)
          {
            double Size = atof(reSizeHtml.GetMatch(1).c_str());
            std::string strUnit(reSizeHtml.GetMatch(2));

            if (strUnit == "K")
              Size = Size * 1024;
            else if (strUnit == "M")
              Size = Size * 1024 * 1024;
            else if (strUnit == "G")
              Size = Size * 1024 * 1024 * 1024;

            pItem->m_dwSize = (int64_t)Size;
          }
          else if (reSize.RegFind(strMetadata.c_str()) >= 0)
          {
            double Size = atof(reSize.GetMatch(1).c_str());
            std::string strUnit(reSize.GetMatch(2));

            if (strUnit == "K")
              Size = Size * 1024;
            else if (strUnit == "M")
              Size = Size * 1024 * 1024;
            else if (strUnit == "G")
              Size = Size * 1024 * 1024 * 1024;

            pItem->m_dwSize = (int64_t)Size;
          }
          else
          if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bHTTPDirectoryStatFilesize) // As a fallback get the size by stat-ing the file (slow)
          {
            CCurlFile file;
            file.Open(url);
            pItem->m_dwSize=file.GetLength();
            file.Close();
          }
        }
        items.Add(pItem);
      }
    }
  }
  http.Close();

  items.SetProperty("IsHTTPDirectory", true);

  return true;
}

bool CHTTPDirectory::Exists(const CURL &url)
{
  CCurlFile http;
  struct __stat64 buffer;

  if( http.Stat(url, &buffer) != 0 )
  {
    return false;
  }

  if (buffer.st_mode == _S_IFDIR)
	  return true;

  return false;
}
