/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ChapterEdlParser.h"

#include "FileItem.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <tuple>

using namespace EDL;
using namespace std::chrono_literals;

namespace
{
std::tuple<bool, std::chrono::milliseconds, std::chrono::milliseconds> ParseChapters(
    const CFileItem& item)
{
  if (!item.HasVideoInfoTag())
    return std::make_tuple(false, 0ms, 0ms);

  const CVideoInfoTag* tag{item.GetVideoInfoTag()};
  const auto& chapters{tag->GetChapters()};
  if (chapters.empty())
    return std::make_tuple(false, 0ms, 0ms);

  const CURL url{item.GetDynPath()};
  const std::string chaptersOption{url.GetOption("chapters")};
  if (chaptersOption.empty())
    return std::make_tuple(false, 0ms, 0ms);

  CRegExp c{true, CRegExp::autoUtf8, R"((\d{1,3})(?:-)(\d{1,3})$)"};
  if (c.RegFind(chaptersOption) == -1)
    return std::make_tuple(false, 0ms, 0ms);

  int startChapter{std::stoi(c.GetMatch(1))};
  int endChapter{std::stoi(c.GetMatch(2))};
  if (startChapter < 1 || endChapter > static_cast<int>(chapters.size()) ||
      startChapter > endChapter)
    return std::make_tuple(false, 0ms, 0ms);

  // Chapter numbers are 1 based
  return std::make_tuple(true, chapters[startChapter - 1].start,
                         chapters[endChapter - 1].start + chapters[endChapter - 1].duration);
}
} // namespace

bool CChapterEdlParser::CanParse(const CFileItem& item) const
{
  [[maybe_unused]] const auto& [success, start, end] = ParseChapters(item);
  return success;
}

CEdlParserResult CChapterEdlParser::Parse(const CFileItem& item,
                                          float fps,
                                          std::chrono::milliseconds duration)
{
  CEdlParserResult result;

  const auto& [success, start, end] = ParseChapters(item);
  if (!success)
    return result;

  // Check EDL actually needed
  if (start == 0ms && end == duration)
    return result;

  // Check times are valid
  if (start < 0ms || start > duration || end < 0ms || end > duration || start >= end)
  {
    CLog::LogF(LOGERROR, "Invalid chapter times for item: {}. Start: {}, End: {}.",
               CURL::GetRedacted(item.GetDynPath()), StringUtils::MillisecondsToTimeString(start),
               StringUtils::MillisecondsToTimeString(end));
    return result;
  }

  Edit edit;
  edit.action = Action::CUT;
  if (start > 0ms)
  {
    edit.start = 0ms;
    edit.end = start;
    result.AddEdit(edit);
    CLog::LogF(LOGDEBUG, "Adding start EDL cut [{} - {}] for chapters in item: {}.",
               StringUtils::MillisecondsToTimeString(0ms),
               StringUtils::MillisecondsToTimeString(start), CURL::GetRedacted(item.GetDynPath()));
  }
  if (end > 0ms && end < duration)
  {
    edit.start = end;
    edit.end = duration;
    result.AddEdit(edit);
    CLog::LogF(LOGDEBUG, "Adding end EDL cut [{} - {}] for chapters in item: {}.",
               StringUtils::MillisecondsToTimeString(end),
               StringUtils::MillisecondsToTimeString(duration),
               CURL::GetRedacted(item.GetDynPath()));
  }

  return result;
}
