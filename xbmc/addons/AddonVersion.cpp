/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonVersion.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
// Add-on versions are used e.g. in file names and should
// not have too much freedom in their accepted characters
// Things that should be allowed: e.g. 0.1.0~beta3+git010cab3
// Note that all of these characters are url-safe
const std::string VALID_ADDON_VERSION_CHARACTERS =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.+_@~";
} // namespace

namespace ADDON
{
CAddonVersion::CAddonVersion(const std::string& version)
  : m_epoch(0), m_upstream(version.empty() ? "0.0.0" : [&version] {
      auto versionLowerCase = std::string(version);
      StringUtils::ToLower(versionLowerCase);
      return versionLowerCase;
    }())
{
  size_t pos = m_upstream.find(':');
  if (pos != std::string::npos)
  {
    m_epoch = std::strtol(m_upstream.c_str(), nullptr, 10);
    m_upstream.erase(0, pos + 1);
  }

  pos = m_upstream.find('-');
  if (pos != std::string::npos)
  {
    m_revision = m_upstream.substr(pos + 1);
    if (m_revision.find_first_not_of(VALID_ADDON_VERSION_CHARACTERS) != std::string::npos)
    {
      CLog::Log(LOGERROR, "AddonVersion: {} is not a valid revision number", m_revision);
      m_revision = "";
    }
    m_upstream.erase(pos);
  }

  if (m_upstream.find_first_not_of(VALID_ADDON_VERSION_CHARACTERS) != std::string::npos)
  {
    CLog::Log(LOGERROR, "AddonVersion: {} is not a valid version", m_upstream);
    m_upstream = "0.0.0";
  }
}

CAddonVersion::CAddonVersion(const char* version)
  : CAddonVersion(std::string(version ? version : ""))
{
}

/**Compare two components of a Debian-style version.  Return -1, 0, or 1
   * if a is less than, equal to, or greater than b, respectively.
   */
int CAddonVersion::CompareComponent(const char* a, const char* b)
{
  while (*a && *b)
  {
    while (*a && *b && !std::isdigit(*a) && !std::isdigit(*b))
    {
      if (*a != *b)
      {
        if (*a == '~')
          return -1;
        if (*b == '~')
          return 1;
        return *a < *b ? -1 : 1;
      }
      a++;
      b++;
    }
    if (*a && *b && (!std::isdigit(*a) || !std::isdigit(*b)))
    {
      if (*a == '~')
        return -1;
      if (*b == '~')
        return 1;
      return std::isdigit(*a) ? -1 : 1;
    }

    char* next_a;
    char* next_b;
    long num_a = std::strtol(a, &next_a, 10);
    long num_b = std::strtol(b, &next_b, 10);
    if (num_a != num_b)
      return num_a < num_b ? -1 : 1;

    a = next_a;
    b = next_b;
  }
  if (!*a && !*b)
    return 0;
  if (*a)
    return *a == '~' ? -1 : 1;
  else
    return *b == '~' ? 1 : -1;
}

bool CAddonVersion::operator<(const CAddonVersion& other) const
{
  if (m_epoch != other.m_epoch)
    return m_epoch < other.m_epoch;

  int result = CompareComponent(m_upstream.c_str(), other.m_upstream.c_str());
  if (result)
    return (result < 0);

  return (CompareComponent(m_revision.c_str(), other.m_revision.c_str()) < 0);
}

bool CAddonVersion::operator>(const CAddonVersion& other) const
{
  return !(*this <= other);
}

bool CAddonVersion::operator==(const CAddonVersion& other) const
{
  return m_epoch == other.m_epoch &&
         CompareComponent(m_upstream.c_str(), other.m_upstream.c_str()) == 0 &&
         CompareComponent(m_revision.c_str(), other.m_revision.c_str()) == 0;
}

bool CAddonVersion::operator<=(const CAddonVersion& other) const
{
  return *this < other || *this == other;
}

bool CAddonVersion::operator>=(const CAddonVersion& other) const
{
  return !(*this < other);
}

bool CAddonVersion::empty() const
{
  return m_epoch == 0 && m_upstream == "0.0.0" && m_revision.empty();
}

std::string CAddonVersion::asString() const
{
  std::string out;
  if (m_epoch)
    out = StringUtils::Format("{}:", m_epoch);
  out += m_upstream;
  if (!m_revision.empty())
    out += "-" + m_revision;
  return out;
}

bool CAddonVersion::SplitFileName(std::string& ID,
                                  std::string& version,
                                  const std::string& filename)
{
  size_t dpos = filename.rfind('-');
  if (dpos == std::string::npos)
    return false;
  ID = filename.substr(0, dpos);
  version = filename.substr(dpos + 1);
  version.resize(version.size() - 4);

  return true;
}
} // namespace ADDON
