/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaType.h"

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

static constexpr int VIDEO_VERSION_ID_BEGIN = 40400;
static constexpr int VIDEO_VERSION_ID_END = 40800;
static constexpr int VIDEO_VERSION_ID_DEFAULT = VIDEO_VERSION_ID_BEGIN;
static constexpr int VIDEO_VERSION_ID_ALL = 0;

struct VideoAssetInfo
{
  int m_idFile{-1};
  int m_assetTypeId{-1};
  std::string m_assetTypeName;
  int m_idMedia{-1};
  MediaType m_mediaType{MediaTypeNone};
  VideoAssetType m_assetType{VideoAssetType::UNKNOWN};
};
