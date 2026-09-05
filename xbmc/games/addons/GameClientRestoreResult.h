/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI::GAME
{
enum class RestoreResult
{
  Restored, // Target machine and media state restored.
  Rejected, // Target rejected; the previous running state is still valid.
  StateUncertain, // Machine or media state may be inconsistent.
};
} // namespace KODI::GAME
