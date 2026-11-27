/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

enum class VideoDbContentType
{
  UNKNOWN = -1,
  MOVIES = 1,
  TVSHOWS = 2,
  MUSICVIDEOS = 3,
  EPISODES = 4,
  MOVIE_SETS = 5,
  MUSICALBUMS = 6
};

template<>
struct fmt::formatter<VideoDbContentType> : fmt::formatter<std::string_view>
{
  template<typename FormatContext>
  constexpr auto format(const VideoDbContentType& type, FormatContext& ctx)
  {
    return fmt::formatter<std::string_view>::format(enumToSV(type), ctx);
  }

private:
  static constexpr std::string_view enumToSV(VideoDbContentType type)
  {
    using namespace std::literals::string_view_literals;
    switch (type)
    {
      using enum VideoDbContentType;

      case UNKNOWN:
        return "unknown"sv;
      case MOVIES:
        return "movies"sv;
      case TVSHOWS:
        return "TV shows"sv;
      case MUSICVIDEOS:
        return "music videos"sv;
      case EPISODES:
        return "episodes"sv;
      case MOVIE_SETS:
        return "movie sets"sv;
      case MUSICALBUMS:
        return "music albums"sv;
    };
    throw std::invalid_argument("no videodb content string found");
  }
};
