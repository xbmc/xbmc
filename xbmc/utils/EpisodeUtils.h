/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/Episode.h"

class CFileItem;

class CEpisodeUtils
{
public:
  static bool EnumerateEpisodeItem(const CFileItem* item, KODI::VIDEO::EPISODELIST& episodeList);
};
