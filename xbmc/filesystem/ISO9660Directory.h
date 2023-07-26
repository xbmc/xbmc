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

class CISO9660Directory : public IFileDirectory
{
public:
  CISO9660Directory() = default;
  ~CISO9660Directory() = default;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Exists(const CURL& url) override;
  bool ContainsFiles(const CURL& url) override { return true; }
  bool Resolve(CFileItem& item) const override;
};
}
