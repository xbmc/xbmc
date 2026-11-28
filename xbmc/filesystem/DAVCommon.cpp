/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DAVCommon.h"

#include "FileItem.h"
#include "filesystem/IDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <locale>
#include <sstream>

#include <tinyxml2.h>

using namespace XFILE;

namespace
{
// Converts broken-down UTC fields to a Unix epoch. Unlike mktime() the fields
// are not interpreted in the local timezone.
time_t UtcTmToTimeT(const struct tm& tmDate)
{
  KODI::TIME::SystemTime st{};
  st.year = tmDate.tm_year + 1900;
  st.month = tmDate.tm_mon + 1;
  st.day = tmDate.tm_mday;
  st.hour = tmDate.tm_hour;
  st.minute = tmDate.tm_min;
  st.second = tmDate.tm_sec;

  KODI::TIME::FileTime fileTime{};
  if (KODI::TIME::SystemTimeToFileTime(&st, &fileTime) != 1)
    return 0;

  const int64_t ticks = (static_cast<int64_t>(fileTime.highDateTime) << 32) | fileTime.lowDateTime;
  // FILETIME counts 100ns intervals since 1601-01-01 UTC, time_t seconds since 1970-01-01 UTC.
  return static_cast<time_t>((ticks - 116444736000000000LL) / 10000000LL);
}
} // namespace

/*
 * Return true if pElement value is equal value without namespace.
 *
 * if pElement is <DAV:foo> and value is foo then ValueWithoutNamespace is true
 */
bool CDAVCommon::ValueWithoutNamespace(const tinyxml2::XMLNode* node, const std::string& value)
{
  if (!node)
  {
    return false;
  }

  auto* element = node->ToElement();

  if (!element)
  {
    return false;
  }

  std::vector<std::string> tag = StringUtils::Split(element->Value(), ":", 2);

  if (tag.size() == 1 && tag[0] == value)
  {
    return true;
  }
  else if (tag.size() == 2 && tag[1] == value)
  {
    return true;
  }
  else if (tag.size() > 2)
  {
    CLog::LogF(LOGERROR, "Splitting {} failed, size(): {}, value: {}", element->Value(), tag.size(),
               value);
  }

  return false;
}

/*
 * Search for <status> and return its content
 */
std::string CDAVCommon::GetStatusTag(const tinyxml2::XMLElement* element)
{
  for (auto* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    if (ValueWithoutNamespace(child, "status"))
    {
      return child->NoChildren() ? "" : child->FirstChild()->Value();
    }
  }

  return "";
}

// Parses an HTTP-date (RFC 9110 IMF-fixdate), which is always GMT.
bool CDAVCommon::ParseHttpDate(const std::string& date, time_t& result)
{
  struct tm tmDate = {};
  // HTTP-dates use English day and month names whatever the LC_TIME locale
  std::istringstream stream(date);
  stream.imbue(std::locale::classic());
  stream >> std::get_time(&tmDate, "%a, %d %b %Y %H:%M:%S");
  if (stream.fail())
    return false;

  result = UtcTmToTimeT(tmDate);
  return true;
}

// Parses an ISO-8601 date-time as used by the creationdate property, honoring
// an explicit zone designator (Z or +/-hh:mm). A bare date-time is UTC.
bool CDAVCommon::ParseIsoDateTime(const std::string& dateTime, time_t& result)
{
  struct tm tmDate = {};
  const char* rest = strptime(dateTime.c_str(), "%Y-%m-%dT%T", &tmDate);
  if (!rest)
    return false;

  const time_t epoch = UtcTmToTimeT(tmDate);

  // Skip fractional seconds.
  if (*rest == '.')
  {
    rest++;
    while (*rest >= '0' && *rest <= '9')
      rest++;
  }

  if (*rest == 'Z' || *rest == 'z' || *rest == '\0')
  {
    result = epoch;
    return true;
  }
  if (*rest != '+' && *rest != '-')
    return false;

  const bool negative = (*rest == '-');
  rest++;
  if (!(rest[0] >= '0' && rest[0] <= '9' && rest[1] >= '0' && rest[1] <= '9'))
    return false;
  const int hours = (rest[0] - '0') * 10 + (rest[1] - '0');
  rest += 2;

  int minutes = 0;
  if (*rest == ':')
    rest++;
  if (rest[0] >= '0' && rest[0] <= '9' && rest[1] >= '0' && rest[1] <= '9')
    minutes = (rest[0] - '0') * 10 + (rest[1] - '0');

  if (hours > 23 || minutes > 59)
    return false;

  const int64_t offset = static_cast<int64_t>(hours) * 3600 + minutes * 60;
  result = static_cast<time_t>(epoch + (negative ? offset : -offset));
  return true;
}

// Parses the properties of a single <response> from a PROPFIND multistatus
// reply into a stat structure. Returns false if no usable properties were found.
bool CDAVCommon::ParseDavStatResponse(const tinyxml2::XMLNode* response, struct __stat64& stat)
{
  bool found = false;
  bool hasLength = false;
  bool isDir = false;
  int64_t size = 0;
  time_t mtime = 0;

  for (auto* propstat = response->FirstChild(); propstat; propstat = propstat->NextSibling())
  {
    if (!ValueWithoutNamespace(propstat, "propstat") ||
        GetStatusTag(propstat->ToElement()).find("200 OK") == std::string::npos)
      continue;

    for (auto* prop = propstat->FirstChild(); prop; prop = prop->NextSibling())
    {
      if (!ValueWithoutNamespace(prop, "prop"))
        continue;

      for (auto* item = prop->FirstChildElement(); item; item = item->NextSiblingElement())
      {
        found = true;
        if (ValueWithoutNamespace(item, "getcontentlength") && !item->NoChildren())
        {
          size = std::strtoll(item->FirstChild()->Value(), nullptr, 10);
          hasLength = true;
        }
        else if (ValueWithoutNamespace(item, "getlastmodified") && !item->NoChildren())
          ParseHttpDate(item->FirstChild()->Value(), mtime);
        else if (ValueWithoutNamespace(item, "resourcetype") &&
                 ValueWithoutNamespace(item->FirstChild(), "collection"))
          isDir = true;
      }
    }
  }

  if (!found)
    return false;

  // Collections have no content length; a regular file without one has
  // incomplete metadata, so let the caller fall back to HEAD.
  if (!isDir && !hasLength)
    return false;

  stat = {};
  stat.st_mode = isDir ? _S_IFDIR : _S_IFREG;
  stat.st_size = size;
  stat.st_mtime = mtime;
  return true;
}

void CDAVCommon::ParseDavDirectoryResponse(const tinyxml2::XMLElement* element, CFileItem& item)
{
  bool hasLength = false;
  bool hasMtime = false;
  time_t mtime = 0;

  /* Iterate response children elements */
  for (auto* responseChild = element->FirstChildElement(); responseChild;
       responseChild = responseChild->NextSiblingElement())
  {
    if (ValueWithoutNamespace(responseChild, "href") && !responseChild->NoChildren())
    {
      std::string path(responseChild->FirstChild()->Value());
      URIUtils::RemoveSlashAtEnd(path);
      item.SetPath(path);
    }
    else if (ValueWithoutNamespace(responseChild, "propstat"))
    {
      if (GetStatusTag(responseChild->ToElement()).find("200 OK") != std::string::npos)
      {
        /* Iterate propstat children elements */
        for (auto* propstatChild = responseChild->FirstChild(); propstatChild;
             propstatChild = propstatChild->NextSibling())
        {
          if (ValueWithoutNamespace(propstatChild, "prop"))
          {
            /* Iterate all properties available */
            for (auto* propChild = propstatChild->FirstChildElement(); propChild;
                 propChild = propChild->NextSiblingElement())
            {
              if (ValueWithoutNamespace(propChild, "getcontentlength") && !propChild->NoChildren())
              {
                item.SetSize(std::strtoll(propChild->FirstChild()->Value(), nullptr, 10));
                hasLength = true;
              }
              else if (ValueWithoutNamespace(propChild, "getlastmodified") &&
                       !propChild->NoChildren())
              {
                if (ParseHttpDate(propChild->FirstChild()->Value(), mtime))
                {
                  item.SetDateTime(mtime);
                  hasMtime = true;
                }
              }
              else if (ValueWithoutNamespace(propChild, "displayname") && !propChild->NoChildren())
              {
                item.SetLabel(URIUtils::DecodePathEscapes(propChild->FirstChild()->Value()));
              }
              else if (ValueWithoutNamespace(propChild, "creationdate") && !propChild->NoChildren())
              {
                // creationdate is birth/display metadata only; it must not feed
                // the stat:st_ctime change-detection property.
                time_t ctime;
                if (ParseIsoDateTime(propChild->FirstChild()->Value(), ctime) &&
                    !item.GetDateTime().IsValid())
                  item.SetDateTime(ctime);
              }
              else if (ValueWithoutNamespace(propChild, "resourcetype"))
              {
                if (ValueWithoutNamespace(propChild->FirstChild(), "collection"))
                {
                  item.SetFolder(true);
                }
              }
            }
          }
        }
      }
    }
  }

  // Regular files without a reported content length have incomplete stat data,
  // so leave DIR_PROPERTY_STAT_MTIME unset and let callers fall back to Stat().
  if (hasMtime && (item.IsFolder() || hasLength))
    item.SetProperty(DIR_PROPERTY_STAT_MTIME, static_cast<int64_t>(mtime));
}
