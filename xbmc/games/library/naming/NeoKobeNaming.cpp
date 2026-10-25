/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NeoKobeNaming.h"

#include "utils/RegExp.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto companyTypeWords =
    std::to_array<std::string_view>({"co", "corp", "corporation", "doujin", "game", "games", "inc",
                                     "kk", "ltd", "soft", "software", "the"});

struct Patterns
{
  CRegExp alternateSet{true};
  CRegExp dataDisk{true};

  Patterns()
  {
    alternateSet.RegComp("^(alt|set) ?[0-9]+$");
    dataDisk.RegComp(
        "^(.+ )?(data|user|save|scenario|music|sound|map|character|backup|utility) dis[ck]( .+)?$");
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

std::string CNeoKobeNaming::RemoveLoadingInstructions(std::string_view name)
{
  std::string result(name);
  for (size_t open = result.find('{'); open != std::string::npos; open = result.find('{', open))
  {
    const size_t close = result.find('}', open);
    result.erase(open, close == std::string::npos ? std::string::npos : close - open + 1);
  }
  return result;
}

bool CNeoKobeNaming::IsAlternateSet(std::string_view flag)
{
  return Matches(GetPatterns().alternateSet, flag);
}

bool CNeoKobeNaming::IsBadDump(std::string_view flag)
{
  const std::string lower = StringUtils::ToLower(std::string(flag));
  return lower == "bad" || lower.starts_with("bad ");
}

bool CNeoKobeNaming::IsDataDisk(std::string_view tag)
{
  return Matches(GetPatterns().dataDisk, tag);
}

bool CNeoKobeNaming::IsHack(std::string_view tag)
{
  const std::string lower = StringUtils::ToLower(std::string(tag));
  return lower == "hack" || lower == "hacked" || lower.ends_with(" hack");
}

std::vector<std::string> CNeoKobeNaming::Credits(std::string_view tag)
{
  std::string credit(tag);
  if (StringUtils::StartsWithNoCase(credit, "doujin"))
  {
    credit.erase(0, 6);
    StringUtils::TrimLeft(credit, " -");
  }

  std::vector<std::string> credits;
  for (std::string part : StringUtils::Split(credit, " - "))
  {
    StringUtils::Trim(part);
    if (!part.empty())
      credits.emplace_back(std::move(part));
  }
  return credits;
}

bool CNeoKobeNaming::IsCompanyTypeWord(std::string_view word)
{
  return std::ranges::find(companyTypeWords, word) != companyTypeWords.end();
}
