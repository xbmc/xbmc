/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

static constexpr unsigned int MIN_EPISODE_DURATION = 10 * 60; // 10 minutes
static constexpr unsigned int MAX_EPISODE_DIFFERENCE = 30; // 30 seconds
static constexpr unsigned int MIN_SPECIAL_DURATION = 5 * 60; // 5 minutes
static constexpr unsigned int MAIN_TITLE_LENGTH_PERCENT = 70;

struct DiscStreamInfo
{
  unsigned int coding{0};
  unsigned int format{0};
  unsigned int rate{0};
  unsigned int aspect{0};
  std::string lang;
};

struct PlaylistInfo
{
  unsigned int playlist;
  unsigned int duration{0};
  std::vector<unsigned int> clips;
  std::map<unsigned int, unsigned int> clipDuration;
  std::vector<unsigned int> chapters;
  std::vector<DiscStreamInfo> videoStreams;
  std::vector<DiscStreamInfo> audioStreams;
  std::vector<DiscStreamInfo> pgStreams;
  std::string languages;
};

struct ClipInfo
{
  unsigned int duration{0};
  std::vector<unsigned int> playlists;
};

using PlaylistMap = std::map<unsigned int, PlaylistInfo>;
using PlaylistMapEntry = std::pair<unsigned int, PlaylistInfo>;
using PlaylistVector = std::vector<std::pair<unsigned int, PlaylistInfo>>;
using PlaylistVectorEntry = std::pair<unsigned int, PlaylistInfo>;
using ClipMap = std::map<unsigned int, ClipInfo>;

enum class GetTitlesJob
{
  GET_TITLES_ONE = 0,
  GET_TITLES_MAIN,
  GET_TITLES_ALL
};
enum class SortTitlesJob
{
  SORT_TITLES_NONE = 0,
  SORT_TITLES_EPISODE,
  SORT_TITLES_MOVIE,
};

class CFileItem;
class CFileItemList;
class CURL;
class CVideoInfoTag;

namespace XFILE
{
class CDiscDirectoryHelper
{
public:
  static bool GetEpisodePlaylists(const CURL& url,
                                  CFileItemList& items,
                                  int episodeIndex,
                                  const std::vector<CVideoInfoTag>& episodesOnDisc,
                                  const ClipMap& clips,
                                  const PlaylistMap& playlists);
  static void GenerateItem(const CURL& url,
                           const std::shared_ptr<CFileItem>& item,
                           unsigned int playlist,
                           const PlaylistMap& playlists,
                           const CVideoInfoTag& tag,
                           bool isSpecial = false);
  static void AddRootOptions(const CURL& url, CFileItemList& items, bool addMenuOption);
  static std::vector<CVideoInfoTag> GetEpisodesOnDisc(const CURL& url);
  static std::string GetEpisodesLabel(CFileItem& newItem, const CFileItem& item);

  static std::string HexToString(std::span<const uint8_t> buf, int count);
  static std::string HexToString(int num, int count);
  static unsigned int GetDWord(const std::vector<char>& bytes, unsigned int offset);
  static unsigned int GetWord(const std::vector<char>& bytes, unsigned int offset);
  static unsigned int GetByte(const std::vector<char>& bytes, unsigned int offset);
  static std::string GetString(const std::vector<char>& bytes,
                               unsigned int offset,
                               unsigned int length);
};
} // namespace XFILE