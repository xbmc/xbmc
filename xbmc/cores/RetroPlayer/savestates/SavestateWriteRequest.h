/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SavestateThumbnail.h"

#include <memory>
#include <optional>
#include <string>

namespace KODI
{
namespace RETRO
{
class ISavestate;

/*!
 * \brief Move-only request for writing a captured savestate off-thread
 *
 * The request owns the finalized savestate data that will be written to disk.
 * Thumbnail data is optional because thumbnail capture is best-effort and can
 * fail independently of the savestate file write.
 */
struct SavestateWriteRequest
{
  //! Destination path for the savestate file
  std::string savePath;

  //! Path to the game file associated with the savestate
  std::string gamePath;

  //! Captured savestate object to finalize and write
  std::unique_ptr<ISavestate> savestate;

  //! Optional pre-copied thumbnail payload for the savestate
  std::optional<SavestateThumbnailPayload> thumbnail;
};
} // namespace RETRO
} // namespace KODI
