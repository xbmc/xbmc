/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateThumbnail.h"

#include "ISavestate.h"
#include "pictures/Picture.h"
#include "utils/log.h"

extern "C"
{
#include <libavutil/imgutils.h>
}

using namespace KODI;
using namespace RETRO;

std::optional<SavestateThumbnailPayload> KODI::RETRO::CreateSavestateThumbnailPayload(
    const std::string& thumbnailPath, const ISavestate& savestate)
{
  const uint8_t* const videoData = savestate.GetVideoData();
  const size_t videoSize = savestate.GetVideoSize();
  const unsigned int width = savestate.GetVideoWidth();
  const unsigned int height = savestate.GetVideoHeight();
  const AVPixelFormat pixelFormat = savestate.GetPixelFormat();

  if (thumbnailPath.empty() || videoData == nullptr || videoSize == 0 || width == 0 ||
      height == 0 || pixelFormat == AV_PIX_FMT_NONE)
  {
    return std::nullopt;
  }

  SavestateThumbnailPayload payload;
  payload.thumbnailPath = thumbnailPath;
  payload.pixels.assign(videoData, videoData + videoSize);
  payload.width = width;
  payload.height = height;
  payload.rotationCCW = savestate.GetRotationDegCCW();
  payload.pixelFormat = pixelFormat;

  return payload;
}

bool KODI::RETRO::WriteSavestateThumbnailPayload(const SavestateThumbnailPayload& payload)
{
  if (payload.thumbnailPath.empty() || payload.pixels.empty() || payload.width == 0 ||
      payload.height == 0 || payload.pixelFormat == AV_PIX_FMT_NONE)
  {
    return false;
  }

  const int stride = av_image_get_linesize(payload.pixelFormat, static_cast<int>(payload.width), 0);
  if (stride <= 0)
    return false;

  unsigned int scaleWidth = 400;
  unsigned int scaleHeight = 220;
  CPicture::GetScale(payload.width, payload.height, scaleWidth, scaleHeight);

  const int bytesPerPixel = 4;
  std::vector<uint8_t> scaledImage(scaleWidth * scaleHeight * bytesPerPixel);

  const AVPixelFormat outFormat = AV_PIX_FMT_BGR0;
  const int scaleStride = av_image_get_linesize(outFormat, static_cast<int>(scaleWidth), 0);
  if (scaleStride <= 0)
    return false;

  if (!CPicture::ScaleImage(const_cast<uint8_t*>(payload.pixels.data()), payload.width,
                            payload.height, stride, payload.pixelFormat, scaledImage.data(),
                            scaleWidth, scaleHeight, scaleStride, outFormat))
  {
    CLog::Log(LOGERROR, "Failed to scale image from size {}x{} to size {}x{}", payload.width,
              payload.height, scaleWidth, scaleHeight);
    return false;
  }

  //! @todo Rotate image by rotationCCW
  (void)payload.rotationCCW;

  return CPicture::CreateThumbnailFromSurface(scaledImage.data(), scaleWidth, scaleHeight,
                                              scaleStride, payload.thumbnailPath);
}
