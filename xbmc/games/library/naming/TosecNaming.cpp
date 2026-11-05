/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TosecNaming.h"

#include "language/i18n/Iso639_1.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <utility>

using namespace KODI;
using namespace GAME;
using namespace GAME::TOSEC;

namespace
{
// The convention's country codes: ISO 3166-1 alpha-2's, CS and YU among them
// from before ISO withdrew them, and UK, which sets write for GB
constexpr auto countries = std::to_array<std::string_view>({
    "AE", "AL", "AS", "AT", "AU", "BA", "BE", "BG", "BR", "CA", "CH", "CL", "CN", "CS",
    "CY", "CZ", "DE", "DK", "EE", "EG", "ES", "EU", "FI", "FR", "GB", "GR", "HK", "HR",
    "HU", "ID", "IE", "IL", "IN", "IR", "IS", "IT", "JO", "JP", "KR", "LT", "LU", "LV",
    "MN", "MX", "MY", "NL", "NO", "NP", "NZ", "OM", "PE", "PH", "PL", "PT", "QA", "RO",
    "RU", "SE", "SG", "SI", "SK", "TH", "TR", "TW", "UK", "US", "VN", "YU", "ZA",
});

// The codes whose meaning isn't BCP 47's: the convention's EU and AS are UN
// M.49's Europe and Asia, not the European Union and American Samoa
constexpr auto ownCodes = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"AS", "142"},
    {"EU", "150"},
    {"UK", "GB"},
});

constexpr auto demos = std::to_array<std::pair<std::string_view, Demo>>({
    {"demo", Demo::DEMO},
    {"demo-kiosk", Demo::KIOSK},
    {"demo-playable", Demo::PLAYABLE},
    {"demo-rolling", Demo::ROLLING},
    {"demo-slideshow", Demo::SLIDESHOW},
});

constexpr auto systems = std::to_array<std::string_view>(
    {// ZX Spectrum
     "+2", "+2a", "+3",
     // Atari 8-bit
     "130XE",
     // Atari ST
     "Mega ST", "Mega-STE", "ST", "STE", "TT",
     // Commodore Amiga
     "A1000", "A1200", "A1200-A4000", "A2000", "A2000-A3000", "A2024", "A2500-A3000UX", "A3000",
     "A4000", "A4000T", "A500", "A500+", "A500-A1000-A2000", "A500-A1000-A2000-CDTV", "A500-A1200",
     "A500-A1200-A2000-A4000", "A500-A2000", "A500-A600-A2000", "A570", "A600", "A600HD", "AGA",
     "AGA-CD32", "CD32", "CDTV", "ECS", "ECS-AGA", "OCS", "OCS-AGA",
     // Nintendo NES
     "Aladdin Deck Enhancer", "Computrainer", "Doctor PC Jr.", "PlayChoice-10", "VS DualSystem",
     "VS UniSystem",
     // Osborne 1 & Executive, spelled as the convention spells it
     "Executive", "Osbourne 1",
     // Microkey Primo
     "Primo-A", "Primo-A64", "Primo-B", "Primo-B64", "Pro-Primo",
     // MSX
     "TURBO-R GT", "TURBO-R ST",
     // Not assigned to a platform by the convention
     "ORCH80", "PIANO90", "Plus4", "STE-Falcon"});

constexpr auto videos =
    std::to_array<std::string_view>({"CGA", "EGA", "HGC", "MCGA", "MDA", "NTSC", "NTSC-PAL", "PAL",
                                     "PAL-60", "PAL-NTSC", "SVGA", "VGA", "XGA"});

constexpr auto copyrights = std::to_array<std::pair<std::string_view, Copyright>>({
    {"CW", Copyright::CARDWARE},
    {"CW-R", Copyright::CARDWARE_REGISTERED},
    {"FW", Copyright::FREEWARE},
    {"GW", Copyright::GIFTWARE},
    {"GW-R", Copyright::GIFTWARE_REGISTERED},
    {"LW", Copyright::LICENCEWARE},
    {"PD", Copyright::PUBLIC_DOMAIN},
    {"SW", Copyright::SHAREWARE},
    {"SW-R", Copyright::SHAREWARE_REGISTERED},
});

constexpr auto statuses = std::to_array<std::pair<std::string_view, DevelopmentStatus>>({
    {"alpha", DevelopmentStatus::ALPHA},
    {"beta", DevelopmentStatus::BETA},
    {"preview", DevelopmentStatus::PREVIEW},
    {"pre-release", DevelopmentStatus::PRE_RELEASE},
    {"proto", DevelopmentStatus::PROTO},
});

constexpr auto mediaTypes =
    std::to_array<std::string_view>({"Disc", "Disk", "File", "Part", "Side", "Tape"});

// "tr" comes before "t", which it starts with
constexpr auto dumpFlags = std::to_array<std::pair<std::string_view, DumpFlagType>>({
    {"cr", DumpFlagType::CRACKED},
    {"f", DumpFlagType::FIXED},
    {"h", DumpFlagType::HACKED},
    {"m", DumpFlagType::MODIFIED},
    {"p", DumpFlagType::PIRATED},
    {"tr", DumpFlagType::TRANSLATED},
    {"t", DumpFlagType::TRAINED},
    {"o", DumpFlagType::OVER_DUMP},
    {"u", DumpFlagType::UNDER_DUMP},
    {"v", DumpFlagType::VIRUS},
    {"b", DumpFlagType::BAD_DUMP},
    {"a", DumpFlagType::ALTERNATE_DUMP},
    {"!", DumpFlagType::VERIFIED},
});

template<typename T, size_t N>
const T* Find(const std::array<std::pair<std::string_view, T>, N>& table, std::string_view key)
{
  for (const auto& [k, v] : table)
  {
    if (k == key)
      return &v;
  }
  return nullptr;
}

template<size_t N>
bool Contains(const std::array<std::string_view, N>& list, std::string_view value)
{
  return std::ranges::find(list, value) != list.end();
}

bool IsDigits(std::string_view s)
{
  return !s.empty() &&
         std::ranges::all_of(s, [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
}

// A part of a date: its digits, with "x" where it isn't known
bool ReadDatePart(std::string_view part, size_t length, int& value)
{
  if (part.size() != length)
    return false;
  value = 0;
  bool known = true;
  for (const char c : part)
  {
    if (c == 'x' || c == 'X')
      known = false;
    else if (std::isdigit(static_cast<unsigned char>(c)))
      value = value * 10 + (c - '0');
    else
      return false;
  }
  if (!known)
    value = 0;
  return true;
}

std::vector<std::string> SplitCodes(std::string_view field)
{
  return StringUtils::Split(std::string(field), '-');
}

bool IsCapitalised(std::string_view code)
{
  return code.size() == 2 && std::isupper(static_cast<unsigned char>(code[0])) &&
         std::islower(static_cast<unsigned char>(code[1]));
}

// Sets write some languages "De" rather than "de"
std::string LanguageCode(const std::string& code)
{
  const std::string lower = IsCapitalised(code) ? StringUtils::ToLower(code) : code;
  return CTosecNaming::IsLanguage(lower) ? lower : "";
}

bool ReadCountries(std::string_view field, std::vector<std::string>& out)
{
  std::vector<std::string> codes = SplitCodes(field);
  for (std::string& code : codes)
  {
    // and a country "Jp", which is read as one where it can't be a language
    if (IsCapitalised(code) && LanguageCode(code).empty())
      StringUtils::ToUpper(code);
    if (!CTosecNaming::Region(code))
      return false;
  }
  out = std::move(codes);
  return true;
}

bool ReadLanguages(std::string_view field, Name& name)
{
  if (field.size() > 1 && field[0] == 'M' && IsDigits(field.substr(1)))
  {
    // A count too large to be one is left unread
    const std::string_view count = field.substr(1);
    std::from_chars(count.data(), count.data() + count.size(), name.languageCount);
    return true;
  }
  std::vector<std::string> codes = SplitCodes(field);
  for (std::string& code : codes)
  {
    code = LanguageCode(code);
    if (code.empty())
      return false;
  }
  name.languages = std::move(codes);
  name.languageCount = static_cast<int>(name.languages.size());
  return true;
}

// "Disk 1 of 2", "Disk 06 of 13", "Side B", "Side 1 of 2", "Part 1-2 of 3", "Tape 2 of 2 Side B"
bool ReadMedia(std::string_view field, Media& media)
{
  std::vector<std::string> words = StringUtils::Split(std::string(field), ' ');
  if (words.size() < 2 || !Contains(mediaTypes, words[0]))
    return false;

  auto isNumber = [](const std::string& w)
  {
    const std::vector<std::string> range = StringUtils::Split(w, '-');
    return range.size() <= 2 &&
           std::ranges::all_of(range, [](const std::string& n) { return IsDigits(n); });
  };
  auto isSide = [](const std::string& w)
  { return w.size() == 1 && std::isupper(static_cast<unsigned char>(w[0])); };

  Media read;
  read.type = words[0];
  if (!isNumber(words[1]) && !(read.type == "Side" && isSide(words[1])))
    return false;
  read.number = words[1];

  size_t i = 2;
  if (i + 1 < words.size() && words[i] == "of" && IsDigits(words[i + 1]))
  {
    read.total = words[i + 1];
    i += 2;
  }
  if (i + 1 < words.size() && words[i] == "Side" &&
      (isSide(words[i + 1]) || IsDigits(words[i + 1])))
  {
    read.side = words[i + 1];
    i += 2;
  }
  if (i != words.size())
    return false;

  media = std::move(read);
  return true;
}

std::optional<DumpFlag> ReadDumpFlag(std::string_view flag)
{
  for (const auto& [prefix, type] : dumpFlags)
  {
    if (!flag.starts_with(prefix))
      continue;

    std::string_view rest = flag.substr(prefix.size());
    DumpFlag read;
    read.type = type;
    size_t digits = 0;
    while (digits < rest.size() && std::isdigit(static_cast<unsigned char>(rest[digits])))
      ++digits;
    if (digits > 0)
    {
      std::from_chars(rest.data(), rest.data() + digits, read.number);
      rest.remove_prefix(digits);
    }
    if (!rest.empty() && rest[0] != ' ')
      return std::nullopt;

    read.detail = rest;
    StringUtils::Trim(read.detail);
    return read;
  }
  return std::nullopt;
}

// The fields after the publisher, in the order the convention writes them
enum class FlagField
{
  SYSTEM,
  VIDEO,
  COUNTRY,
  LANGUAGE,
  COPYRIGHT,
  STATUS,
  MEDIA,
};
constexpr size_t FIELD_COUNT = static_cast<size_t>(FlagField::MEDIA) + 1;

bool ReadField(FlagField field, std::string_view text, Name& name)
{
  switch (field)
  {
    case FlagField::SYSTEM:
      if (!Contains(systems, text))
        return false;
      name.system = text;
      return true;
    case FlagField::VIDEO:
      if (!Contains(videos, text))
        return false;
      name.video = text;
      return true;
    case FlagField::COUNTRY:
      return ReadCountries(text, name.countries);
    case FlagField::LANGUAGE:
      return ReadLanguages(text, name);
    case FlagField::COPYRIGHT:
      if (const Copyright* copyright = Find(copyrights, text))
      {
        name.copyright = *copyright;
        return true;
      }
      return false;
    case FlagField::STATUS:
      if (const DevelopmentStatus* status = Find(statuses, text))
      {
        name.status = *status;
        return true;
      }
      return false;
    case FlagField::MEDIA:
      return ReadMedia(text, name.media);
  }
  return false;
}

bool IsVersion(std::string_view text)
{
  return (text.size() > 1 && text[0] == 'v' && std::isdigit(static_cast<unsigned char>(text[1]))) ||
         (text.size() > 4 && text.starts_with("Rev "));
}

// "Legend of TOSEC, The v1.03b" -> "Legend of TOSEC, The" and "v1.03b"
void ReadVersion(std::string& title, std::string& version)
{
  const size_t space = title.rfind(' ');
  if (space == std::string::npos)
    return;

  const std::string last = title.substr(space + 1);
  if (IsVersion(last))
  {
    version = last;
    title.erase(space);
  }
  else if (space >= 4 && title.compare(space - 4, 5, " Rev ") == 0 && !last.empty())
  {
    version = title.substr(space - 3);
    title.erase(space - 4);
  }
  StringUtils::TrimRight(title);
}
} // namespace

std::optional<Name> CTosecNaming::Parse(std::string_view text)
{
  struct Group
  {
    char open;
    std::string text;
    bool closed;
    size_t end; // the position after it
  };

  const size_t first = text.find_first_of("([");
  if (first == std::string_view::npos)
    return std::nullopt;

  // Text between or after the fields is passed over: sets add a tape number
  // after the last one, "(Electric Dreams Software)2"
  std::vector<Group> groups;
  for (size_t pos = first; pos < text.size();)
  {
    const size_t open = text.find_first_of("([", pos);
    if (open == std::string_view::npos)
      break;
    const char close = text[open] == '(' ? ')' : ']';
    const size_t end = text.find(close, open + 1);
    if (end == std::string_view::npos)
    {
      // Cut short inside a field: the fields before it still stand
      groups.push_back({text[open], std::string(text.substr(open + 1)), false, text.size()});
      break;
    }
    groups.push_back(
        {text[open], std::string(text.substr(open + 1, end - open - 1)), true, end + 1});
    pos = end + 1;
  }

  // The date, written against the publisher: "(1991)(Softdisk Publishing)".
  // Sets put other tags before it, "Catacomb (v1.3) (1992)(Softdisk
  // Publishing)", and a space after it, "Tunnel B1 (1996) (Acclaim)". A date
  // followed by a space is only taken for TOSEC's when every tag before it is
  // one of TOSEC's fields, or a No-Intro prototype's build date would be.
  size_t date = 0;
  bool against = false;
  for (; date < groups.size(); ++date)
  {
    const Group& group = groups[date];
    if (!group.closed || group.open != '(')
      return std::nullopt;
    const size_t next = text.find_first_not_of(' ', group.end);
    if (ParseDate(group.text) && next != std::string_view::npos && text[next] == '(')
    {
      against = next == group.end;
      break;
    }
  }
  if (date == groups.size())
    return std::nullopt;

  Name name;
  name.title = text.substr(0, first);
  StringUtils::Trim(name.title);
  if (name.title.empty())
    return std::nullopt;
  ReadVersion(name.title, name.version);
  name.date = *ParseDate(groups[date].text);

  std::vector<const Group*> fields;
  for (size_t g = 0; g < date; ++g)
  {
    const std::string& field = groups[g].text;
    Name probe;
    bool isField = false;
    for (size_t i = 0; i < FIELD_COUNT && !isField; ++i)
      isField = ReadField(static_cast<FlagField>(i), field, probe);

    if (const Demo* demo = Find(demos, field))
      name.demo = *demo;
    else if (IsVersion(field))
      name.version = field;
    else if (isField)
      fields.push_back(&groups[g]);
    else if (against)
      name.unknownTags.emplace_back(field);
    else
      return std::nullopt;
  }

  // A set that leaves the publisher out puts the next field in its place:
  // "Il Faut Fuir Thesee (1984)(Fr)(Disk 1 of 1 Side A)"
  size_t g = date + 1;
  Name probe;
  if (g < groups.size() && groups[g].closed &&
      !ReadField(FlagField::COUNTRY, groups[g].text, probe) &&
      !ReadField(FlagField::LANGUAGE, groups[g].text, probe))
  {
    if (groups[g].text != "-")
    {
      for (std::string& publisher : StringUtils::Split(groups[g].text, " - "))
      {
        StringUtils::Trim(publisher);
        if (!publisher.empty())
          name.publishers.emplace_back(std::move(publisher));
      }
    }
    ++g;
  }

  // Sets don't always keep the fields before the dump flags
  for (; g < groups.size() && groups[g].closed; ++g)
  {
    if (groups[g].open == '(')
    {
      fields.push_back(&groups[g]);
      continue;
    }
    if (std::optional<DumpFlag> flag = ReadDumpFlag(groups[g].text))
      name.dumpFlags.emplace_back(std::move(*flag));
    else
      name.moreInfo.emplace_back(groups[g].text);
  }

  // Each optional field is read once. One that no field takes is the media
  // label if it's the last.
  std::array<bool, FIELD_COUNT> read{};
  for (size_t f = 0; f < fields.size(); ++f)
  {
    const std::string& field = fields[f]->text;
    bool taken = false;
    for (size_t i = 0; i < read.size() && !taken; ++i)
    {
      if (!read[i] && ReadField(static_cast<FlagField>(i), field, name))
        read[i] = taken = true;
    }
    if (taken)
      continue;
    if (f + 1 == fields.size())
      name.mediaLabel = field;
    else
      name.unknownTags.emplace_back(field);
  }

  return name;
}

std::optional<Date> CTosecNaming::ParseDate(std::string_view field)
{
  const std::vector<std::string> parts = StringUtils::Split(std::string(field), '-');
  if (parts.empty() || parts.size() > 3)
    return std::nullopt;

  // The century is always written: "198x", "19xx", never "1xxx"
  const std::string& year = parts[0];
  if (year.size() != 4 || !(year.starts_with("19") || year.starts_with("20")))
    return std::nullopt;

  Date date;
  if (!ReadDatePart(year, 4, date.year))
    return std::nullopt;
  if (parts.size() > 1 && !ReadDatePart(parts[1], 2, date.month))
    return std::nullopt;
  if (parts.size() > 2 && !ReadDatePart(parts[2], 2, date.day))
    return std::nullopt;
  return date;
}

std::optional<GameRegion> CTosecNaming::Region(std::string_view code)
{
  if (std::ranges::find(countries, code) == countries.end())
    return std::nullopt;
  if (const std::string_view* territory = Find(ownCodes, code))
    return GameRegion::Make(*territory);
  return GameRegion::Make(code);
}

bool CTosecNaming::IsLanguage(std::string_view code)
{
  return code.size() == 2 && std::islower(static_cast<unsigned char>(code[0])) &&
         std::islower(static_cast<unsigned char>(code[1])) &&
         LANGUAGE::I18N::CIso639_1::LookupByCode(code).has_value();
}
