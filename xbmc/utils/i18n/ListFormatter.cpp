/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/ListFormatter.h"

#include "utils/ILocalizer.h"

#include <utility>

using namespace KODI::UTILS::I18N;

namespace
{
// English fallbacks if the localized string is missing.
CListFormatter::Patterns FallbackPatterns(ListFormatType type)
{
  switch (type)
  {
    case ListFormatType::OR:
      return {"{0} or {1}", "{0}, {1}", "{0}, {1}", "{0}, or {1}"};
    case ListFormatType::UNITS:
      return {"{0}, {1}", "{0}, {1}", "{0}, {1}", "{0}, {1}"};
    case ListFormatType::AND:
    default:
      return {"{0} and {1}", "{0}, {1}", "{0}, {1}", "{0}, and {1}"};
  }
}

std::string Localize(const ILocalizer& localizer, uint32_t id, std::string_view fallback)
{
  std::string s = localizer.Localize(id);
  return s.empty() ? std::string(fallback) : s;
}
} // unnamed namespace

CListFormatter CListFormatter::CreateInstance(const ILocalizer& localizer,
                                              ListFormatType type,
                                              ListFormatWidth width,
                                              bool isolateItems)
{
  // NOTE: width (WIDE/SHORT/NARROW) is accepted for API parity with ICU
  (void)width;

  const Patterns fb = FallbackPatterns(type);
  if (type == ListFormatType::UNITS)
  {
    const std::string sepStart = Localize(localizer, STR_LIST_AND_START, fb.start);
    const std::string sepMiddle = Localize(localizer, STR_LIST_AND_MIDDLE, fb.middle);
    return CListFormatter(Patterns{sepStart, sepStart, sepMiddle, sepMiddle}, isolateItems);
  }

  const bool isOr = type == ListFormatType::OR;
  Patterns p;
  p.two = Localize(localizer, isOr ? STR_LIST_OR_TWO : STR_LIST_AND_TWO, fb.two);
  p.start = Localize(localizer, STR_LIST_AND_START, fb.start);
  p.middle = Localize(localizer, STR_LIST_AND_MIDDLE, fb.middle);
  p.end = Localize(localizer, isOr ? STR_LIST_OR_END : STR_LIST_AND_END, fb.end);
  return CListFormatter(std::move(p), isolateItems);
}

std::string CListFormatter::Apply(std::string_view pattern, std::string_view a, std::string_view b)
{
  std::string out;
  out.reserve(pattern.size() + a.size() + b.size());
  for (size_t i = 0; i < pattern.size();)
  {
    if (pattern[i] == '{' && i + 2 < pattern.size() && pattern[i + 2] == '}')
    {
      if (pattern[i + 1] == '0')
      {
        out.append(a);
        i += 3;
        continue;
      }
      if (pattern[i + 1] == '1')
      {
        out.append(b);
        i += 3;
        continue;
      }
    }
    out.push_back(pattern[i++]);
  }
  return out;
}

namespace
{
// Explicit UTF-8 bytes to avoid execution-charset ambiguity (MSVC without /utf-8).
constexpr std::string_view FSI = "\xE2\x81\xA8"; // U+2068 First Strong Isolate
constexpr std::string_view PDI = "\xE2\x81\xA9"; // U+2069 Pop Directional Isolate
} // unnamed namespace

std::string CListFormatter::Format(const std::vector<std::string>& items) const
{
  const size_t n = items.size();
  if (n == 0)
    return {};

  // Wrap only the *raw* items, never the accumulator (it already carries isolates).
  const auto iso = [this](std::string_view item)
  {
    if (!m_isolateItems)
      return std::string(item);
    std::string s;
    s.reserve(item.size() + FSI.size() + PDI.size());
    s.append(FSI).append(item).append(PDI);
    return s;
  };

  if (n == 1)
    return iso(items[0]);
  if (n == 2)
    return Apply(m_patterns.two, iso(items[0]), iso(items[1]));

  std::string result = Apply(m_patterns.start, iso(items[0]), iso(items[1]));
  for (size_t i = 2; i < n - 1; ++i)
    result = Apply(m_patterns.middle, result, iso(items[i]));
  return Apply(m_patterns.end, result, iso(items[n - 1]));
}
