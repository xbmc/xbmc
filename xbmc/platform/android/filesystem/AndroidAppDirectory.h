/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"

class CFileItemList;

namespace XFILE
{

class CAndroidAppDirectory :
      public IDirectory
{
public:
  CAndroidAppDirectory() = default;
  ~CAndroidAppDirectory() override = default;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Exists(const CURL& url) override { return true; }
  bool AllowAll() const override { return true; }
  DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_NEVER; }
};
}
