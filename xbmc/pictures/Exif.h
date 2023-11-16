/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

constexpr unsigned int MAX_DATE_COPIES = 10;
constexpr int EXIF_COMMENT_CHARSET_CONVERTED = -1;

struct ExifInfo
{
  ExifInfo() = default;
  ExifInfo(const ExifInfo&) = default;
  ExifInfo(ExifInfo&&) = default;

  ExifInfo& operator=(const ExifInfo&) = default;
  ExifInfo& operator=(ExifInfo&&) = default;

  std::string CameraMake;
  std::string CameraModel;
  std::string DateTime;
  int Height{};
  int Width{};
  int Orientation{};
  int IsColor{};
  std::string Process{};
  int FlashUsed{};
  float FocalLength{};
  float ExposureTime{};
  float ApertureFNumber{};
  float Distance{};
  float CCDWidth{};
  float ExposureBias{};
  float DigitalZoomRatio{};
  int FocalLength35mmEquiv{};
  int Whitebalance{};
  int MeteringMode{};
  int ExposureProgram{};
  int ExposureMode{};
  int ISOequivalent{};
  int LightSource{};
  //! TODO: Remove me, not needed anymore (still exposed to addon interface)
  int CommentsCharset{};
  //! TODO: Remove me, not needed anymore (still exposed to addon interface)
  int XPCommentsCharset{};
  std::string Comments;
  std::string FileComment;
  std::string XPComment;
  std::string Description;

  //! TODO: this is not used anywhere, just nuke it
  unsigned ThumbnailOffset{};
  unsigned ThumbnailSize{};
  unsigned LargestExifOffset{};
  //! TODO: this is not used anywhere, just nuke it
  char ThumbnailAtEnd{};
  int ThumbnailSizeOffset{};
  //! TODO: this is not used anywhere, just nuke it
  std::vector<int> DateTimeOffsets;

  int GpsInfoPresent{};
  std::string GpsLat;
  std::string GpsLong;
  std::string GpsAlt;
};
