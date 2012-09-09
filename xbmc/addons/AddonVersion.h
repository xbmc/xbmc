/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <boost/operators.hpp>
#include "utils/StdString.h"

namespace ADDON
{
  /* \brief Addon versioning using the debian versioning scheme

    AddonVersion uses debian versioning, which means in the each section of the period
    separated version string, numbers are compared numerically rather than lexicographically,
    thus any preceding zeros are ignored.

    i.e. 1.00 is considered the same as 1.0, and 1.01 is considered the same as 1.1.

    Further, 1.0 < 1.0.0

    See here for more info: http://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Version
    */
  class AddonVersion : public boost::totally_ordered<AddonVersion> {
  public:
    AddonVersion(const AddonVersion& other) : mUpstream(NULL), mRevision(NULL) { *this = other; }
    explicit AddonVersion(const CStdString& version);
    ~AddonVersion();

    int Epoch() const { return mEpoch; }
    const char *Upstream() const { return mUpstream; }
    const char *Revision() const { return mRevision; }

    AddonVersion& operator=(const AddonVersion& other);
    bool operator<(const AddonVersion& other) const;
    bool operator==(const AddonVersion& other) const;
    CStdString Print() const;
    const char *c_str() const { return m_originalVersion.c_str(); };

    static bool SplitFileName(CStdString& ID, CStdString& version,
                              const CStdString& filename);

    static bool Test();
  protected:
    CStdString m_originalVersion;
    int mEpoch;
    char *mUpstream;
    char *mRevision;

    static int CompareComponent(const char *a, const char *b);
  };

  inline AddonVersion::~AddonVersion()
  {
    free(mUpstream);
    free(mRevision);
  }

  inline AddonVersion& AddonVersion::operator=(const AddonVersion& other)
  {
    free(mUpstream);
    free(mRevision);
    mEpoch = other.Epoch();
    mUpstream = strdup(other.Upstream());
    mRevision = strdup(other.Revision());
    m_originalVersion = other.m_originalVersion;
    return *this;
  }
}
