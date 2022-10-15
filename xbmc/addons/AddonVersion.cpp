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
  : mEpoch(0), mUpstream(version.empty() ? "0.0.0" : [&version] {
      auto versionLowerCase = std::string(version);
      StringUtils::ToLower(versionLowerCase);
      return versionLowerCase;
    }())
{
  size_t pos = mUpstream.find(':');
  if (pos != std::string::npos)
  {
    mEpoch = strtol(mUpstream.c_str(), nullptr, 10);
    mUpstream.erase(0, pos + 1);
  }

  pos = mUpstream.find('-');
  if (pos != std::string::npos)
  {
    mRevision = mUpstream.substr(pos + 1);
    if (mRevision.find_first_not_of(VALID_ADDON_VERSION_CHARACTERS) != std::string::npos)
    {
      CLog::Log(LOGERROR, "AddonVersion: {} is not a valid revision number", mRevision);
      mRevision = "";
    }
    mUpstream.erase(pos);
  }

  if (mUpstream.find_first_not_of(VALID_ADDON_VERSION_CHARACTERS) != std::string::npos)
  {
    CLog::Log(LOGERROR, "AddonVersion: {} is not a valid version", mUpstream);
    mUpstream = "0.0.0";
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
    while (*a && *b && !isdigit(*a) && !isdigit(*b))
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
    if (*a && *b && (!isdigit(*a) || !isdigit(*b)))
    {
      if (*a == '~')
        return -1;
      if (*b == '~')
        return 1;
      return isdigit(*a) ? -1 : 1;
    }

    char *next_a, *next_b;
    long int num_a = strtol(a, &next_a, 10);
    long int num_b = strtol(b, &next_b, 10);
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
  if (mEpoch != other.mEpoch)
    return mEpoch < other.mEpoch;

  int result = CompareComponent(mUpstream.c_str(), other.mUpstream.c_str());
  if (result)
    return (result < 0);

  return (CompareComponent(mRevision.c_str(), other.mRevision.c_str()) < 0);
}

bool CAddonVersion::operator>(const CAddonVersion& other) const
{
  return !(*this <= other);
}

bool CAddonVersion::operator==(const CAddonVersion& other) const
{
  return mEpoch == other.mEpoch &&
         CompareComponent(mUpstream.c_str(), other.mUpstream.c_str()) == 0 &&
         CompareComponent(mRevision.c_str(), other.mRevision.c_str()) == 0;
}

bool CAddonVersion::operator!=(const CAddonVersion& other) const
{
  return !(*this == other);
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
  return mEpoch == 0 && mUpstream == "0.0.0" && mRevision.empty();
}

std::string CAddonVersion::asString() const
{
  std::string out;
  if (mEpoch)
    out = StringUtils::Format("{}:", mEpoch);
  out += mUpstream;
  if (!mRevision.empty())
    out += "-" + mRevision;
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
  version = version.substr(0, version.size() - 4);

  return true;
}
} // namespace ADDON
