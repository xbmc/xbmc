/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "AddonVersion.h"
#include "guilib/LocalizeStrings.h"

namespace ADDON
{
  AddonVersion::AddonVersion(const CStdString& version)
  {
    m_originalVersion = version;
    if (m_originalVersion.IsEmpty())
      m_originalVersion = "0.0.0";
    const char *epoch_end = strchr(m_originalVersion.c_str(), ':');
    if (epoch_end != NULL) {
      mEpoch = atoi(m_originalVersion.c_str());
    } else {
      mEpoch = 0;
    }

    const char *upstream_start;
    if (epoch_end) {
      upstream_start = epoch_end + 1;
    } else {
      upstream_start = m_originalVersion.c_str();
    }

    const char *upstream_end = strrchr(upstream_start, '-');
    size_t upstream_size;
    if (upstream_end == NULL) {
      upstream_size = strlen(upstream_start);
    } else {
      upstream_size = upstream_end - upstream_start;
    }

    mUpstream = (char*) malloc(upstream_size + 1);
    strncpy(mUpstream, upstream_start, upstream_size);
    mUpstream[upstream_size] = '\0';

    if (upstream_end == NULL) {
      mRevision = strdup("0");
    } else {
      mRevision = strdup(upstream_end + 1);
    }
  }

  /**Compare two components of a Debian-style version.  Return -1, 0, or 1
   * if a is less than, equal to, or greater than b, respectively.
   */
  int AddonVersion::CompareComponent(const char *a, const char *b)
  {
    long iA, iB;
    while (*a || *b)
    {
      iA = iB = 0;
      if (*a)
      {
        if (!isdigit(*a))
          a++;
        else
        {
          char *next;
          iA = strtol(a, &next, 10);
          a = next;
        }
      }
      if (*b)
      {
        if (!isdigit(*b))
          b++;
        else
        {
          char *next;
          iB = strtol(b, &next, 10);
          b = next;
        }
      }
    
      if (iA != iB)
        return iA < iB ? -1 : 1;
    }
    return 0;
  }

  bool AddonVersion::operator<(const AddonVersion& other) const
  {
    if (Epoch() != other.Epoch()) {
      return Epoch() < other.Epoch();
    }

    int result = CompareComponent(Upstream(), other.Upstream());
    if (result) {
      return -1 == result;
    }

    return -1 == CompareComponent(Revision(), other.Revision());
  }

  CStdString AddonVersion::Print() const
  {
    CStdString out;
    out.Format("%s %s", g_localizeStrings.Get(24051), m_originalVersion); // "Version <str>"
    return CStdString(out);
  }
}