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

#include <string>
#include <boost/operators.hpp>

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
    AddonVersion(const AddonVersion& other) { *this = other; }
    explicit AddonVersion(const std::string& version);
    virtual ~AddonVersion() {};

    int Epoch() const { return mEpoch; }
    const std::string &Upstream() const { return mUpstream; }
    const std::string &Revision() const { return mRevision; }

    AddonVersion& operator=(const AddonVersion& other);
    bool operator<(const AddonVersion& other) const;
    bool operator==(const AddonVersion& other) const;
    std::string asString() const;
    bool empty() const;

    static bool SplitFileName(std::string& ID, std::string& version,
                              const std::string& filename);

  protected:
    int mEpoch;
    std::string mUpstream;
    std::string mRevision;

    static int CompareComponent(const char *a, const char *b);
  };

  inline AddonVersion& AddonVersion::operator=(const AddonVersion& other)
  {
    mEpoch = other.mEpoch;
    mUpstream = other.mUpstream;
    mRevision = other.mRevision;
    return *this;
  }
}
