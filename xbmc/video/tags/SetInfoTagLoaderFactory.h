/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISetInfoTagLoader.h"
#include "addons/Scraper.h"

class CFileItem;

namespace KODI::VIDEO
{
class CSetInfoTagLoaderFactory
{
public:
  //! \brief Returns a tag loader for the given item.
  //! \param item The title of the set to find tag loader for
  static std::unique_ptr<ISetInfoTagLoader> CreateLoader(const std::string& title);

protected:
  // No instancing of this class
  CSetInfoTagLoaderFactory(void) = delete;
  virtual ~CSetInfoTagLoaderFactory() = delete;
};
} // namespace KODI::VIDEO
