/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"
#include "SMBFile.h"
#include "filesystem/IDirectory.h"

namespace XFILE
{
class CSMBDirectory : public IDirectory
{
public:
  CSMBDirectory(void);
  ~CSMBDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_ONCE; }
  bool Create(const CURL& url) override;
  bool Exists(const CURL& url) override;
  bool Remove(const CURL& url) override;

  int Open(const CURL &url);

private:
  int OpenDir(const CURL &url, std::string& strAuth);
};
}
