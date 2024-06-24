/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ImageMetadata.h"

#include <memory>
#include <string>

#include <exiv2/exiv2.hpp>

class CImageMetadataParser
{
public:
  ~CImageMetadataParser() = default;

  static std::unique_ptr<ImageMetadata> ExtractMetadata(const std::string& picFileName);

private:
  CImageMetadataParser();
  void ExtractCommonMetadata(Exiv2::Image& image);
  void ExtractExif(Exiv2::ExifData& exifData);
  void ExtractIPTC(Exiv2::IptcData& iptcData);

  int m_imageWidth{0};
  float m_focalPlaneXRes{0.0};
  float m_focalPlaneUnits{0};
  std::unique_ptr<ImageMetadata> m_imageMetadata;
};
