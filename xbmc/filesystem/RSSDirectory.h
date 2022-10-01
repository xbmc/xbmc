/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"
#include "XBDateTime.h"
#include "threads/CriticalSection.h"

#include <map>
#include <string>

class CFileItemList;

namespace XFILE
{
  class CRSSDirectory : public IFileDirectory
  {
  public:
    CRSSDirectory();
    ~CRSSDirectory() override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool Exists(const CURL& url) override;
    bool AllowAll() const override { return true; }
    bool ContainsFiles(const CURL& url) override;
    DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_ONCE; }

  protected:
    // key is path, value is cache invalidation date
    static std::map<std::string,CDateTime> m_cache;
    static CCriticalSection m_section;
  };
}

