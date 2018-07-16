/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined(TARGET_ANDROID)
#include "filesystem/IDirectory.h"
#include "FileItem.h"
namespace XFILE
{

class CAndroidAppDirectory :
      public IDirectory
{
public:
  CAndroidAppDirectory(void);
  virtual ~CAndroidAppDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList &items) override;
  virtual bool Exists(const CURL& url) override { return true; };
  virtual bool AllowAll() const override { return true; };
  virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_NEVER; }
};
}
#endif

