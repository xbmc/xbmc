/*
 *  Copyright (C) 2025-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/i18n/Bcp47Parser.h"

#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/i18n/Bcp47.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>

using namespace KODI::UTILS::I18N;
using namespace std::literals;

std::optional<ParsedBcp47Tag> CBcp47Parser::Parse(std::string str)
{
  StringUtils::Trim(str);
  StringUtils::ToLower(str);

  // Try fast parse of ISO 639 codes first
  auto tag = TryParseIso639(str);

  // Maybe an regular grandfathered tag
  if (!tag.has_value())
    tag = TryParseRegularGrandfathered(str);

  // Full parse of the language tag
  if (!tag.has_value())
    tag = TryGenericParse(str);

  // Maybe an irregular grandfathered tag
  if (!tag.has_value())
    tag = TryParseIrregularGrandfathered(str);

  return tag;
}

std::optional<ParsedBcp47Tag> CBcp47Parser::TryParseIso639(const std::string& str)
{
  assert(std::ranges::none_of(str, [](char c) { return StringUtils::isasciiuppercaseletter(c); }));

  if ((str.length() == 2 || str.length() == 3) && StringUtils::IsAsciiLetters(str))
  {
    ParsedBcp47Tag tag;
    tag.m_language = str;
    return tag;
  }

  return std::nullopt;
}

std::optional<ParsedBcp47Tag> CBcp47Parser::TryGenericParse(const std::string& str)
{
  assert(std::ranges::none_of(str, [](char c) { return StringUtils::isasciiuppercaseletter(c); }));

  // BCP 47 language tags are composed of multiple subtags in the following order:
  // primary language subtag 2 or 3 letters (ISO 639-1/-2/-3/-5)
  //   OR 4 letters reserved for future use
  //   OR 5 to 8 letters registered subtag
  // [optional] up to 3 extended languages subtags of 3 letters each, - separated, may follow a 2 or 3 letters language subtag
  // [optional] script subtag: 4 letters (ISO 15924)
  // [optional] region subtag: 2 letters (ISO3166) or 3 digits (UN M.49)
  // [optional] variant subtags 5 to 8 characters each or 4 characters starting with a digit
  // [optional] extension/private use subtags 1 letter except x then (- and 2 to 8 letters) at least once
  // [optional] private-use subtag letter x then (- and 1 to 8 letters) at least once

  // clang-format off
  const char bcp47Regexp[] =
      "^(?:(?:(?:(?:(?<langiso639>[a-z]{2,3})(?<extlangs>(?:-[a-z]{3}){0,3}))|(?<langother>[a-z]{4,8}))"
      "(?:-(?<script>[a-z]{4}))?"
      "(?:-(?<region>[a-z]{2}|[0-9]{3}))?"
      "(?<variants>(?:-(?:(?:[a-z0-9]{5,8})|(?:[0-9][a-z0-9]{3})))*)"
      "(?<extensions>(?:-(?:[a-wy-z0-9](?:-[a-z0-9]{2,8})+))*)"
      "(?:-[x](?<privateuse>(?:-[a-z0-9]{1,8})+))?)"
      "|(?:x(?<globalprivateuse>(?:-[a-z0-9]{1,8})+)))$";
  // clang-format on

  static CRegExp regLangCode;
  static bool initialized = regLangCode.RegComp(bcp47Regexp);

  if (!initialized || regLangCode.RegFind(str) < 0)
    return {};

  ParsedBcp47Tag tag{};

  tag.m_language = regLangCode.GetMatch("langiso639");
  if (tag.m_language.empty())
    tag.m_language = regLangCode.GetMatch("langother");

  std::string extLangsRaw = regLangCode.GetMatch("extlangs");
  if (!extLangsRaw.empty())
    // Skip the initial - of the match
    tag.m_extLangs = StringUtils::Split(extLangsRaw.substr(1, std::string::npos), '-');

  tag.m_script = regLangCode.GetMatch("script");
  tag.m_region = regLangCode.GetMatch("region");
  std::string variantsRaw = regLangCode.GetMatch("variants");

  if (!variantsRaw.empty())
    // Skip the initial - of the match
    tag.m_variants = StringUtils::Split(variantsRaw.substr(1, std::string::npos), '-');

  std::string extensionsRaw = regLangCode.GetMatch("extensions");
  if (!extensionsRaw.empty())
  {
    // The string starts with -a-xxxx..., with a the extension name and xxxx the extension text,
    // at least 2 characters.
    assert(extensionsRaw.length() >= std::size("-a-xx"));

    char key = extensionsRaw[1];
    std::vector<std::string> extSegments;

    // Skip the initial -a-
    auto extensions = StringUtils::Split(extensionsRaw.substr(3, std::string::npos), '-');

    for (auto& token : extensions)
    {
      if (token.length() == 1)
      {
        // found a new extension name: save the segments accumulated since the last extension name
        if (!extSegments.empty())
        {
          Bcp47Extension e{.name = key, .segments = std::move(extSegments)};
          tag.m_extensions.push_back(e);
          extSegments.clear();
        }
        key = token[0];
        continue;
      }
      extSegments.push_back(std::move(token));
    }
    // Save the text accumulated for the last found extension name
    if (!extSegments.empty())
    {
      Bcp47Extension e{.name = key, .segments = std::move(extSegments)};
      tag.m_extensions.push_back(e);
    }
  }

  std::string puRaw = regLangCode.GetMatch("privateuse");
  if (puRaw.empty())
  {
    puRaw = regLangCode.GetMatch("globalprivateuse");
    if (!puRaw.empty())
      tag.m_type = Bcp47TagType::PRIVATE_USE;
  }

  if (!puRaw.empty())
    // Skip the initial - of the match
    tag.m_privateUse = StringUtils::Split(puRaw.substr(1, std::string::npos), '-');

  return tag;
}

std::optional<ParsedBcp47Tag> CBcp47Parser::TryParseRegularGrandfathered(const std::string& str)
{
  assert(std::ranges::none_of(str, [](char c) { return StringUtils::isasciiuppercaseletter(c); }));

  // List safe to hardcode because RFC 5646 2.1 states
  // ... grandfathered tags listed in the productions 'regular' and 'irregular' ...  were registered
  // under [RFC3066] and are a fixed list that can never change.
  constexpr auto RegularGrandfatheredTags =
      std::array{"art-lojban"sv, "cel-gaulish"sv, "no-bok"sv,     "no-nyn"sv,  "zh-guoyu"sv,
                 "zh-hakka"sv,   "zh-min"sv,      "zh-min-nan"sv, "zh-xiang"sv};

  static_assert(std::ranges::is_sorted(RegularGrandfatheredTags, {}, {}));

  constexpr auto minSize =
      std::ranges::min(RegularGrandfatheredTags, {}, &std::string_view::size).size();
  constexpr auto maxSize =
      std::ranges::max(RegularGrandfatheredTags, {}, &std::string_view::size).size();

  if (str.size() < minSize || str.size() > maxSize)
    return {};

  auto it = std::ranges::lower_bound(RegularGrandfatheredTags, str, {}, {});
  if (it != RegularGrandfatheredTags.end() && *it == str)
  {
    ParsedBcp47Tag tag{};
    tag.m_grandfathered = *it;
    tag.m_type = Bcp47TagType::GRANDFATHERED;
    return tag;
  }

  return std::nullopt;
}

std::optional<ParsedBcp47Tag> CBcp47Parser::TryParseIrregularGrandfathered(const std::string& str)
{
  assert(std::ranges::none_of(str, [](char c) { return StringUtils::isasciiuppercaseletter(c); }));

  // List safe to hardcode because RFC 5646 2.1 states
  // ... grandfathered tags listed in the productions 'regular' and 'irregular' ...  were registered
  // under [RFC3066] and are a fixed list that can never change.

  constexpr auto IrregularGrandfatheredTags =
      std::array{"en-GB-oed"sv, "i-ami"sv, "i-bnn"sv,     "i-default"sv, "i-enochian"sv, "i-hak"sv,
                 "i-klingon"sv, "i-lux"sv, "i-mingo"sv,   "i-navajo"sv,  "i-pwn"sv,      "i-tao"sv,
                 "i-tay"sv,     "i-tsu"sv, "sgn-BE-FR"sv, "sgn-BE-NL"sv, "sgn-CH-DE"sv};

  static_assert(std::ranges::is_sorted(IrregularGrandfatheredTags, {}, {}));

  constexpr auto minSize =
      std::ranges::min(IrregularGrandfatheredTags, {}, &std::string_view::size).size();
  constexpr auto maxSize =
      std::ranges::max(IrregularGrandfatheredTags, {}, &std::string_view::size).size();

  if (str.size() < minSize || str.size() > maxSize)
    return {};

  auto it = std::ranges::lower_bound(IrregularGrandfatheredTags, str, {}, {});
  if (it != IrregularGrandfatheredTags.end() && *it == str)
  {
    ParsedBcp47Tag tag{};
    tag.m_grandfathered = *it;
    tag.m_type = Bcp47TagType::GRANDFATHERED;
    return tag;
  }

  return std::nullopt;
}
