/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

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
  class AddonVersion
  {
  public:
    AddonVersion(const AddonVersion& other) { *this = other; }
    explicit AddonVersion(const std::string& version);
    explicit AddonVersion(const char* version = nullptr);
    virtual ~AddonVersion() = default;

    int Epoch() const { return mEpoch; }
    const std::string &Upstream() const { return mUpstream; }
    const std::string &Revision() const { return mRevision; }

    AddonVersion& operator=(const AddonVersion& other);
    bool operator< (const AddonVersion& other) const;
    bool operator> (const AddonVersion& other) const;
    bool operator<=(const AddonVersion& other) const;
    bool operator>=(const AddonVersion& other) const;
    bool operator==(const AddonVersion& other) const;
    bool operator!=(const AddonVersion& other) const;
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

  inline AddonVersion& AddonVersion::operator=(const AddonVersion& other) = default;
}
