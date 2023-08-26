/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "imagefiles/SpecialImageFileLoader.h"

namespace PVR
{
/*!
 * @brief Generates a thumbnail for a PVR channel group; tile up to 9 channel icons in the group.
*/
class CPVRChannelGroupImageFileLoader : public IMAGE_FILES::ISpecialImageFileLoader
{
public:
  CPVRChannelGroupImageFileLoader() = default;
  ~CPVRChannelGroupImageFileLoader() override = default;

  bool CanLoad(const std::string& specialType) const override;
  std::unique_ptr<CTexture> Load(const std::string& specialType,
                                 const std::string& goofyChapterPath,
                                 unsigned int preferredWidth,
                                 unsigned int preferredHeight) const override;
};

} // namespace PVR
