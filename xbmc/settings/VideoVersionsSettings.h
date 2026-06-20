/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum class SimilarVideoScanAction
{
  NONE = 0, //!< no action
  ASK = 1, //!< ask the user if a new version of an existing video should be created
  AUTO, //!< automatically create a new version
};
