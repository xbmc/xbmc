/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

#include <string>
#include <utility>

class CURL;

namespace XFILE
{

class CMagnetDirectory : public IFileDirectory
{
public:
  CMagnetDirectory() = default;
  ~CMagnetDirectory() override = default;

  // Implementation of IDirectory via IFileDirectory
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Exists(const CURL& url) override;
  DIR_CACHE_TYPE GetCacheType(const CURL& url) const override { return DIR_CACHE_ALWAYS; }

  // Implementation of IFileDirectory
  bool ContainsFiles(const CURL& url) override;
};

} // namespace XFILE
