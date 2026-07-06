/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class ILocalizer;

namespace KODI::UTILS::I18N
{
/*! \brief List conjunction type, mirroring ICU UListFormatterType. */
enum class ListFormatType
{
  AND, //!< "A, B and C"
  OR, //!< "A, B or C"
  UNITS //!< "A, B, C" (no conjunction)
};

/*! \brief List width, mirroring ICU UListFormatterWidth. */
enum class ListFormatWidth
{
  WIDE,
  SHORT,
  NARROW
};

inline constexpr uint32_t STR_LIST_AND_TWO = 39900; // "{0} and {1}"
inline constexpr uint32_t STR_LIST_AND_START = 39901; // "{0}, {1}"
inline constexpr uint32_t STR_LIST_AND_MIDDLE = 39902; // "{0}, {1}"
inline constexpr uint32_t STR_LIST_AND_END = 39903; // "{0}, and {1}"
inline constexpr uint32_t STR_LIST_OR_TWO = 39910; // "{0} or {1}"
inline constexpr uint32_t STR_LIST_OR_END = 39911; // "{0}, or {1}"

/*!
 * \brief Immutable list formatter.
 *        Joins items using locale-aware list patterns. Depending on the
 *        ListFormatType, the final item may use a conjunction or only a
 *        separator.
 */
class CListFormatter
{
public:
  /*! \brief The four CLDR list patterns. Each uses {0} and {1} placeholders. */
  struct Patterns
  {
    std::string two; //!< exactly two items
    std::string start; //!< first pair of 3+ items
    std::string middle; //!< inner items of 3+ items
    std::string end; //!< last pair of 3+ items
  };

  explicit CListFormatter(Patterns patterns, bool isolateItems = true)
    : m_patterns(std::move(patterns)),
      m_isolateItems(isolateItems)
  {
  }

  /*!
   * \brief Create a formatter for the active locale (equivalent to
   *        ListFormatter::createInstance).
   * \param localizer source of translated patterns (e.g. g_localizeStrings)
   * \param type list conjunction type (AND, OR, UNITS)
   * \param width list width (WIDE, SHORT, NARROW)
   * \param isolateItems whether to isolate items with Unicode isolation characters
   */
  static CListFormatter CreateInstance(const ILocalizer& localizer,
                                       ListFormatType type = ListFormatType::AND,
                                       ListFormatWidth width = ListFormatWidth::WIDE,
                                       bool isolateItems = true);

  /*! \brief Format a list of strings (equivalent to ListFormatter::format). */
  std::string Format(const std::vector<std::string>& items) const;

private:
  static std::string Apply(std::string_view pattern, std::string_view a, std::string_view b);

  Patterns m_patterns;
  bool m_isolateItems{true};
};
} // namespace KODI::UTILS::I18N
