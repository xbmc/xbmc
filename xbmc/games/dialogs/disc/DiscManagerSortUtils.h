/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string>
#include <utility>

namespace KODI
{
namespace GAME
{
/*!
 * \brief Extract a normalized stem plus disc number for conservative display-only sorting
 */
std::optional<std::pair<std::string, int>> GetNormalizedStemAndTrailingNumber(
    const std::string& label);
} // namespace GAME
} // namespace KODI
