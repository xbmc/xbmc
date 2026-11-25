/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "video/VideoDbContentType.h"
#include "video/VideoManagerTypes.h"

#include <optional>
#include <string>

namespace KODI::VIDEO
{
//! A playlist of a disc, and the library item it belongs to
struct DiscPlaylist
{
  int playlist{-1};
  int idFile{-1};
  VideoDbContentType mediaType{-1};
  int idMedia{-1};
  std::string title{};

  //! Which of a movie's assets holds the playlist. Unset for an episode, which is named by its
  //! title instead.
  std::optional<VideoAssetType> itemType{};
  CDateTime dateAdded{};
};
} // namespace KODI::VIDEO
