/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaType.h"

#include <cstdint>

enum class VideoAssetTypeOwner
{
  UNKNOWN = -1,
  SYSTEM = 0,
  AUTO = 1,
  USER = 2
};

enum class VideoAssetType : int
{
  VERSIONSANDEXTRASFOLDER =
      -2, //!< reserved for nodes navigation, returns versions + extras virtual folder. do not use in the db.
  UNKNOWN = -1,
  ALL =
      0, //!< reserved for nodes navigation, returns all assets of all types. do not use in the db.
  VERSION = 1,
  EXTRA = 2,
};

enum class MediaRole
{
  NewVersion,
  Parent
};

enum class VersionConversionResult : uint8_t
{
  SUCCESS,
  CANCELLED,
  FAILED,
  NOT_NEEDED,
  NOT_ALLOWED
};

static constexpr int VIDEO_VERSION_ID_BEGIN = 40400;
static constexpr int VIDEO_VERSION_ID_END = 40800;
static constexpr int VIDEO_VERSION_ID_DEFAULT = VIDEO_VERSION_ID_BEGIN;
static constexpr int VIDEO_VERSION_ID_ALL = 0;

//! The kinds of video extra (eg. "Deleted scenes"), within the range above
static constexpr int VIDEO_EXTRA_ID_BEGIN = 40500;
static constexpr int VIDEO_EXTRA_ID_END = 40699;
//! What an extra of a kind is called (eg. "Deleted scene: {0:s}"), which are not types
static constexpr int VIDEO_EXTRA_NAME_ID_BEGIN = 40700;
static constexpr int VIDEO_EXTRA_NAME_ID_END = 40799;

struct VideoAssetInfo
{
  int m_idFile{-1};
  int m_assetTypeId{-1};
  std::string m_assetTypeName;
  int m_idMedia{-1};
  MediaType m_mediaType{MediaTypeNone};
  VideoAssetType m_assetType{VideoAssetType::UNKNOWN};
};
