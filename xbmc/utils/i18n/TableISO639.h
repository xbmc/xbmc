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

namespace
{
/*!
 * \brief Convert a language code from 2-3 letters string to a 4 bytes integer
 * \param[in] The string representation of the code
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

/*!
 * \brief Converts a language code given as a 4-byte integer to its string representation.
 * \param[in] code The language code coded as a 4-byte integer
 * \return The string representation
 */
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

/*!
 * \brief Concatenate arrays containing the same type of elements
 * \tparam T type of the common array element
 * \tparam ...Ns sizes of the arrays
 * \param[in] ...arrays arrays to concatenate
 * \return array containing the concatenation of all parameter arrays
 */
template<typename T, std::size_t... Ns>
constexpr auto ConcatenateArrays(const std::array<T, Ns>&... arrays)
{
  constexpr std::size_t totalSize = (Ns + ...); // Sum of sizes
  std::array<T, totalSize> result{};
  std::size_t currentIndex = 0;
  (
      [&](const auto& arr)
      {
        for (const T& elem : arr)
          result[currentIndex++] = elem;
      }(arrays),
      ...); // Apply lambda to each array in the pack
  return result;
}
} // namespace
