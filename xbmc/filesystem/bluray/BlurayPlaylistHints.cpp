/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BlurayPlaylistHints.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace XFILE
{
namespace
{
/*!
 \brief The whole feature - FPL_MainFeature, FPL_MainFeature_EXT, SEG_MainFeature.
 Not one of the numbered segments a feature is sometimes assembled from.
 */
bool IsFeature(std::string_view name)
{
  // FPL_MainFeature and the variants that follow it - _EXT, _Narrative, _EXT_Narrative, and one
  // per language. The prefix is not always FPL_, as "SEG FPL_MainFeature" also occurs, so this
  // looks for the name rather than testing the start of it.
  if (name.find("FPL_MainFeature") != std::string_view::npos)
    return true;

  // SEG_MainFeature on its own is the whole presentation too. Suffixed it is not - a disc that
  // assembles its feature from parts numbers them SEG_MainFeature_01, _EXT_12, _TH_23 and so on,
  // and those are minutes long where the feature is hours.
  return name == "SEG_MainFeature";
}

/*! \brief An episode of a series - EPL_01, SEG_EPL_02. */
bool IsEpisode(std::string_view name)
{
  return name.starts_with("EPL_") || name.starts_with("SEG_EPL_");
}

/*!
 \brief A special feature - SF_Inside_Derry_102, SEG_SF_BTV_Power_Of_Sound, SEG SF_01_Finding.
 */
bool IsSpecialFeature(std::string_view name)
{
  return name.starts_with("SF_") || name.starts_with("SEG_SF_") || name.starts_with("SEG SF_");
}

/*!
 \brief Front matter - a studio ident, piracy or copyright warning, age certificate or
 disclaimer, shown before anything is played.
 */
bool IsWarningOrLogo(std::string_view name)
{
  // Studios name these a dozen ways, so matching a single prefix covers only one of them
  static constexpr std::array MARKERS{"Warn", "Disclaimer", "Logo", "Parental", " AGE "};
  if (std::ranges::any_of(MARKERS, [name](std::string_view marker)
                          { return name.find(marker) != std::string_view::npos; }))
    return true;

  // The same thing under names that do not say so - an anti-piracy warning, a studio ident, an
  // intellectual property notice, a certificate
  return name.starts_with("WRN_") || name.starts_with("FBI") || name.starts_with("Studio") ||
         name.starts_with("IPR") || name == "MPAA";
}

/*! \brief A menu background or transition rather than content. */
bool IsMenu(std::string_view name)
{
  return name.find("Menu") != std::string_view::npos || name.starts_with("TMPL");
}

struct EpisodeName
{
  unsigned int ordinal{0};

  //! Whether the name ends at the ordinal. EPL_02 is the episode.
  //! EPL_02_Narrative and EPL_02_JPN are the same episode described or dubbed.
  bool base{false};
};

//! \brief The ordinal in an episode playlist's name - the 2 of EPL_02, SEG_EPL_02, EPL_02_Narrative
std::optional<EpisodeName> ReadEpisodeName(std::string_view name)
{
  const size_t marker{name.find("EPL_")};
  if (marker == std::string_view::npos)
    return std::nullopt;

  EpisodeName episode;
  size_t i{marker + 4};
  size_t digits{0};
  for (; i < name.size() && name[i] >= '0' && name[i] <= '9'; ++i, ++digits)
    episode.ordinal = episode.ordinal * 10 + static_cast<unsigned int>(name[i] - '0');

  if (digits == 0)
    return std::nullopt;

  episode.base = i == name.size();
  return episode;
}

//! \brief The words of a name without its role prefix - SEG_SF_Becoming_Pennywise -> "Becoming Pennywise"
std::string GetTitle(std::string_view name)
{
  if (name.starts_with("SEG_") || name.starts_with("SEG "))
    name.remove_prefix(4);
  if (name.starts_with("SF_"))
    name.remove_prefix(3);

  std::string title{name};
  std::ranges::replace(title, '_', ' ');
  return title;
}

bool IsUpper(char c)
{
  return c >= 'A' && c <= 'Z';
}

bool IsLower(char c)
{
  return c >= 'a' && c <= 'z';
}

bool IsDigit(char c)
{
  return c >= '0' && c <= '9';
}

bool IsDigits(std::string_view token)
{
  return !token.empty() && std::ranges::all_of(token, IsDigit);
}

std::string ToLower(std::string_view text)
{
  std::string lowered{text};
  std::ranges::transform(lowered, lowered.begin(),
                         [](char c) { return IsUpper(c) ? static_cast<char>(c - 'A' + 'a') : c; });
  return lowered;
}

/*!
 \brief A language an extra is dubbed or subtitled into - _JPN, _jpn, _lang2.
 A word in title case is not one, as SF_01_Day shows.
 */
bool IsLanguage(std::string_view token)
{
  if (token.starts_with("lang") && IsDigits(token.substr(4)))
    return true;

  static constexpr std::array LANGUAGES{
      "ara", "ces", "chi", "cze", "dan", "deu", "dut", "ell", "eng", "fin", "fra",
      "fre", "ger", "gre", "heb", "hin", "hun", "ind", "ita", "jpn", "kor", "nld",
      "nor", "pol", "por", "rus", "spa", "swe", "tha", "tur", "ukr", "vie", "zho"};
  if (token.size() != 3 ||
      !(std::ranges::all_of(token, IsLower) || std::ranges::all_of(token, IsUpper)))
    return false;

  return std::ranges::find(LANGUAGES, ToLower(token)) != LANGUAGES.end();
}

//! \brief The variety of a language - _spa_CS, _fra_PF, _M_nld
bool IsLanguageQualifier(std::string_view token)
{
  return !token.empty() && token.size() <= 2 && std::ranges::all_of(token, IsUpper);
}

/*!
 \brief A code a disc orders or groups its extras by, rather than naming them - 01, DS, DA01, CH3.
 */
bool IsCode(std::string_view token)
{
  size_t i{0};
  while (i < token.size() && IsUpper(token[i]))
    ++i;
  if (i > 5)
    return false;
  while (i < token.size() && IsDigit(token[i]))
    ++i;
  return !token.empty() && i == token.size();
}

//! \brief The kind of extra a code in its name says it is - SF_DS_06_02_TakeOffShoes
ExtraGroup GetExtraGroup(std::string_view code)
{
  using CodeGroup = std::pair<std::string_view, ExtraGroup>;
  static constexpr std::array<CodeGroup, 8> GROUPS{{
      {"DS", ExtraGroup::DELETED_SCENES},
      {"MV", ExtraGroup::MUSIC_VIDEOS},
      {"SA", ExtraGroup::SING_ALONGS},
      {"TRLR", ExtraGroup::TRAILERS},
      {"COMM", ExtraGroup::COMMERCIALS},
      {"PROMO", ExtraGroup::PROMOS},
      {"BTS", ExtraGroup::BEHIND_THE_SCENES},
      {"CAST", ExtraGroup::CAST},
  }};

  const auto group{std::ranges::find(GROUPS, code, &CodeGroup::first)};
  return group != GROUPS.end() ? group->second : ExtraGroup::NONE;
}

//! \brief The words run together in a name - GagReel -> "Gag Reel", Trailer1 -> "Trailer 1"
std::string SplitWords(std::string_view token)
{
  // A zero-padded or single digit number leading a word only orders it - 02VideoGraphics, 5Prod
  size_t start{0};
  while (start < token.size() && IsDigit(token[start]))
    ++start;
  if (start == token.size() || !IsUpper(token[start]) || (start > 1 && token[0] != '0'))
    start = 0;

  std::string words;
  for (size_t i = start; i < token.size(); ++i)
  {
    const char c{token[i]};
    if (i > start)
    {
      const char previous{token[i - 1]};
      const bool nextIsLower{i + 1 < token.size() && IsLower(token[i + 1])};
      if ((IsLower(previous) && (IsUpper(c) || IsDigit(c))) || (IsDigit(previous) && IsUpper(c)) ||
          (IsUpper(previous) && IsUpper(c) && nextIsLower))
        words += ' ';
    }
    words += c;
  }
  return words;
}

struct ExtraName
{
  std::string title;
  ExtraGroup group{ExtraGroup::NONE};
  bool playAll{false};
  bool base{true};
};

/*!
 \brief What an extra is, from its name - SF_DS_06_02_TakeOffShoes is the deleted scene
 "Take Off Shoes".

 Copies of an extra share its title but are not its base presentation - a segment of it (SEG_),
 the same with a slate (_Binge, _Slate), or dubbed (_JPN, _M_nld, _spa_CS). _NCR marks no copy.
 */
ExtraName ReadExtraName(std::string_view name)
{
  ExtraName extra;
  if (name.starts_with("SEG_") || name.starts_with("SEG "))
  {
    name.remove_prefix(4);
    extra.base = false;
  }
  if (name.starts_with("SF_"))
    name.remove_prefix(3);

  std::vector<std::string> tokens;
  for (const auto part : std::views::split(name, '_'))
  {
    if (!part.empty())
      tokens.emplace_back(part.begin(), part.end());
  }

  bool language{false};
  while (tokens.size() > 1)
  {
    const std::string last{ToLower(tokens.back())};
    const bool copy{last == "binge" || last == "slate" || last == "cr"};
    const bool isLanguage{IsLanguage(tokens.back())};
    const bool qualifier{IsLanguageQualifier(tokens.back()) &&
                         (language || IsLanguage(tokens[tokens.size() - 2]))};
    if (!copy && !isLanguage && !qualifier && last != "ncr")
      break;

    if (copy || isLanguage)
      extra.base = false;
    language = language || isLanguage;
    tokens.pop_back();
  }

  for (std::string& token : tokens)
  {
    if (const size_t playAll{ToLower(token).find("playall")}; playAll != std::string::npos)
    {
      extra.playAll = true;
      token.erase(playAll, 7);
    }
  }
  std::erase_if(tokens, [](const std::string& token) { return token.empty(); });

  // The codes leading a name order or group the extras rather than name them
  std::vector<std::string> codes;
  auto token{tokens.begin()};
  for (; token != tokens.end() && IsCode(*token); ++token)
  {
    if (extra.group == ExtraGroup::NONE && GetExtraGroup(*token) != ExtraGroup::NONE)
      extra.group = GetExtraGroup(*token);
    else
      codes.emplace_back(*token);
  }

  std::vector<std::string> words;
  for (; token != tokens.end(); ++token)
  {
    if (!IsDigits(*token))
      words.emplace_back(SplitWords(*token));
  }

  // A name made of nothing but codes is known by them - SF_BTS, SF_DS_01
  if (words.empty())
  {
    std::ranges::copy_if(codes, std::back_inserter(words),
                         [](const std::string& code) { return !IsDigits(code); });
    if (words.empty() && !extra.playAll)
      words = codes;
  }

  for (const std::string& word : words)
  {
    if (!extra.title.empty())
      extra.title += ' ';
    extra.title += word;
  }
  return extra;
}
} // namespace

PlaylistRole GetProjectPlaylistRole(std::string_view name)
{
  if (IsFeature(name))
    return PlaylistRole::FEATURE;
  if (IsEpisode(name))
    return PlaylistRole::EPISODE;
  if (IsSpecialFeature(name))
    return PlaylistRole::SPECIAL;
  if (IsWarningOrLogo(name))
    return PlaylistRole::FRONT_MATTER;
  if (IsMenu(name))
    return PlaylistRole::MENU;
  return PlaylistRole::UNKNOWN;
}

CBlurayPlaylistHints::CBlurayPlaylistHints(const ProjectInformation& project)
  : m_present(project.present)
{
  for (const auto& [playlist, information] : project.playlists)
  {
    PlaylistHint hint{.role = GetProjectPlaylistRole(information.name), .name = information.name};

    switch (hint.role)
    {
      case PlaylistRole::FEATURE:
        // FPL_MainFeature_EXT, _Narrative and the language variants are not the plain feature
        hint.basePresentation = information.name.ends_with("MainFeature");
        break;
      case PlaylistRole::EPISODE:
        // A disc numbering its episodes some other way offers no ordinal, and cannot be matched
        // to them by number
        if (const std::optional<EpisodeName> episode{ReadEpisodeName(information.name)})
        {
          hint.ordinal = episode->ordinal;
          hint.basePresentation = episode->base;
        }
        break;
      case PlaylistRole::SPECIAL:
      {
        hint.title = GetTitle(information.name);
        ExtraName extra{ReadExtraName(information.name)};
        hint.extraTitle = std::move(extra.title);
        hint.extraGroup = extra.group;
        hint.playAll = extra.playAll;
        hint.basePresentation = extra.base;
        break;
      }
      default:
        break;
    }

    m_hints.emplace(playlist, std::move(hint));
  }
}
} // namespace XFILE
