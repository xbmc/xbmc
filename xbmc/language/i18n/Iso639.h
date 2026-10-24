/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace KODI::LANGUAGE::I18N
{
//! How many characters a code packed into a 32 bit integer can hold, one per byte
inline constexpr std::size_t LONG_CODE_LENGTH{4};

/*!
 * \brief Converts a language code given as a 4-byte integer to its string representation.
 * \param[in] code The language code coded as a 4-byte integer
 * \return The string representation
 */
std::string LongCodeToString(uint32_t code);

/*!
 * \brief The correspondence between ISO 639 codes.
 *
 * ISO 639-1 and ISO 639-2 assign their codes independently, so neither can be derived from the
 * other and only a table records which alpha-2 code belongs with which alpha-3 one. ISO 639-2
 * also spells about twenty languages two ways, bibliographic (B) and terminological (T), and a
 * table records those pairs too.
 */
class CIso639
{
public:
  CIso639() = delete;

  /*!
   * \brief The ISO 639-2/B code of a language given by its ISO 639-1 code.
   * \param[in] code The alpha-2 code in lowercase, including the spellings ISO 639-1 has since
   *            withdrawn.
   * \return The alpha-3 code, or nullopt when the text is not an ISO 639-1 code.
   */
  static std::optional<std::string> Alpha2ToAlpha3B(std::string_view code);

  /*!
   * \brief The ISO 639-1 code of a language given by an ISO 639-2 code.
   * \param[in] code The alpha-3 code in lowercase, in either the bibliographic or the
   *            terminological form.
   * \return The alpha-2 code, or nullopt when the text is not an ISO 639-2 code or names a
   *         language ISO 639-1 gives no code to.
   */
  static std::optional<std::string> Alpha3ToAlpha2(std::string_view code);

  /*!
   * \brief The ISO 639-2/B code of a language given by its ISO 639-2/T code.
   * \param[in] tCode The terminological code.
   * \return The bibliographic code, or nullopt where the language has no separate one.
   */
  static std::optional<std::string> TCodeToBCode(std::string_view tCode);

  /*!
   * \brief The ISO 639-2/T code of a language given by its ISO 639-2/B code.
   * \param[in] bCode The bibliographic code.
   * \return The terminological code, or nullopt where the language has no separate one.
   */
  static std::optional<std::string> BCodeToTCode(std::string_view bCode);
};

} // namespace KODI::LANGUAGE::I18N

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

  assert(len <= KODI::LANGUAGE::I18N::LONG_CODE_LENGTH);

  return static_cast<uint32_t>(len >= 4 ? a[len - 4] : 0) << (3 * CHAR_BIT) |
         static_cast<uint32_t>(len >= 3 ? a[len - 3] : 0) << (2 * CHAR_BIT) |
         static_cast<uint32_t>(len >= 2 ? a[len - 2] : 0) << CHAR_BIT |
         static_cast<uint32_t>(len >= 1 ? a[len - 1] : 0);
}

struct LCENTRY
{
  uint32_t code;
  std::string_view name;
};
} // namespace
