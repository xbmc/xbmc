/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameNameParser.h"

#include "URL.h"
#include "games/library/naming/GoodToolsNaming.h"
#include "games/library/naming/NeoKobeNaming.h"
#include "games/library/naming/NoIntroNaming.h"
#include "games/library/naming/SceneNaming.h"
#include "games/library/naming/TosecNaming.h"
#include "language/i18n/Bcp47Registry/SubTagRegistryManager.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <utility>

using namespace KODI;
using namespace GAME;

namespace
{
std::string Lower(std::string_view s)
{
  return StringUtils::ToLower(std::string(s));
}

std::vector<std::string> SplitTrim(const std::string& s, char sep)
{
  std::vector<std::string> parts = StringUtils::Split(s, sep);
  for (std::string& p : parts)
    StringUtils::Trim(p);
  std::erase_if(parts, [](const std::string& p) { return p.empty(); });
  return parts;
}

// A region as the conventions write it outside a TOSEC name. GoodTools gives
// the country first, and only there is a lone letter a country: Redump's
// "(Disc 1) (R)" isn't Russia.
std::optional<GameRegion> RegionOf(std::string_view part, bool firstTag)
{
  if (std::optional<GameRegion> region = CNoIntroNaming::Region(part))
    return region;
  if (std::optional<GameRegion> region = CSceneNaming::Region(part))
    return region;
  if (firstTag)
    return CGoodToolsNaming::Region(part);
  return std::nullopt;
}

// A region the set doesn't know, "(Unknown)", is read but names none
bool AllRegions(const std::vector<std::string>& parts,
                bool firstTag,
                std::vector<GameRegion>& regions)
{
  regions.clear();
  for (const std::string& p : parts)
  {
    std::optional<GameRegion> region = RegionOf(p, firstTag);
    if (!region)
      return false;
    if (!region->IsEmpty())
      regions.emplace_back(std::move(*region));
  }
  return !parts.empty();
}

bool AllLanguages(const std::vector<std::string>& parts, std::vector<std::string>& codes)
{
  codes.clear();
  for (const std::string& p : parts)
  {
    if (!CNoIntroNaming::IsLanguage(p))
      return false;
    codes.emplace_back(Lower(p));
  }
  return !codes.empty();
}

int DiscNumber(const std::string& number)
{
  // "1-2" is the first; a letter counts from A
  if (std::isalpha(static_cast<unsigned char>(number[0])))
    return std::toupper(static_cast<unsigned char>(number[0])) - 'A' + 1;
  const std::string first = number.substr(0, number.find('-'));
  return first.size() <= 4 ? std::stoi(first) : 0;
}

ParsedGameName FromTosec(const TOSEC::Name& tosec)
{
  ParsedGameName out;
  out.tosec = true;
  out.title = tosec.title;
  StringUtils::Replace(out.title, "  ", " ");
  StringUtils::Trim(out.title, " -_");
  out.displayTitle = CGameNameParser::DisplayTitle(out.title);
  out.revision = tosec.version;
  out.year = tosec.date.year;
  out.publisher = StringUtils::Join(tosec.publishers, " - ");
  for (const std::string& country : tosec.countries)
  {
    if (std::optional<GameRegion> region = CTosecNaming::Region(country))
      out.regions.emplace_back(std::move(*region));
  }
  out.languages = tosec.languages;

  if (tosec.demo != TOSEC::Demo::NONE)
    out.status = ReleaseStatus::DEMO;
  switch (tosec.status)
  {
    case TOSEC::DevelopmentStatus::ALPHA:
      out.status = ReleaseStatus::ALPHA;
      break;
    case TOSEC::DevelopmentStatus::BETA:
    case TOSEC::DevelopmentStatus::PREVIEW:
    case TOSEC::DevelopmentStatus::PRE_RELEASE:
      out.status = ReleaseStatus::BETA;
      break;
    case TOSEC::DevelopmentStatus::PROTO:
      out.status = ReleaseStatus::PROTOTYPE;
      break;
    case TOSEC::DevelopmentStatus::RELEASE:
      break;
  }

  if (!tosec.media.number.empty())
  {
    out.disc = DiscNumber(tosec.media.number);
    if (!tosec.media.total.empty() && tosec.media.total.size() <= 4)
      out.discs = std::stoi(tosec.media.total);
  }

  for (const TOSEC::DumpFlag& flag : tosec.dumpFlags)
  {
    switch (flag.type)
    {
      case TOSEC::DumpFlagType::HACKED:
        out.hack = true;
        break;
      case TOSEC::DumpFlagType::PIRATED:
        out.licence = Licence::PIRATE;
        break;
      case TOSEC::DumpFlagType::TRANSLATED:
      {
        // "[tr de]", "[tr de-partial Translator]"
        out.translated = true;
        const std::string code = StringUtils::ToLower(flag.detail.substr(0, 2));
        if (CTosecNaming::IsLanguage(code))
          out.translation = code;
        break;
      }
      case TOSEC::DumpFlagType::BAD_DUMP:
        out.bad = true;
        break;
      case TOSEC::DumpFlagType::ALTERNATE_DUMP:
        out.alternate = true;
        break;
      case TOSEC::DumpFlagType::VERIFIED:
        out.verified = true;
        break;
      default:
        break;
    }
  }
  for (const std::string& info : tosec.moreInfo)
  {
    if (CNoIntroNaming::IsBiosFlag(info))
      out.status = ReleaseStatus::PROGRAM;
  }

  // Fields the sets write where the convention doesn't: "(Rev1)" after the
  // publisher, a disk number after the dump flags
  std::vector<std::string> loose = tosec.unknownTags;
  if (!tosec.mediaLabel.empty())
    loose.emplace_back(tosec.mediaLabel);
  for (const std::string& tag : loose)
  {
    if (out.revision.empty() && CNoIntroNaming::IsRevision(tag))
      out.revision = tag;
    else if (out.disc == 0 && CNoIntroNaming::ParseDisc(tag, out.disc, out.discs))
      continue;
    else if (CNeoKobeNaming::IsDataDisk(tag))
      out.dataDisk = true;
  }

  out.unknownTags = tosec.unknownTags;
  return out;
}

std::vector<std::string> CompanyWords(std::string_view company)
{
  std::vector<std::string> words;
  std::string word;
  auto add = [&words, &word]
  {
    if (!word.empty() && !CNeoKobeNaming::IsCompanyTypeWord(word))
      words.emplace_back(word);
    word.clear();
  };
  for (const char c : company)
  {
    if (std::isalnum(static_cast<unsigned char>(c)))
      word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    else
      add();
  }
  add();
  return words;
}
} // namespace

std::optional<GameRegion> CGameNameParser::Region(std::string_view region)
{
  std::string upper(region);
  StringUtils::ToUpper(upper);
  for (std::optional<GameRegion> found :
       {CNoIntroNaming::Region(region), CTosecNaming::Region(upper), CSceneNaming::Region(upper),
        CGoodToolsNaming::Region(region), CGoodToolsNaming::Region(upper)})
  {
    if (found)
      return found;
  }
  return std::nullopt;
}

std::string CGameNameParser::RegionName(const GameRegion& region)
{
  if (const std::string_view name = CNoIntroNaming::Name(region); !name.empty())
    return std::string(name);
  if (const std::string& code = region.territory.ToString(); !code.empty())
  {
    using LANGUAGE::I18N::CSubTagRegistryManager;
    const auto subtag =
        CSubTagRegistryManager::GetInstance().GetRegionSubTags().Lookup(StringUtils::ToLower(code));
    if (subtag && !subtag->m_descriptions.empty())
      return subtag->m_descriptions.front();
    return code;
  }
  return region.market;
}

std::string CGameNameParser::DisplayTitle(std::string_view title)
{
  // "Legend of Zelda, The - A Link to the Past" -> article first, subtitle kept
  const std::string text(title);
  const size_t subtitle = text.find(" - ");
  const std::string head = subtitle == std::string::npos ? text : text.substr(0, subtitle);
  const std::string tail = subtitle == std::string::npos ? "" : text.substr(subtitle);

  const size_t comma = head.rfind(", ");
  if (comma == std::string::npos)
    return text;

  const std::string article = head.substr(comma + 2);
  if (CNoIntroNaming::IsArticle(article))
    return article + " " + head.substr(0, comma) + tail;
  return text;
}

std::string CGameNameParser::ParseWhdLoadName(std::string_view fileName)
{
  std::string name(fileName);

  // Only WHDLoad names look like this. A catalogue name always has spaces in it,
  // so requiring their absence keeps every other convention out of here.
  if (name.find(' ') != std::string::npos)
    return "";

  const size_t dot = name.rfind('.');
  if (dot != std::string::npos && name.size() - dot <= 5)
    name.erase(dot);

  // The title is whatever precedes the version, which every slave carries
  static const std::string version = "_v";
  const size_t at = name.find(version);
  if (at == std::string::npos || at == 0 || at + version.size() >= name.size() ||
      !std::isdigit(static_cast<unsigned char>(name[at + version.size()])))
    return "";
  name.erase(at);

  std::string title;
  for (size_t i = 0; i < name.size(); ++i)
  {
    const unsigned char c = static_cast<unsigned char>(name[i]);
    if (c == '_' || c == '-')
    {
      title += ' ';
      continue;
    }
    if (i > 0 && !title.empty() && title.back() != ' ')
    {
      const unsigned char prev = static_cast<unsigned char>(name[i - 1]);
      const bool startsWord = std::isupper(c) && (std::islower(prev) || std::isdigit(prev));
      // "UFOEnemy" splits before the last capital of a run, not inside it
      const bool endsAcronym = std::isupper(c) && std::isupper(prev) && i + 1 < name.size() &&
                               std::islower(static_cast<unsigned char>(name[i + 1]));
      // "Turrican3" is two words, "A10" is one: a lone letter before the digits
      // belongs to them
      const size_t wordStart = title.rfind(' ');
      const size_t wordLength = title.size() - (wordStart == std::string::npos ? 0 : wordStart + 1);
      const bool startsNumber = std::isdigit(c) && std::isalpha(prev) && wordLength > 1;
      if (startsWord || endsAcronym || startsNumber)
        title += ' ';
    }
    title += static_cast<char>(c);
  }

  StringUtils::Trim(title);
  return title;
}

ParsedGameName CGameNameParser::Parse(std::string_view fileName, bool hasExtension)
{
  if (const std::string whdload = ParseWhdLoadName(fileName); !whdload.empty())
  {
    ParsedGameName out;
    out.title = whdload;
    out.displayTitle = DisplayTitle(whdload);
    return out;
  }

  std::string name(fileName);
  if (hasExtension)
  {
    const size_t dot = name.rfind('.');
    if (dot != std::string::npos && name.size() - dot >= 2 && name.size() - dot <= 5 &&
        std::all_of(name.begin() + dot + 1, name.end(),
                    [](char c) { return std::isalnum(static_cast<unsigned char>(c)); }))
    {
      const std::string ext = StringUtils::ToLower(name.substr(dot + 1));
      name.erase(dot);
      // A compressed tar archive has two extensions
      if ((ext == "gz" || ext == "bz2" || ext == "xz") && StringUtils::EndsWithNoCase(name, ".tar"))
        name.erase(name.size() - 4);
    }
  }
  name = CNeoKobeNaming::RemoveLoadingInstructions(name);

  if (const std::optional<TOSEC::Name> tosec = CTosecNaming::Parse(name))
    return FromTosec(*tosec);

  ParsedGameName out;

  // Bracket tags: dump flags
  std::string rest;
  size_t pos = 0;
  while (pos < name.size())
  {
    const size_t open = name.find('[', pos);
    if (open == std::string::npos)
    {
      rest += name.substr(pos);
      break;
    }
    const size_t close = name.find(']', open);
    if (close == std::string::npos)
    {
      // A tag that never closes means the name was cut short -- extracted sets
      // routinely hit a 64 character filename limit -- so what follows is half
      // a tag, not part of the title. "Bulldog (1988)(Ariolasoft UK LTD)(Tape 2"
      // is Bulldog.
      rest += name.substr(pos, open - pos);
      break;
    }
    rest += name.substr(pos, open - pos);

    std::string tag = name.substr(open + 1, close - open - 1);
    StringUtils::Trim(tag);

    if (CNoIntroNaming::IsBiosFlag(tag))
      out.status = ReleaseStatus::PROGRAM;
    else if (CNeoKobeNaming::IsBadDump(tag))
      out.bad = true;
    else if (CNeoKobeNaming::IsAlternateSet(tag))
      out.alternate = true;
    else if (const std::optional<GOODTOOLS::Flag> flag = CGoodToolsNaming::ParseFlag(tag))
    {
      switch (flag->type)
      {
        case GOODTOOLS::FlagType::VERIFIED:
          out.verified = true;
          break;
        case GOODTOOLS::FlagType::TRANSLATION:
        case GOODTOOLS::FlagType::OLD_TRANSLATION:
          out.translated = true;
          out.translation = flag->language;
          break;
        case GOODTOOLS::FlagType::BAD_DUMP:
          out.bad = true;
          break;
        case GOODTOOLS::FlagType::ALTERNATE_DUMP:
          out.alternate = true;
          break;
        case GOODTOOLS::FlagType::HACK:
          out.hack = true;
          break;
        case GOODTOOLS::FlagType::PIRATE:
          out.licence = Licence::PIRATE;
          break;
        default:
          break;
      }
    }
    // Any other flag describes the dump ([FD], [extras]), not who made it, so
    // it isn't kept with the unknown tags that can credit a company

    pos = close + 1;
  }
  name = rest;

  // Parenthesised tags: No-Intro and Redump attributes, and the GoodTools and
  // scene countries
  rest.clear();
  pos = 0;
  bool firstTag = true;
  while (pos < name.size())
  {
    const size_t open = name.find('(', pos);
    if (open == std::string::npos)
    {
      rest += name.substr(pos);
      break;
    }
    const size_t close = name.find(')', open);
    if (close == std::string::npos)
    {
      rest += name.substr(pos, open - pos);
      break;
    }
    rest += name.substr(pos, open - pos);
    pos = close + 1;

    std::string tag = name.substr(open + 1, close - open - 1);
    StringUtils::Trim(tag);
    if (tag.empty())
      continue;
    const bool first = std::exchange(firstTag, false);
    const std::vector<std::string> parts = SplitTrim(tag, ',');

    std::vector<GameRegion> regions;
    if (AllRegions(parts, first, regions))
    {
      out.regions.insert(out.regions.end(), regions.begin(), regions.end());
      continue;
    }
    std::vector<std::string> values;
    if (AllLanguages(parts, values))
    {
      out.languages.insert(out.languages.end(), values.begin(), values.end());
      continue;
    }
    if (CNoIntroNaming::IsRevision(tag))
    {
      out.revision = tag;
      continue;
    }
    if (CNeoKobeNaming::IsDataDisk(tag))
    {
      out.dataDisk = true;
      continue;
    }
    if (CNoIntroNaming::ParseDisc(tag, out.disc, out.discs))
      continue;
    if (const std::optional<ReleaseStatus> status = CNoIntroNaming::Status(tag))
    {
      out.status = *status;
      continue;
    }
    if (const std::optional<Licence> licence = CNoIntroNaming::LicenceOf(tag))
    {
      out.licence = *licence;
      continue;
    }
    if (CNoIntroNaming::IsAlternate(tag))
    {
      out.alternate = true;
      continue;
    }
    if (CNeoKobeNaming::IsHack(tag))
    {
      out.hack = true;
      continue;
    }
    // A prototype's build date: "(Proto) (1999-10-07)"
    if (const std::optional<TOSEC::Date> date = CTosecNaming::ParseDate(tag))
    {
      out.year = date->year;
      continue;
    }
    if (CNoIntroNaming::IsFeatureTag(tag) || CGoodToolsNaming::IsLanguageCount(tag))
      continue;
    out.unknownTags.emplace_back(tag);
  }

  std::string title = rest;
  StringUtils::Replace(title, "  ", " ");
  StringUtils::Trim(title, " -_");
  out.title = title;
  out.displayTitle = DisplayTitle(title);
  return out;
}

ParsedGameName CGameNameParser::ParsePath(const std::string& path)
{
  const CURL url(path);
  if (URIUtils::HasParentInHostname(url) && url.GetFileName().empty())
    return ParsePath(url.GetHostName());

  return Parse(URIUtils::IsURL(path) ? URIUtils::GetDecodedFileName(path)
                                     : URIUtils::GetFileName(path));
}

std::vector<std::string> CGameNameParser::Credits(const ParsedGameName& name)
{
  // Only Neo Kobe's names credit a company, and they give no region or year
  if (name.tosec || !name.regions.empty() || name.year != 0 || name.unknownTags.size() != 1)
    return {};
  return CNeoKobeNaming::Credits(name.unknownTags.front());
}

bool CGameNameParser::SameCompany(std::string_view a, std::string_view b)
{
  const std::vector<std::string> wordsA = CompanyWords(a);
  const std::vector<std::string> wordsB = CompanyWords(b);
  if (wordsA.empty() || wordsB.empty())
    return true;

  for (const std::string& word : wordsA)
  {
    if (std::ranges::find(wordsB, word) != wordsB.end())
      return true;
  }

  const std::string joinedA = StringUtils::Join(wordsA, "");
  const std::string joinedB = StringUtils::Join(wordsB, "");
  return std::min(joinedA.size(), joinedB.size()) >= 4 &&
         (joinedA.find(joinedB) != std::string::npos || joinedB.find(joinedA) != std::string::npos);
}
