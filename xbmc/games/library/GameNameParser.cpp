/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameNameParser.h"

#include "utils/RegExp.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr std::array<std::pair<std::string_view, std::string_view>, 54> regionNames{{
    {"usa", "USA"},
    {"us", "USA"},
    {"u", "USA"},
    {"canada", "Canada"},
    {"europe", "Europe"},
    {"eu", "Europe"},
    {"e", "Europe"},
    {"japan", "Japan"},
    {"jp", "Japan"},
    {"j", "Japan"},
    {"world", "World"},
    {"w", "World"},
    {"asia", "Asia"},
    {"australia", "Australia"},
    {"brazil", "Brazil"},
    {"china", "China"},
    {"france", "France"},
    {"germany", "Germany"},
    {"italy", "Italy"},
    {"spain", "Spain"},
    {"netherlands", "Netherlands"},
    {"sweden", "Sweden"},
    {"korea", "Korea"},
    {"taiwan", "Taiwan"},
    {"hong kong", "Hong Kong"},
    {"uk", "United Kingdom"},
    {"united kingdom", "United Kingdom"},
    {"russia", "Russia"},
    {"poland", "Poland"},
    {"finland", "Finland"},
    {"denmark", "Denmark"},
    {"norway", "Norway"},
    {"portugal", "Portugal"},
    {"greece", "Greece"},
    {"israel", "Israel"},
    {"india", "India"},
    {"mexico", "Mexico"},
    {"argentina", "Argentina"},
    {"scandinavia", "Scandinavia"},
    {"latin america", "Latin America"},
    {"unknown", "Unknown"},
    // TOSEC's ISO 3166 country codes, apart from those that are also language
    // codes, such as "de" and "fr", which a language tag needs
    {"gb", "United Kingdom"},
    {"au", "Australia"},
    {"se", "Sweden"},
    {"dk", "Denmark"},
    {"gr", "Greece"},
    {"br", "Brazil"},
    {"mx", "Mexico"},
    {"kr", "Korea"},
    {"cn", "China"},
    {"tw", "Taiwan"},
    {"hk", "Hong Kong"},
    {"in", "India"},
    {"il", "Israel"},
}};

constexpr std::array<std::string_view, 40> languageCodes{
    "en", "ja", "fr", "de", "es", "it", "nl", "pt", "sv", "no", "da", "fi", "zh", "ko",
    "pl", "ru", "cs", "hu", "el", "tr", "ar", "he", "ca", "th", "hr", "ro", "bg", "uk",
    "sk", "sl", "lt", "lv", "et", "ga", "eu", "gl", "id", "ms", "vi", "fa",
};

constexpr std::array<std::string_view, 15> noiseTags{
    "gb compatible",
    "sgb enhanced",
    "enhancement chip",
    "rumble version",
    "virtual console",
    "nintendo power",
    "np",
    "st",
    "mb",
    "wii virtual console",
    "3ds virtual console",
    "wii u virtual console",
    "switch online",
    "lock-on",
    "en,ja",
};

constexpr std::array<std::string_view, 7> articles{"the", "a", "an", "le", "la", "les", "der"};

// GoodTools names a translation's language in three letters: "[T+Ger]"
constexpr std::array<std::pair<std::string_view, std::string_view>, 26> translationLanguages{{
    {"ara", "ar"}, {"bra", "pt"}, {"bul", "bg"}, {"cat", "ca"}, {"chi", "zh"}, {"cze", "cs"},
    {"dan", "da"}, {"dut", "nl"}, {"eng", "en"}, {"fin", "fi"}, {"fre", "fr"}, {"ger", "de"},
    {"gre", "el"}, {"heb", "he"}, {"hun", "hu"}, {"ita", "it"}, {"jap", "ja"}, {"kor", "ko"},
    {"nor", "no"}, {"pol", "pl"}, {"por", "pt"}, {"rom", "ro"}, {"rus", "ru"}, {"spa", "es"},
    {"swe", "sv"}, {"tur", "tr"},
}};

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

bool AllRegions(const std::vector<std::string>& parts, std::vector<std::string>& codes)
{
  codes.clear();
  for (const std::string& p : parts)
  {
    const std::string name = CGameNameParser::RegionName(Lower(p));
    if (name.empty())
      return false;
    codes.emplace_back(name);
  }
  return !codes.empty();
}

bool AllLanguages(const std::vector<std::string>& parts, std::vector<std::string>& codes)
{
  codes.clear();
  for (const std::string& p : parts)
  {
    const std::string code = Lower(p);
    if (code.size() != 2)
      return false;
    bool known = false;
    for (std::string_view lang : languageCodes)
    {
      if (lang == code)
      {
        known = true;
        break;
      }
    }
    if (!known)
      return false;
    codes.emplace_back(code);
  }
  return !codes.empty();
}

bool Matches(CRegExp& re, const std::string& text)
{
  return re.RegFind(text) >= 0;
}

struct Patterns
{
  CRegExp revision{true};
  CRegExp disc{true};
  CRegExp dataDisk{true};
  CRegExp tosecYear{true};
  CRegExp beta{true};
  CRegExp proto{true};
  CRegExp sample{true};
  CRegExp demo{true};
  CRegExp alpha{true};
  CRegExp debug{true};
  CRegExp program{true};
  CRegExp alt{true};
  CRegExp alternateSet{true};
  CRegExp extension{true};
  CRegExp sceneNumber;

  Patterns()
  {
    revision.RegComp("^(rev|revision|version|v)\\.? ?([0-9][0-9a-z.]*|[a-z])$");
    disc.RegComp(
        "^(disc|disk|side|tape|cd|cart|part) ?([0-9]{1,4}[a-z]?|[a-z])( of ([0-9]{1,4}))?$");
    dataDisk.RegComp(
        "^(.+ )?(data|user|save|scenario|music|sound|map|character|backup|utility) dis[ck]( .+)?$");
    tosecYear.RegComp("^(19|20)([0-9x?]{2})(-[0-9]{2}(-[0-9]{2})?)?$");
    beta.RegComp("^beta( ?[0-9]+)?$");
    proto.RegComp("^proto(type)?( ?[0-9]+)?$");
    sample.RegComp("^sample$");
    demo.RegComp("^(demo|kiosk|preview|trial|taikenban)\\b");
    alpha.RegComp("^alpha$");
    debug.RegComp("^debug( version)?$");
    program.RegComp("^(check |sample |test )?program$");
    alt.RegComp("^alt( ?[0-9]+)?$");
    alternateSet.RegComp("^(alt|set) ?[0-9]+$");
    extension.RegComp("\\.[a-z0-9]{1,4}$");
    // A scene set numbers every release, "2797 Kimi no Yusha (JP)". Its
    // two-letter region and its lack of a date tell it from a title that starts
    // with a number, "1943 Kai (Japan)" or TOSEC's "1942 (1986)(Elite)(GB)".
    sceneNumber.RegComp("^[0-9]{4} +(?=.*\\((?:JP|EU|US|FR|KS|DE|IT|ES|NL|AU)\\))"
                        "(?!.*\\([0-9]{4}\\))");
  }
};

Patterns& GetPatterns()
{
  // CRegExp keeps the last match, so each thread needs its own
  thread_local Patterns patterns;
  return patterns;
}

// Words that say what kind of company a name is rather than which one
constexpr std::array<std::string_view, 12> COMPANY_NOISE = {
    "soft",        "software", "co",    "ltd",  "inc", "corp",
    "corporation", "kk",       "games", "game", "the", "doujin"};

std::vector<std::string> CompanyWords(std::string_view company)
{
  std::vector<std::string> words;
  std::string word;
  for (const char c : company)
  {
    if (std::isalnum(static_cast<unsigned char>(c)))
    {
      word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      continue;
    }
    if (!word.empty() && std::ranges::find(COMPANY_NOISE, word) == COMPANY_NOISE.end())
      words.emplace_back(word);
    word.clear();
  }
  if (!word.empty() && std::ranges::find(COMPANY_NOISE, word) == COMPANY_NOISE.end())
    words.emplace_back(word);
  return words;
}
} // namespace

std::string CGameNameParser::RegionName(std::string_view region)
{
  for (const auto& [alias, name] : regionNames)
  {
    if (alias == region)
      return std::string(name);
  }
  return "";
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
  const std::string lower = Lower(article);
  for (std::string_view a : articles)
  {
    if (a == lower)
      return article + " " + head.substr(0, comma) + tail;
  }
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
  Patterns& re = GetPatterns();
  ParsedGameName out;

  if (const std::string whdload = ParseWhdLoadName(fileName); !whdload.empty())
  {
    out.title = whdload;
    out.displayTitle = DisplayTitle(whdload);
    return out;
  }

  std::string name(fileName);
  if (hasExtension && Matches(re.extension, Lower(name)))
    name.erase(name.size() - re.extension.GetFindLen());
  if (Matches(re.sceneNumber, name))
    name.erase(0, re.sceneNumber.GetFindLen());

  // Neo Kobe gives how to load a game in braces: "Apploon {V1 mode, MON R G8F00}"
  for (size_t open = name.find('{'); open != std::string::npos; open = name.find('{', open))
  {
    const size_t close = name.find('}', open);
    name.erase(open, close == std::string::npos ? std::string::npos : close - open + 1);
  }

  // Bracket tags: GoodTools and TOSEC dump flags
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
    const std::string tl = Lower(tag);

    if (tl == "!")
      out.verified = true;
    else if (tl.starts_with("t+") || tl.starts_with("t-"))
    {
      // "[T+Ger1.00_Translator]"
      out.translated = true;
      const std::string code = tl.substr(2, 3);
      for (const auto& [goodTools, iso] : translationLanguages)
      {
        if (goodTools == code)
          out.translation = iso;
      }
    }
    else if ((!tl.empty() && tl[0] == 'b' &&
              tl.find_first_not_of("0123456789", 1) == std::string::npos) ||
             tl == "bad" || tl.starts_with("bad "))
      out.bad = true;
    // Neo Kobe names another dump of the same disks "[Set 2]" or "[Alt 1]"
    else if ((!tl.empty() && tl[0] == 'a' &&
              tl.find_first_not_of("0123456789", 1) == std::string::npos) ||
             Matches(re.alternateSet, tl))
      out.alternate = true;
    else if (!tl.empty() && tl[0] == 'h' &&
             tl.find_first_not_of("0123456789", 1) == std::string::npos)
      out.hack = true;
    else if (!tl.empty() && tl[0] == 'p' &&
             tl.find_first_not_of("0123456789", 1) == std::string::npos)
      out.licence = Licence::PIRATE;
    else if (tl == "bios")
      out.status = ReleaseStatus::PROGRAM;
    // Any other flag describes the dump ([FD], [extras]), not who made it, so
    // it isn't kept with the unknown tags that can credit a company

    pos = close + 1;
  }
  name = rest;

  // Parenthesised tags: No-Intro, Redump and TOSEC attributes
  rest.clear();
  pos = 0;
  bool publisherRead = false;
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
    const std::string tl = Lower(tag);
    const std::vector<std::string> parts = SplitTrim(tag, ',');

    std::vector<std::string> codes;
    if (AllRegions(parts, codes))
    {
      out.regions.insert(out.regions.end(), codes.begin(), codes.end());
      continue;
    }
    if (AllLanguages(parts, codes))
    {
      out.languages.insert(out.languages.end(), codes.begin(), codes.end());
      continue;
    }
    if (Matches(re.revision, tl))
    {
      out.revision = tag;
      continue;
    }
    if (Matches(re.dataDisk, tl))
    {
      out.dataDisk = true;
      continue;
    }
    if (Matches(re.disc, tl))
    {
      const std::string number = re.disc.GetMatch(2);
      const std::string total = re.disc.GetMatch(4);
      out.disc =
          std::isdigit(static_cast<unsigned char>(number[0]))
              ? std::stoi(number)
              : static_cast<int>(std::tolower(static_cast<unsigned char>(number[0])) - 'a' + 1);
      out.discs = total.empty() ? 0 : std::stoi(total);
      continue;
    }
    if (Matches(re.beta, tl))
    {
      out.status = ReleaseStatus::BETA;
      continue;
    }
    if (Matches(re.proto, tl))
    {
      out.status = ReleaseStatus::PROTOTYPE;
      continue;
    }
    if (Matches(re.sample, tl))
    {
      out.status = ReleaseStatus::SAMPLE;
      continue;
    }
    if (Matches(re.demo, tl))
    {
      out.status = ReleaseStatus::DEMO;
      continue;
    }
    if (Matches(re.alpha, tl))
    {
      out.status = ReleaseStatus::ALPHA;
      continue;
    }
    if (Matches(re.debug, tl))
    {
      out.status = ReleaseStatus::DEBUG;
      continue;
    }
    if (Matches(re.program, tl))
    {
      out.status = ReleaseStatus::PROGRAM;
      continue;
    }
    if (tl == "unl" || tl == "unlicensed")
    {
      out.licence = Licence::UNLICENSED;
      continue;
    }
    if (tl == "pirate")
    {
      out.licence = Licence::PIRATE;
      continue;
    }
    if (tl == "aftermarket")
    {
      out.licence = Licence::AFTERMARKET;
      continue;
    }
    if (tl == "homebrew")
    {
      out.licence = Licence::HOMEBREW;
      continue;
    }
    if (Matches(re.alt, tl))
    {
      out.alternate = true;
      continue;
    }
    if (tl == "hack" || tl == "hacked" || tl.ends_with(" hack"))
    {
      out.hack = true;
      continue;
    }
    if (Matches(re.tosecYear, tl))
    {
      out.tosec = true;
      if (std::isdigit(static_cast<unsigned char>(tl[2])) &&
          std::isdigit(static_cast<unsigned char>(tl[3])))
        out.year = std::stoi(tl.substr(0, 4));
      continue;
    }
    bool noise = false;
    for (std::string_view n : noiseTags)
    {
      if (n == tl)
      {
        noise = true;
        break;
      }
    }
    if (noise)
      continue;
    // TOSEC: the tag after the year names the publisher, "-" when it's unknown
    if (out.tosec && !publisherRead && !std::isdigit(static_cast<unsigned char>(tag[0])))
    {
      publisherRead = true;
      out.publisher = (tag == "-") ? "" : tag;
      continue;
    }
    out.unknownTags.emplace_back(tag);
  }

  std::string title = rest;
  StringUtils::Replace(title, "  ", " ");
  StringUtils::Trim(title, " -_");
  out.title = title;
  out.displayTitle = DisplayTitle(title);
  return out;
}

std::vector<std::string> CGameNameParser::Credits(const ParsedGameName& name)
{
  if (name.tosec || name.unknownTags.size() != 1)
    return {};

  std::string tag = name.unknownTags.front();
  if (StringUtils::StartsWithNoCase(tag, "doujin"))
  {
    tag.erase(0, 6);
    StringUtils::TrimLeft(tag, " -");
  }

  std::vector<std::string> credits;
  for (std::string part : StringUtils::Split(tag, " - "))
  {
    StringUtils::Trim(part);
    if (!part.empty())
      credits.emplace_back(std::move(part));
  }
  return credits;
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
