/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NoIntroNaming.h"

#include "language/i18n/Iso639_1.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cctype>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto regions = std::to_array<std::string_view>(
    {// The convention's examples
     "Asia", "Australia", "Brazil", "Canada", "China", "Europe", "France", "Germany", "Hong Kong",
     "Italy", "Japan", "Korea", "Netherlands", "Spain", "Sweden", "USA", "World",
     // The others the DATs use
     "Argentina", "Denmark", "Finland", "Greece", "India", "Israel", "Latin America", "Mexico",
     "Norway", "Poland", "Portugal", "Russia", "Scandinavia", "Taiwan", "United Kingdom",
     "Unknown"});

// Redump writes the United Kingdom "UK"
constexpr std::string_view UK = "UK";

// The convention's examples are ST (Sufami Turbo), MB (Multiboot), NP
// (Nintendo Power) and Rumble Version; the rest are as the DATs write them.
// Sets name the ROMs that lock on to Sonic & Knuckles "(Lock-on)".
constexpr auto featureTags = std::to_array<std::string_view>(
    {"classic mini", "dsi enhanced", "gb compatible", "lock-on", "mb", "np", "rumble version",
     "sgb enhanced", "st", "switch online", "virtual console"});

// TOSEC's convention lists The, A, De, Die, Le, La and Les. No-Intro's moves
// "common articles", non-English ones too, without listing them; the others
// are those its DATs move.
constexpr auto articles = std::to_array<std::string_view>(
    {"a", "an", "das", "de", "der", "die", "el", "il", "la", "le", "les", "the"});

struct Patterns
{
  // "Rev X" and "vX.XX" are the convention's
  CRegExp revision{true};
  // The convention's Beta, Proto and Sample; the DATs' others
  CRegExp beta{true};
  CRegExp proto{true};
  CRegExp sample{true};
  CRegExp demo{true};
  CRegExp alpha{true};
  CRegExp debug{true};
  CRegExp program{true};
  CRegExp alt{true};
  // Redump's Disc; Disk, Side and Tape in the computer DATs
  CRegExp disc{true};

  Patterns()
  {
    revision.RegComp("^(rev|revision|version|v)\\.? ?([0-9][0-9a-z.]*|[a-z])$");
    beta.RegComp("^beta( ?[0-9]+)?$");
    proto.RegComp("^proto(type)?( ?[0-9]+)?$");
    sample.RegComp("^sample$");
    // Taikenban is what a Japanese trial version calls itself
    demo.RegComp("^(demo|kiosk|preview|trial|taikenban)\\b");
    alpha.RegComp("^alpha$");
    debug.RegComp("^debug( version)?$");
    program.RegComp("^(check |sample |test )?program$");
    alt.RegComp("^alt( ?[0-9]+)?$");
    disc.RegComp(
        "^(disc|disk|side|tape|cd|cart|part) ?([0-9]{1,4}[a-z]?|[a-z])( of ([0-9]{1,4}))?$");
  }
};

Patterns& GetPatterns()
{
  // CRegExp keeps the last match, so each thread needs its own
  thread_local Patterns patterns;
  return patterns;
}

bool Matches(CRegExp& re, std::string_view tag)
{
  return re.RegFind(StringUtils::ToLower(std::string(tag))) >= 0;
}
} // namespace

std::string_view CNoIntroNaming::RegionName(std::string_view region)
{
  if (region == UK)
    return "United Kingdom";
  for (std::string_view name : regions)
  {
    if (StringUtils::EqualsNoCase(name, region))
      return name;
  }
  return {};
}

bool CNoIntroNaming::IsLanguage(std::string_view code)
{
  if (code.size() != 2 || !std::isupper(static_cast<unsigned char>(code[0])) ||
      !std::islower(static_cast<unsigned char>(code[1])))
    return false;
  const std::string lower = StringUtils::ToLower(std::string(code));
  return LANGUAGE::I18N::CIso639_1::LookupByCode(lower).has_value();
}

bool CNoIntroNaming::IsRevision(std::string_view tag)
{
  return Matches(GetPatterns().revision, tag);
}

std::optional<ReleaseStatus> CNoIntroNaming::Status(std::string_view tag)
{
  Patterns& re = GetPatterns();
  if (Matches(re.beta, tag))
    return ReleaseStatus::BETA;
  if (Matches(re.proto, tag))
    return ReleaseStatus::PROTOTYPE;
  if (Matches(re.sample, tag))
    return ReleaseStatus::SAMPLE;
  if (Matches(re.demo, tag))
    return ReleaseStatus::DEMO;
  if (Matches(re.alpha, tag))
    return ReleaseStatus::ALPHA;
  if (Matches(re.debug, tag))
    return ReleaseStatus::DEBUG_BUILD;
  if (Matches(re.program, tag))
    return ReleaseStatus::PROGRAM;
  return std::nullopt;
}

std::optional<Licence> CNoIntroNaming::LicenceOf(std::string_view tag)
{
  const std::string lower = StringUtils::ToLower(std::string(tag));
  // Unl is the convention's; the DATs' others
  if (lower == "unl" || lower == "unlicensed")
    return Licence::UNLICENSED;
  if (lower == "pirate")
    return Licence::PIRATE;
  if (lower == "aftermarket")
    return Licence::AFTERMARKET;
  if (lower == "homebrew")
    return Licence::HOMEBREW;
  return std::nullopt;
}

bool CNoIntroNaming::ParseDisc(std::string_view tag, int& disc, int& discs)
{
  CRegExp& re = GetPatterns().disc;
  if (!Matches(re, tag))
    return false;

  const std::string number = re.GetMatch(2);
  const std::string total = re.GetMatch(4);
  disc = std::isdigit(static_cast<unsigned char>(number[0]))
             ? std::stoi(number)
             : static_cast<int>(std::tolower(static_cast<unsigned char>(number[0])) - 'a' + 1);
  discs = total.empty() ? 0 : std::stoi(total);
  return true;
}

bool CNoIntroNaming::IsAlternate(std::string_view tag)
{
  return Matches(GetPatterns().alt, tag);
}

bool CNoIntroNaming::IsFeatureTag(std::string_view tag)
{
  // One tag can list several: "(SGB Enhanced, GB Compatible)"
  const std::vector<std::string> parts = StringUtils::Split(std::string(tag), ',');
  return std::ranges::all_of(parts,
                             [](std::string part)
                             {
                               StringUtils::Trim(part);
                               StringUtils::ToLower(part);
                               return std::ranges::find(featureTags, part) != featureTags.end();
                             });
}

bool CNoIntroNaming::IsBiosFlag(std::string_view flag)
{
  return StringUtils::EqualsNoCase(flag, "bios");
}

bool CNoIntroNaming::IsArticle(std::string_view word)
{
  const std::string lower = StringUtils::ToLower(std::string(word));
  return std::ranges::find(articles, lower) != articles.end();
}
