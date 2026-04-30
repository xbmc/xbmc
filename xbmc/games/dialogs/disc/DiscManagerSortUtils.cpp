/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerSortUtils.h"

#include "utils/StringUtils.h"

#include <charconv>
#include <optional>
#include <regex>
#include <string>
#include <system_error>

namespace
{
std::optional<int> ParseAsciiInteger(const std::string& value)
{
  int parsedValue = 0;
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), parsedValue);
  if (ec != std::errc{} || ptr != value.data() + value.size())
    return std::nullopt;

  return parsedValue;
}

std::optional<int> ParseRomanNumeral(const std::string& roman)
{
  int total = 0;
  int previous = 0;

  for (auto it = roman.rbegin(); it != roman.rend(); ++it)
  {
    int current = 0;
    switch (*it)
    {
      case 'I':
      case 'i':
        current = 1;
        break;
      case 'V':
      case 'v':
        current = 5;
        break;
      case 'X':
      case 'x':
        current = 10;
        break;
      default:
        return std::nullopt;
    }

    if (current < previous)
      total -= current;
    else
    {
      total += current;
      previous = current;
    }
  }

  if (total <= 0)
    return std::nullopt;

  return total;
}

std::optional<int> ParseDiscOrdinal(const std::string& value)
{
  if (const auto numericValue = ParseAsciiInteger(value); numericValue)
    return numericValue;

  return ParseRomanNumeral(value);
}

std::string NormalizeStem(std::string stem)
{
  StringUtils::Trim(stem);
  stem = std::regex_replace(stem, std::regex(R"(\s+)"), " ");
  StringUtils::ToLower(stem);
  return stem;
}
} // namespace

namespace KODI
{
namespace GAME
{
/*!
 * \brief Extract a lowercase stem plus trailing disc number from explicit disc markers
 *
 * Recognized suffix families are intentionally conservative:
 *   - <stem> (Disc|CD|Disk N) and <stem> (Disc|CD|Disk N of M)
 *   - <stem> Disc|CD|Disk N and <stem> Disc|CD|Disk N of M
 *   - <stem> [Disc|CD|Disk N], <stem> [discN], and <stem> [discNofM][...]
 *   - compact trailing forms like <stem> discN / cdN / diskN
 * Roman numerals are accepted only when attached to an explicit marker (e.g.
 * "Disc II"). Ambiguous or mixed labels keep original model order.
 */
std::optional<std::pair<std::string, int>> GetNormalizedStemAndTrailingNumber(
    const std::string& label)
{
  static const std::regex stemWithParenMarkerRegex(
      R"(^(.+\S)\s*\(\s*(disc|cd|disk)\s*([0-9]+|[ivx]+)\s*(?:of\s*\d+)?\s*\)\s*$)",
      std::regex::icase);
  static const std::regex stemWithMarkerRegex(
      R"(^(.+\S)\s+(disc|cd|disk)\s*([0-9]+|[ivx]+)\s*(?:of\s*\d+)?\s*$)", std::regex::icase);
  static const std::regex stemWithBracketedMarkerRegex(
      R"(^(.+\S)\s*\[\s*(disc|cd|disk)\s*([0-9]+|[ivx]+)\s*(?:of\s*\d+)?\s*\](?:\s*\[[^\]]+\])*\s*$)",
      std::regex::icase);
  static const std::regex stemWithCompactMarkerRegex(R"(^(.+\S)\s*(disc|cd|disk)\s*([0-9]+)\s*$)",
                                                     std::regex::icase);

  std::string trimmedLabel = label;
  StringUtils::Trim(trimmedLabel);

  std::smatch matches;
  if (!std::regex_match(trimmedLabel, matches, stemWithParenMarkerRegex) &&
      !std::regex_match(trimmedLabel, matches, stemWithMarkerRegex) &&
      !std::regex_match(trimmedLabel, matches, stemWithBracketedMarkerRegex) &&
      !std::regex_match(trimmedLabel, matches, stemWithCompactMarkerRegex))
  {
    return std::nullopt;
  }

  std::string normalizedStem = NormalizeStem(matches[1].str());
  if (normalizedStem.empty())
    return std::nullopt;

  const std::string ordinal = matches[3].str();
  const std::optional<int> trailingNumber = ParseDiscOrdinal(ordinal);
  if (!trailingNumber.has_value())
    return std::nullopt;

  return std::make_pair(normalizedStem, *trailingNumber);
}
} // namespace GAME
} // namespace KODI
