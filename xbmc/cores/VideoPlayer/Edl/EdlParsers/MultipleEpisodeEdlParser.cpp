/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultipleEpisodeEdlParser.h"

#include "FileItem.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <chrono>
#include <cstdint>
#include <ranges>

using namespace EDL;
using namespace std::chrono_literals;

bool CMultipleEpisodeEdlParser::CanParse(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;

  const CVideoInfoTag* tag{item.GetVideoInfoTag()};
  if (tag->m_type != MediaTypeEpisode)
    return false;
  if (tag->m_iIdShow <= 0 || tag->m_iFileId <= 0)
    return false;

  return true;
}

CEdlParserResult CMultipleEpisodeEdlParser::Parse(const CFileItem& item,
                                                  float fps,
                                                  int64_t duration)
{
  CEdlParserResult result;
  const CVideoInfoTag* tag{item.GetVideoInfoTag()};

  EpisodeFileMap fileMap;
  CVideoDatabase db;
  if (!db.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return result;
  }
  const bool success{db.GetEpisodeMap(tag->m_iIdShow, fileMap, tag->m_iFileId)};
  db.Close();
  if (!success)
    return result;

  result = Process(item, fps, duration, fileMap);
  return result;
}

CEdlParserResult CMultipleEpisodeEdlParser::Process(const CFileItem& item,
                                                    float fps,
                                                    int64_t duration,
                                                    const EpisodeFileMap& fileMap)
{
  CEdlParserResult result;

  // Find this episode in the map
  const CVideoInfoTag* tag{item.GetVideoInfoTag()};
  const auto it{std::ranges::find_if(
      fileMap, [tag](const auto& i)
      { return i.second.season == tag->m_iSeason && i.second.episode == tag->m_iEpisode; })};
  if (it == fileMap.end())
    return result;

  const auto episodeInfo{it->second};
  if (fileMap.size() == 1 && episodeInfo.bookmark.timeInSeconds == 0.0 &&
      episodeInfo.bookmark.totalTimeInSeconds == 0.0)
    return result; // No bookmark for single episode

  // If no episode bookmark then timeInSeconds will be 0ms
  const auto start{
      std::chrono::milliseconds(static_cast<int64_t>(episodeInfo.bookmark.timeInSeconds * 1000))};
  const auto length{std::chrono::milliseconds(duration)};
  auto end{length};

  // Now see if the next episode is also in the file (to determine the end of this episode)
  // Note the FileMap only includes episodes in this file
  // Otherwise default to the end of the file
  const auto it2{std::ranges::find_if(
      fileMap, [tag](const auto& i)
      { return i.second.season == tag->m_iSeason && i.second.episode == tag->m_iEpisode + 1; })};
  if (it2 != fileMap.end())
  {
    if (const auto time{it2->second.bookmark.timeInSeconds}; time > 0)
      end = std::chrono::milliseconds(static_cast<int64_t>(time * 1000));
  }

  // Check EDL actually needed
  if (start == 0ms && end == length)
    return result;

  Edit edit;
  edit.action = Action::CUT;
  if (start > 0ms)
  {
    edit.start = 0ms;
    edit.end = start;
    result.AddEdit(edit);
    CLog::LogF(LOGDEBUG, "Adding EDL cut [{} - {}] for multi-episode item: {}.",
               StringUtils::MillisecondsToTimeString(0ms),
               StringUtils::MillisecondsToTimeString(start), CURL::GetRedacted(item.GetDynPath()));
  }
  if (end > 0ms && end < length)
  {
    edit.start = end;
    edit.end = length;
    result.AddEdit(edit);
    CLog::LogF(LOGDEBUG, "Adding EDL cut [{} - {}] for multi-episode item: {}.",
               StringUtils::MillisecondsToTimeString(end),
               StringUtils::MillisecondsToTimeString(length), CURL::GetRedacted(item.GetDynPath()));
  }

  return result;
}
