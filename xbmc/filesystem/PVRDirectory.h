/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

namespace XFILE {

class CPVRDirectory
  : public IDirectory
{
public:
  CPVRDirectory();
  ~CPVRDirectory() override;

  bool GetDirectory(const CURL& url, CFileItemList &items) override;
  bool AllowAll() const override { return true; }
  CacheType GetCacheType(const CURL& url) const override { return CacheType::NEVER; }
  bool Exists(const CURL& url) override;
  bool Resolve(CFileItem& item) const override;

  static bool SupportsWriteFileOperations(const std::string& strPath);

  static bool HasTVRecordings();
  static bool HasDeletedTVRecordings();
  static bool HasRadioRecordings();
  static bool HasDeletedRadioRecordings();
};

}
