/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFileDirectory.h"

namespace XFILE
{

/*!
 \brief Abstracts a DVD virtual directory (dvd://) which in turn points to the actual physical drive
 */
class CDVDDirectory : public IFileDirectory
{
public:
  CDVDDirectory() = default;
  ~CDVDDirectory() override = default;
  bool GetDirectory(const CURL& url, CFileItemList& items) override { return false; };
  bool ContainsFiles(const CURL& url) override { return false; }
  bool Resolve(CFileItem& item) const override;
};
} // namespace XFILE
