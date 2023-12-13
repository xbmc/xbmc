/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoVersionHelper
{
public:
  static std::shared_ptr<CFileItem> ChooseMovieFromVideoVersions(
      const std::shared_ptr<CFileItem>& item);
};
} // namespace GUILIB
} // namespace VIDEO
