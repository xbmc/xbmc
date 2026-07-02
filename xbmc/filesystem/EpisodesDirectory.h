/*
 *  Copyright (C) 2026 Team Kodi
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
 \brief Abstracts a virtual directory (episodes://) which points to one or more episodes
 */
class CEpisodesDirectory : public IFileDirectory
{
public:
  CEpisodesDirectory() = default;
  ~CEpisodesDirectory() override = default;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool ContainsFiles(const CURL& url) override { return false; }
  bool Resolve(CFileItem& item) const override;
};
} // namespace XFILE
