/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "imagefiles/SpecialImageFileLoader.h"

#include <array>
#include <memory>
#include <string>

class CTexture;

namespace IMAGE_FILES
{
class CSpecialImageLoaderFactory
{
public:
  CSpecialImageLoaderFactory();

  std::unique_ptr<CTexture> Load(const std::string& specialType,
                                 const std::string& filePath,
                                 unsigned int preferredWidth,
                                 unsigned int preferredHeight) const;

private:
  std::array<std::unique_ptr<ISpecialImageFileLoader>, 6> m_specialImageLoaders{};
};
} // namespace IMAGE_FILES
