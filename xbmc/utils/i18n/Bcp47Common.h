/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace KODI::UTILS::I18N
{
struct Bcp47Extension
{
  char name;
  std::vector<std::string> segments;
  bool operator==(const Bcp47Extension& other) const = default;
};

enum class Bcp47FormattingStyle
{
  FORMAT_BCP47, ///< BCP47 language tag with the recommended casing
  FORMAT_ENGLISH, ///< Subtags converted to English and formatted to mimick locale names
  FORMAT_DEBUG, ///< Dump all the information of a tag in raw form
};

enum class Bcp47TagType
{
  WELL_FORMED, ///< The tag conforms to the syntax specified by RFC5646 and is not a special case.
  GRANDFATHERED, ///< The tag is a regular or irregular grandfathered tag.
  PRIVATE_USE, ///< The tag consists solely of private-use subtags.
};
} // namespace KODI::UTILS::I18N
