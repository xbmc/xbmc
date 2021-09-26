/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

namespace XFILE
{
  class CZipDirectory : public IFileDirectory
  {
  public:
    CZipDirectory();
    ~CZipDirectory() override;
    bool GetDirectory(const CURL& url, CFileItemList& items) override;
    bool ContainsFiles(const CURL& url) override;
    DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_ALWAYS; }
  };
}
