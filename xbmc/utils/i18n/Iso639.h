/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/StringUtils.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>

namespace KODI::UTILS::I18N
{
/*!
 * \brief Converts a language code given as a 4-byte integer to its string representation.
 * \param[in] code The language code coded as a 4-byte integer
 * \return The string representation
 */
std::string LongCodeToString(uint32_t code);

} // namespace KODI::UTILS::I18N

namespace
{
/*!
 * \brief Convert a language code from 2-3 letter string to a 4-byte integer
 * \param[in] a The string representation of the code
 * \return integer representation of the code
 */
constexpr uint32_t StringToLongCode(std::string_view a)
{
  const size_t len = a.length();

  assert(len <= 4);

  return static_cast<uint32_t>(len >= 4 ? a[len - 4] : 0) << 24 |
         static_cast<uint32_t>(len >= 3 ? a[len - 3] : 0) << 16 |
         static_cast<uint32_t>(len >= 2 ? a[len - 2] : 0) << 8 |
         static_cast<uint32_t>(len >= 1 ? a[len - 1] : 0);
}

struct LCENTRY
{
  uint32_t code;
  std::string_view name;
};

/*!
 * \brief Returns an array of ISO 639 codes sorted by code
 * \param codes array to sort
 * \return sorted array
 */
template<std::ranges::random_access_range T>
constexpr auto CreateIso639ByCode(T codes)
{
  std::ranges::sort(codes, {}, &LCENTRY::code);
  return codes;
}

/*!
 * \brief Returns an array of ISO 639 codes sorted by name (case-insensitive)
 * \param codes array to sort
 * \return sorted array
 */
template<std::ranges::random_access_range T>
constexpr auto CreateIso639ByName(T codes)
{
  //! @todo create the array with lower-cased names to avoid case-insensitive comparison later.
  std::ranges::sort(
      codes, [](std::string_view a, std::string_view b)
      { return StringUtils::CompareNoCase(a, b, 0) < 0; }, &LCENTRY::name);
  return codes;
}

} // namespace
