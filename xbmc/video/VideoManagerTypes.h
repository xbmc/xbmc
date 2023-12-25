/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum class VideoAssetTypeOwner
{
  UNKNOWN = -1,
  SYSTEM = 0,
  AUTO = 1,
  USER = 2
};

enum class VideoAssetType
{
  UNKNOWN = -1,
  VERSION = 0,
  EXTRAS = 1
};

static constexpr int VIDEO_VERSION_ID_BEGIN = 40400;
static constexpr int VIDEO_VERSION_ID_END = 40800;
static constexpr int VIDEO_VERSION_ID_DEFAULT = VIDEO_VERSION_ID_BEGIN;
static constexpr int VIDEO_VERSION_ID_ALL = 0;
