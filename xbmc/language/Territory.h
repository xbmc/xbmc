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
#include <string_view>

namespace KODI::LANGUAGE
{
class CLanguageTag;

/*!
 * \brief A place a language is used in, carried as a value rather than as a bare string.
 *
 * What BCP 47 calls a region subtag: an ISO 3166-1 alpha-2 code spelled the way that standard
 * publishes it, or a three-digit UN M.49 code for an area ISO 3166 does not cover. That is the
 * form it holds; the others are derived on demand and none is stored.
 *
 * \note There is no parse and no invalid state. What counts as a region is CBcp47's answer, and
 *       text is judged against it once, on the way in. A territory holding a code holds a real
 *       one.
 *
 * \note Naming no place is an ordinary state, not a failure: most tags state no region, and a
 *       default-constructed territory is that. Every member answers with nothing for it, so a
 *       caller that has no use for the distinction never has to make it.
 *
 * \note Not every territory is a country. 419 is Latin America and the Caribbean, EU is the
 *       European Union and DD was East Germany - all regions BCP 47 accepts and ISO 3166-1
 *       assigns no current code to. IsCountry is the question, and As* answers with nothing.
 */
class CTerritory
{
public:
  //! \brief A territory naming no place, which is what a tag stating no region has.
  CTerritory() = default;

  /*!
   * \brief Build a territory from a code read from a file or a table.
   * \note For text arriving as loose characters. Anything holding a language tag takes the
   *       territory from the tag instead.
   * \param[in] code An ISO 3166-1 alpha-2 code or a three-digit UN M.49 code.
   * \return The territory, naming no place where the text names none.
   */
  static CTerritory FromCode(std::string_view code);

  /*!
   * \brief Whether the place is one ISO 3166-1 assigns a current code to.
   * \return true for a place with an ISO 3166-1 code.
   */
  bool IsCountry() const;

  //! \brief The canonical form, which is a BCP 47 region subtag.
  const std::string& ToString() const { return m_code; }

  /*!
   * \brief The 2-Char ISO 3166-1 Alpha-2 code.
   * \return The code, upper case as ISO 3166-1 publishes it, or an empty string where the place
   *         has no ISO 3166-1 code.
   */
  std::string AsIso3166_1Alpha2() const;

  /*!
   * \brief The 3-Char ISO 3166-1 Alpha-3 code.
   * \note A projection only. BCP 47 has no alpha-3 region, so nothing states a place that way
   *       and nothing needs reading back from it.
   * \return The code, upper case as ISO 3166-1 publishes it, or an empty string where the place
   *         has no ISO 3166-1 code.
   */
  std::string AsIso3166_1Alpha3() const;

  /*!
   * \brief The English name of the place, as shown to a user.
   * \return The name, or an empty string where the place is one neither ISO 3166-1 nor the
   *         subtag registry names.
   */
  std::string ToEnglishName() const;

  bool operator==(const CTerritory& other) const = default;

private:
  //! \brief From a region subtag already judged to be one, in the case a canonical tag holds it
  explicit CTerritory(std::string code) : m_code(std::move(code)) {}

  std::string m_code;
};
} // namespace KODI::LANGUAGE
