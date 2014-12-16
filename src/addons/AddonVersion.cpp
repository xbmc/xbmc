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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "AddonVersion.h"
#include "utils/StringUtils.h"

namespace ADDON
{
  AddonVersion::AddonVersion(const std::string& version)
  : mEpoch(0), mUpstream(version.empty() ? "0.0.0" : version)
  {
    size_t pos = mUpstream.find(':');
    if (pos != std::string::npos)
    {
      mEpoch = strtol(mUpstream.c_str(), NULL, 10);
      mUpstream.erase(0, pos+1);
    }

    pos = mUpstream.find('-');
    if (pos != std::string::npos)
    {
      mRevision = mUpstream.substr(pos+1);
      mUpstream.erase(pos);
    }
  }

  /**Compare two components of a Debian-style version.  Return -1, 0, or 1
   * if a is less than, equal to, or greater than b, respectively.
   */
  int AddonVersion::CompareComponent(const char *a, const char *b)
  {
    while (*a && *b)
    {
      while (*a && *b && !isdigit(*a) && !isdigit(*b))
      {
        if (*a != *b)
        {
          if (*a == '~') return -1;
          if (*b == '~') return 1;
          return *a < *b ? -1 : 1;
        }
        a++;
        b++;
      }
      if (*a && *b && (!isdigit(*a) || !isdigit(*b)))
      {
        if (*a == '~') return -1;
        if (*b == '~') return 1;
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

  bool AddonVersion::operator<(const AddonVersion& other) const
  {
    if (mEpoch != other.mEpoch)
      return mEpoch < other.mEpoch;

    int result = CompareComponent(mUpstream.c_str(), other.mUpstream.c_str());
    if (result)
      return (result < 0);

    return (CompareComponent(mRevision.c_str(), other.mRevision.c_str()) < 0);
  }

  bool AddonVersion::operator==(const AddonVersion& other) const
  {
    return mEpoch == other.mEpoch
      && CompareComponent(mUpstream.c_str(), other.mUpstream.c_str()) == 0
      && CompareComponent(mRevision.c_str(), other.mRevision.c_str()) == 0;
  }

  bool AddonVersion::empty() const
  {
    return mEpoch == 0 && mUpstream == "0.0.0" && mRevision.empty();
  }

  std::string AddonVersion::asString() const
  {
    std::string out;
    if (mEpoch)
      out = StringUtils::Format("%i:", mEpoch);
    out += mUpstream;
    if (!mRevision.empty())
      out += "-" + mRevision;
    return out;
  }

  bool AddonVersion::SplitFileName(std::string& ID, std::string& version,
                                   const std::string& filename)
  {
    size_t dpos = filename.rfind("-");
    if (dpos == std::string::npos)
      return false;
    ID = filename.substr(0, dpos);
    version = filename.substr(dpos + 1);
    version = version.substr(0, version.size() - 4);

    return true;
  }
}
