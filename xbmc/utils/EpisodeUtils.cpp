/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpisodeUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "i18n/ListFormatter.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cctype>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace KODI;

namespace
{
bool GetEpisodeAndSeasonFromRegExp(CRegExp& reg, VIDEO::EPISODE& episodeInfo, int defaultSeason)
{
  std::string season(reg.GetMatch(1));
  std::string episode(reg.GetMatch(2));

  if (!season.empty() || !episode.empty())
  {
    char* endptr = NULL;
    if (season.empty() && !episode.empty())
    { // no season specified -> assume defaultSeason
      episodeInfo.iSeason = defaultSeason;
      if ((episodeInfo.iEpisode = CUtil::TranslateRomanNumeral(episode.c_str())) == -1)
        episodeInfo.iEpisode = strtol(episode.c_str(), &endptr, 10);
    }
    else if (!season.empty() && episode.empty())
    { // no episode specification -> assume defaultSeason
      episodeInfo.iSeason = defaultSeason;
      if ((episodeInfo.iEpisode = CUtil::TranslateRomanNumeral(season.c_str())) == -1)
        episodeInfo.iEpisode = atoi(season.c_str());
    }
    else
    { // season and episode specified
      episodeInfo.iSeason = atoi(season.c_str());
      episodeInfo.iEpisode = strtol(episode.c_str(), &endptr, 10);
    }
    if (endptr)
    {
      if (isalpha(*endptr))
        episodeInfo.iSubepisode = *endptr - (islower(*endptr) ? 'a' : 'A') + 1;
      else if (*endptr == '.')
        episodeInfo.iSubepisode = atoi(endptr + 1);
    }
    return true;
  }
  return false;
}

bool GetAirDateFromRegExp(CRegExp& reg, VIDEO::EPISODE& episodeInfo)
{
  std::string param1(reg.GetMatch(1));
  std::string param2(reg.GetMatch(2));
  std::string param3(reg.GetMatch(3));

  if (!param1.empty() && !param2.empty() && !param3.empty())
  {
    // regular expression by date
    int len1 = param1.size();
    int len2 = param2.size();
    int len3 = param3.size();

    if (len1 == 4 && len2 == 2 && len3 == 2)
    {
      // yyyy mm dd format
      episodeInfo.cDate.SetDate(atoi(param1.c_str()), atoi(param2.c_str()), atoi(param3.c_str()));
    }
    else if (len1 == 2 && len2 == 2 && len3 == 4)
    {
      // mm dd yyyy format
      episodeInfo.cDate.SetDate(atoi(param3.c_str()), atoi(param1.c_str()), atoi(param2.c_str()));
    }
  }
  return episodeInfo.cDate.IsValid();
}

bool GetEpisodeTitleFromRegExp(CRegExp& reg, VIDEO::EPISODE& episodeInfo)
{
  std::string param1(reg.GetMatch(1));

  if (!param1.empty())
  {
    episodeInfo.strTitle = param1;
    return true;
  }
  return false;
}

/**
 * \brief Maximum allowed number of episodes in a single episode range.
 *
 * The value 50 was chosen as a safety limit to prevent accidental processing of
 * extremely large episode ranges, which could be caused by malformed filenames or
 * incorrect regular expression matches. Note this is a per-file number and not the
 * total number of episodes in a season.
 */
constexpr int MAX_EPISODE_RANGE = 50;

// Character following season/episode range must be one of these for range to be valid.
constexpr std::string_view allowed{"-_.esx "};

/*! \brief Perform checks, then add episodes in a given range to the episode list
 \param first first episode in the range to add.
 \param last last episode in the range.
 \param episode the first episode in the range (already added to the list).
 \param episodeList the list (vector) of episodes to add to.
 \param regex the regex used to match the episode range.
 \param remainder the remainder of the filename after the episode range.
*/
void ProcessEpisodeRange(int first,
                         int last,
                         VIDEO::EPISODE& episode,
                         VIDEO::EPISODELIST& episodeList,
                         const std::string& regex,
                         const std::string& remainder)
{
  if (first > last && !episodeList.empty())
  {
    // SxxEaa-SxxEbb or Eaa-Ebb is backwards - bb<aa
    CLog::LogF(LOGDEBUG,
               "VideoInfoScanner: Removing season {}, episode {} as range {}-{} is backwards",
               episode.iSeason, episode.iEpisode, episodeList.back().iEpisode, last);
    episodeList.pop_back();
    return;
  }
  if ((last - first + 1) > MAX_EPISODE_RANGE && !episodeList.empty())
  {
    CLog::LogF(LOGDEBUG,
               "VideoInfoScanner: Removing season {}, episode {} as range is too large {} (maximum "
               "allowed {})",
               episode.iSeason, episode.iEpisode, last - first + 1, MAX_EPISODE_RANGE);
    episodeList.pop_back();
    return;
  }
  if (!remainder.empty() && allowed.find(static_cast<char>(std::tolower(static_cast<unsigned char>(
                                remainder.front())))) == std::string_view::npos)
  {
    CLog::LogF(LOGDEBUG,
               "VideoInfoScanner:Last episode in range {} is not part of an episode string ({}) "
               "- ignoring",
               last, remainder);
    return;
  }
  for (int e = first; e <= last; ++e)
  {
    episode.iEpisode = e;
    CLog::LogF(LOGDEBUG, "VideoInfoScanner: Adding multipart episode {} [{}]", episode.iEpisode,
               regex);
    episodeList.push_back(episode);
  }
}
} // namespace

bool CEpisodeUtils::EnumerateEpisodeItem(const CFileItem* item,
                                         VIDEO::EPISODELIST& episodeList,
                                         KODI::REGEXP::RegExpCache* cache /* = nullptr */)
{
  const auto advancedSettings{CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};
  const auto& tvShowRegExps{advancedSettings->m_tvshowEnumRegExps};

  // Remove path to main file if it's a bd or dvd folder to regex the right (folder) name
  std::string label;
  if (item->IsOpticalMediaFile())
  {
    label = item->GetLocalMetadataPath();
    URIUtils::RemoveSlashAtEnd(label);
  }
  else
    label = item->GetPath();

  // URLDecode in case an episode is on a http/https/dav/davs:// source and URL-encoded like foo%201x01%20bar.avi
  label = CURL::Decode(CURL::GetRedacted(label));

  // Pre-compile multi-part regex
  std::shared_ptr<CRegExp> multiPartRegex = KODI::REGEXP::GetRegExp(
      advancedSettings->m_tvshowMultiPartEnumRegExp, cache, true, CRegExp::autoUtf8);
  if (multiPartRegex == nullptr)
    CLog::LogF(LOGWARNING, "Invalid multipart RegExp '{}', multipart episode detection disabled",
               advancedSettings->m_tvshowMultiPartEnumRegExp);
  const bool disableEpisodeRanges{advancedSettings->m_disableEpisodeRanges};

  for (const auto& tvShowRegExp : tvShowRegExps)
  {
    std::shared_ptr<CRegExp> reg =
        KODI::REGEXP::GetRegExp(tvShowRegExp.regexp, cache, true, CRegExp::autoUtf8);
    if (reg == nullptr)
      continue; // Failed to compile

    int regPosition;
    if ((regPosition = reg->RegFind(label.c_str())) < 0)
      continue;

    VIDEO::EPISODE episode;
    episode.strPath = item->GetPath();
    episode.iSeason = -1;
    episode.iEpisode = -1;
    episode.cDate.SetValid(false);
    episode.isFolder = false;

    const bool byDate{tvShowRegExp.byDate};
    const bool byTitle{tvShowRegExp.byTitle};
    const int defaultSeason{tvShowRegExp.defaultSeason};

    if (byDate)
    {
      if (!GetAirDateFromRegExp(*reg, episode))
        continue;

      CLog::LogF(LOGDEBUG, "Found date based match {} ({}) [{}]",
                 CURL::GetRedacted(episode.strPath), episode.cDate.GetAsLocalizedDate(),
                 tvShowRegExp.regexp);
    }
    else if (byTitle)
    {
      if (!GetEpisodeTitleFromRegExp(*reg, episode))
        continue;

      CLog::LogF(LOGDEBUG, "Found title based match {} ({}) [{}]",
                 CURL::GetRedacted(episode.strPath), episode.strTitle, tvShowRegExp.regexp);
    }
    else
    {
      if (!GetEpisodeAndSeasonFromRegExp(*reg, episode, defaultSeason))
        continue;

      CLog::LogF(LOGDEBUG, "Found episode match {} (s{}e{}) [{}]",
                 CURL::GetRedacted(episode.strPath), episode.iSeason, episode.iEpisode,
                 tvShowRegExp.regexp);
    }

    // Grab the remainder from first regexp run
    // as second run might modify or empty it.
    std::string remainder(reg->GetMatch(3));

    // Check if the files base path is a dedicated folder that contains
    // only this single episode. If season and episode match with the
    // actual media file, we set episode.isFolder to true.
    std::string basePath{item->GetBaseMoviePath(true)};
    URIUtils::RemoveSlashAtEnd(basePath);
    basePath = URIUtils::GetFileName(basePath);

    if (reg->RegFind(basePath.c_str()) > -1)
    {
      VIDEO::EPISODE parent;
      if (byDate)
      {
        GetAirDateFromRegExp(*reg, parent);
        if (episode.cDate == parent.cDate)
          episode.isFolder = true;
      }
      else
      {
        GetEpisodeAndSeasonFromRegExp(*reg, parent, defaultSeason);
        if (episode.iSeason == parent.iSeason && episode.iEpisode == parent.iEpisode)
          episode.isFolder = true;
      }
    }

    // add what we found by now
    episodeList.push_back(episode);

    // check the remainder of the string for any further episodes.
    // Multi-part only applies to season/episode matches, not date or title based
    if (!byDate && !byTitle && multiPartRegex != nullptr)
    {
      size_t offset{0};
      int reg2Position;
      int currentSeason{episode.iSeason};
      int currentEpisode{episode.iEpisode};

      // we want "long circuit" OR below so that both offsets are evaluated
      while (static_cast<int>((reg2Position = multiPartRegex->RegFind(remainder.c_str() + offset)) >
                              -1) |
             static_cast<int>((regPosition = reg->RegFind(remainder.c_str() + offset)) > -1))
      {
        if ((regPosition <= reg2Position && regPosition != -1) || // season (or 'ep') match
            (regPosition >= 0 && reg2Position == -1))
        {
          GetEpisodeAndSeasonFromRegExp(*reg, episode, defaultSeason);
          if (currentSeason == episode.iSeason)
          {
            // Already added SxxEyy now loop (if needed) to SxxEzz
            const int last{episode.iEpisode};
            const int next{disableEpisodeRanges ||
                                   !std::string_view(remainder).substr(offset).starts_with('-')
                               ? last
                               : currentEpisode + 1};

            ProcessEpisodeRange(next, last, episode, episodeList,
                                advancedSettings->m_tvshowMultiPartEnumRegExp, remainder);

            currentEpisode = episode.iEpisode;
            remainder = reg->GetMatch(3);
          }
          else
          {
            // Two possible scenarios here:
            if (disableEpisodeRanges ||
                !std::string_view(remainder).substr(offset).starts_with('-'))
            {
              // (Sxx)Eyy has already been added and we now in a new range (eg. S00E01S01E01....)
              // Add first episode here
              currentSeason = episode.iSeason;
              currentEpisode = episode.iEpisode;
              episodeList.push_back(episode);
              remainder = reg->GetMatch(3);
            }
            else
            {
              // (Sxx)Eyy has already been added as start of range and we now have SaaEbb (eg. S01E01-S02E05)
              //   this is not allowed as scanner cannot determine how many episodes in a season
              if (offset == 0)
              {
                // Already added first episode of invalid range so remove it
                episodeList.pop_back();
                remainder = reg->GetMatch(3);
                CLog::LogF(
                    LOGDEBUG,
                    "VideoInfoScanner: Removing season {}, episode {} as part of invalid range",
                    episode.iSeason, episode.iEpisode);
              }
              else
              {
                remainder = remainder.substr(offset);
              }
            }
          }
          offset = 0;
        }
        else if ((reg2Position < regPosition && reg2Position != -1) || // episode match
                 (reg2Position >= 0 && regPosition == -1))
        {
          const std::string result{multiPartRegex->GetMatch(2)};
          const int last{std::stoi(result)};
          const std::string prefix{
              offset < remainder.length()
                  ? StringUtils::ToLower(std::string_view(remainder).substr(offset, 2))
                  : std::string{}};
          const int next{(prefix == "-e" || prefix == "-s") && !disableEpisodeRanges
                             ? currentEpisode + 1
                             : last};

          ProcessEpisodeRange(next, last, episode, episodeList,
                              advancedSettings->m_tvshowMultiPartEnumRegExp,
                              multiPartRegex->GetMatch(3));

          currentEpisode = episode.iEpisode;
          offset += reg2Position + multiPartRegex->GetMatch(1).length() + result.length();
        }
      }
    }
    return true;
  }
  return false;
}

namespace
{
std::vector<std::tuple<int, int, int>> ParseEpisodes(const std::string& input)
{
  std::map<int, std::set<int>> allEpisodes;

  CRegExp pattern;
  if (!pattern.RegComp(R"(S(\d{1,3})E(\d{1,3}))"))
  {
    CLog::LogF(LOGERROR, "Failed to compile episode regex pattern");
    return {};
  }

  // Parse string
  int pos = 0;
  while ((pos = pattern.RegFind(input, pos)) >= 0)
  {
    int season = static_cast<int>(std::strtol(pattern.GetMatch(1).c_str(), nullptr, 10));
    int episode = static_cast<int>(std::strtol(pattern.GetMatch(2).c_str(), nullptr, 10));
    allEpisodes[season].insert(episode);
    pos += pattern.GetFindLen();
  }

  // Now find ranges
  std::vector<std::tuple<int, int, int>> result;
  for (const auto& [season, episodes] : allEpisodes)
  {
    if (episodes.empty())
      continue;

    int rangeStart{*episodes.begin()};
    int rangeEnd{rangeStart};
    for (auto it = std::next(episodes.begin()); it != episodes.end(); ++it)
    {
      if (*it == rangeEnd + 1)
        rangeEnd = *it;
      else
      {
        result.emplace_back(season, rangeStart, rangeEnd);
        rangeStart = rangeEnd = *it;
      }
    }
    result.emplace_back(season, rangeStart, rangeEnd);
  }

  return result;
}
} // namespace

std::string CEpisodeUtils::GetEpisodesLabel(const CFileItem& item)
{
  const std::string episodeString{item.GetProperty("episodes").asString("")};
  const int numSpecials{item.GetProperty("episodes_specials").asInteger32(0)};
  const auto episodes{ParseEpisodes(episodeString)};
  const bool hasSpecials{numSpecials > 0};
  bool singleSeason{!episodes.empty() &&
                    std::get<0>(episodes.front()) == std::get<0>(episodes.back())};

  constexpr int BASE{21486};
  constexpr int RANGE{1};
  constexpr int SEASON{2};
  constexpr int SPECIALS{21490};

  std::vector<std::string> labels;
  if (!episodes.empty())
  {
    for (const auto& [season, startEpisode, endEpisode] : episodes)
    {
      if (singleSeason)
      {
        if (endEpisode == startEpisode)
          labels.push_back(StringUtils::Format(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(BASE),
              startEpisode));
        else
          labels.push_back(StringUtils::Format(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(BASE + RANGE),
              startEpisode, endEpisode));
      }
      else
      {
        if (endEpisode == startEpisode)
          labels.push_back(StringUtils::Format(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(BASE + SEASON),
              season, startEpisode));
        else
          labels.push_back(
              StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                                      BASE + SEASON + RANGE),
                                  season, startEpisode, endEpisode));
      }
    }
  }
  if (hasSpecials)
    labels.push_back(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(SPECIALS));

  // Generate label
  using namespace KODI::UTILS::I18N;
  const auto fmt =
      CListFormatter::CreateInstance(CServiceBroker::GetResourcesComponent().GetLocalizeStrings());
  const std::string label{fmt.Format(labels)};

  return label;
}
