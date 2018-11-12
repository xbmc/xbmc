/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"

namespace XFILE
{
  class CUwpSMBFile;

  class CUwpSMBDirectory : public IDirectory
  {
    friend CUwpSMBFile;
  public:
    CUwpSMBDirectory() = default;
    virtual ~CUwpSMBDirectory(void) = default;
    virtual bool GetDirectory(const CURL& url, CFileItemList& items) override;
    DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_ONCE; };
    virtual bool Create(const CURL& url) override;
    virtual bool Exists(const CURL& url) override;
    virtual bool Remove(const CURL& url) override;

  private:
    static winrt::Windows::Storage::StorageFolder GetFolder(const CURL &url);
    static int StatDirectory(const CURL& url, struct __stat64* statData);
  };
}
