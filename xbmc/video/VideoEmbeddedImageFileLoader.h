/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "imagefiles/SpecialImageFileLoader.h"

#include <string>

class EmbeddedArt;

namespace KODI::VIDEO
{
/*!
 * @brief Generates a texture for an image embedded in a video file.
*/
class CVideoEmbeddedImageFileLoader : public IMAGE_FILES::ISpecialImageFileLoader
{
public:
  CVideoEmbeddedImageFileLoader() = default;
  ~CVideoEmbeddedImageFileLoader() override = default;

  bool CanLoad(const std::string& specialType) const override;
  std::unique_ptr<CTexture> Load(const IMAGE_FILES::CImageFileURL& imageFile) const override;

  /*!
   * @brief Finds the image of a type that a video file holds, whether or not an nfo is beside it.
   * @param path The video file
   * @param type The image's type, as "poster"
   * @param[out] art The image found
   * @return true if the file holds one
   */
  static bool GetEmbeddedArt(const std::string& path, const std::string& type, EmbeddedArt& art);
};

} // namespace KODI::VIDEO
