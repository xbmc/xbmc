/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RETRO
{
class ISavestate;

/*!
 * \brief Self-contained data needed to create a savestate thumbnail
 *
 * This payload owns a copy of the savestate video frame so thumbnail encoding can be performed
 * after the savestate object has been moved, finalized, or destroyed.
 */
struct SavestateThumbnailPayload
{
  //! Destination path for the generated thumbnail image
  std::string thumbnailPath;

  //! Raw video frame pixels copied from the savestate
  std::vector<uint8_t> pixels;

  //! Width of the source video frame, in pixels
  unsigned int width{0};

  //! Height of the source video frame, in pixels
  unsigned int height{0};

  //! Source video frame rotation, in degrees counter-clockwise
  unsigned int rotationCCW{0};

  //! FFmpeg pixel format for the source video frame
  AVPixelFormat pixelFormat{AV_PIX_FMT_NONE};
};

/*!
 * \brief Create a thumbnail payload from a savestate video frame
 *
 * \param thumbnailPath Destination path for the thumbnail image
 * \param savestate Savestate containing prepared video frame data
 *
 * \return A populated payload, or std::nullopt if the thumbnail path or required video metadata is
 * invalid
 */
std::optional<SavestateThumbnailPayload> CreateSavestateThumbnailPayload(
    const std::string& thumbnailPath, const ISavestate& savestate);

/*!
 * \brief Scale and write a savestate thumbnail payload to disk
 *
 * \param payload Thumbnail payload created by CreateSavestateThumbnailPayload()
 *
 * \return True when the thumbnail was written successfully, false otherwise
 */
bool WriteSavestateThumbnailPayload(const SavestateThumbnailPayload& payload);
} // namespace RETRO
} // namespace KODI
