/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Iso639.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace KODI::UTILS::I18N
{
std::string LongCodeToString(uint32_t code)
{
  // Build the string in reverse order since appending to a string is more efficient than inserting
  // at position 0 and shifting the existing contents
  std::string ret;
  for (unsigned int j = 0; j < 4; j++)
  {
    char c = static_cast<char>(code) & 0xFF;
    if (c == '\0')
      break;
    ret.push_back(c);
    code >>= 8;
  }
  // Reverse the string for the final result
  std::reverse(ret.begin(), ret.end());
  return ret;
}
} // namespace KODI::UTILS::I18N
