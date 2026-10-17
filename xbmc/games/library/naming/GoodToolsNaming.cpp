/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GoodToolsNaming.h"

#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <utility>

using namespace KODI;
using namespace GAME;
using namespace GAME::GOODTOOLS;

namespace
{
// GoodCodes' country codes, with the names the game library gives them. (D)
// and (Nl) are both the Netherlands.
constexpr auto countries = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"A", "Australia"},       {"As", "Asia"},       {"B", "Brazil"},       {"C", "Canada"},
    {"Ch", "China"},          {"D", "Netherlands"}, {"E", "Europe"},       {"F", "France"},
    {"G", "Germany"},         {"Gr", "Greece"},     {"HK", "Hong Kong"},   {"I", "Italy"},
    {"J", "Japan"},           {"K", "Korea"},       {"Nl", "Netherlands"}, {"No", "Norway"},
    {"R", "Russia"},          {"S", "Spain"},       {"Sw", "Sweden"},      {"U", "USA"},
    {"UK", "United Kingdom"}, {"Unk", "Unknown"},   {"W", "World"},
});

// The codes GoodTools writes for a translation's language that ISO 639-2
// doesn't give that language. ISO's "rom" is Romany and its "bra" is Braj.
constexpr auto translationLanguages = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"bra", "pt"}, // Brazilian Portuguese
    {"jap", "ja"},
    {"rom", "ro"},
});

// The standard codes written as a letter and a number: "[a1]", "[h2]"
constexpr auto numberedFlags = std::to_array<std::pair<char, FlagType>>({
    {'a', FlagType::ALTERNATE_DUMP},
    {'b', FlagType::BAD_DUMP},
    {'f', FlagType::FIXED},
    {'h', FlagType::HACK},
    {'o', FlagType::OVER_DUMP},
    {'p', FlagType::PIRATE},
    {'t', FlagType::TRAINED},
});
Flag MakeFlag(FlagType type)
{
  Flag flag;
  flag.type = type;
  return flag;
}
} // namespace

std::string_view CGoodToolsNaming::CountryName(std::string_view code)
{
  for (const auto& [c, name] : countries)
  {
    if (c == code)
      return name;
  }
  return {};
}

bool CGoodToolsNaming::IsLanguageCount(std::string_view tag)
{
  return tag.size() > 1 && tag[0] == 'M' &&
         std::ranges::all_of(tag.substr(1),
                             [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
}

std::optional<Flag> CGoodToolsNaming::ParseFlag(std::string_view flag)
{
  if (flag == "!")
    return MakeFlag(FlagType::VERIFIED);
  if (flag == "!p")
    return MakeFlag(FlagType::PENDING);

  // "[T+Ger1.00_Translator]": the language, then whatever the translator adds
  if (flag.size() >= 2 && (flag[0] == 'T' || flag[0] == 't') && (flag[1] == '+' || flag[1] == '-'))
  {
    Flag read = MakeFlag(flag[1] == '+' ? FlagType::TRANSLATION : FlagType::OLD_TRANSLATION);
    read.language = TranslationLanguage(flag.substr(2, 3));
    return read;
  }

  if (flag.empty())
    return std::nullopt;
  for (const auto& [letter, type] : numberedFlags)
  {
    if (flag[0] != letter)
      continue;
    const std::string_view number = flag.substr(1);
    if (!std::ranges::all_of(number,
                             [](char c) { return std::isdigit(static_cast<unsigned char>(c)); }))
      return std::nullopt;
    Flag read = MakeFlag(type);
    // A number too large to be one is left unread
    std::from_chars(number.data(), number.data() + number.size(), read.number);
    return read;
  }
  return std::nullopt;
}

std::string CGoodToolsNaming::TranslationLanguage(std::string_view code)
{
  const std::string lower = StringUtils::ToLower(std::string(code));
  for (const auto& [goodTools, iso] : translationLanguages)
  {
    if (goodTools == lower)
      return std::string(iso);
  }

  std::string iso;
  if (lower.size() == 3 && CLangCodeExpander::ConvertISO6392ToISO6391(lower, iso))
    return iso;
  return "";
}
