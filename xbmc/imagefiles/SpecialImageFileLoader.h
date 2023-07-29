/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CTexture;

namespace IMAGE_FILES
{

/*!
 * @brief An interface to load special image files into a texture for display.
 *
 * Special image files are images that are more than just a link or path to an
 * image file, such as generated or embedded images - like album covers
 * embedded in music files or thumbnails generated from video files.
*/
class ISpecialImageFileLoader
{
public:
  virtual bool CanLoad(const std::string& specialType) const = 0;
  virtual std::unique_ptr<CTexture> Load(const std::string& specialType,
                                         const std::string& filePath,
                                         unsigned int preferredWidth,
                                         unsigned int preferredHeight) const = 0;
  virtual ~ISpecialImageFileLoader() = default;
};

} // namespace IMAGE_FILES
