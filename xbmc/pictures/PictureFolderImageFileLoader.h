/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "imagefiles/SpecialImageFileLoader.h"

/*!
 * @brief Generates a thumbnail for a folder in the picture browser, tile 4 images inside the folder.
*/
class CPictureFolderImageFileLoader : public IMAGE_FILES::ISpecialImageFileLoader
{
public:
  CPictureFolderImageFileLoader() = default;
  ~CPictureFolderImageFileLoader() override = default;

  bool CanLoad(const std::string& specialType) const override;
  std::unique_ptr<CTexture> Load(const std::string& specialType,
                                 const std::string& filePath,
                                 unsigned int preferredWidth,
                                 unsigned int preferredHeight) const override;
};
