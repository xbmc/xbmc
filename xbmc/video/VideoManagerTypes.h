/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

enum class VideoAssetTypeOwner
{
  UNKNOWN = -1,
  SYSTEM = 0,
  AUTO = 1,
  USER = 2
};

enum class VideoAssetType : int
{
  UNKNOWN = -1,
  VERSION = 0,
  EXTRA = 1
};

enum class MediaRole
{
  NewVersion,
  Parent
};

static constexpr int VIDEO_VERSION_ID_BEGIN = 40400;
static constexpr int VIDEO_VERSION_ID_END = 40800;
static constexpr int VIDEO_VERSION_ID_DEFAULT = VIDEO_VERSION_ID_BEGIN;
static constexpr int VIDEO_VERSION_ID_ALL = 0;
static const std::string VIDEODB_PATH_VERSION_ID_ALL{"videodb://movies/videoversions/0"};

struct VideoAssetInfo
{
  int m_idFile{-1};
  int m_assetTypeId{-1};
  std::string m_assetTypeName;
  int m_idMedia{-1};
  MediaType m_mediaType{MediaTypeNone};
  VideoAssetType m_assetType{VideoAssetType::UNKNOWN};
};
