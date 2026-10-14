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
#include <vector>

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
  std::unique_ptr<CTexture> Load(const IMAGE_FILES::CImageFileURL& imageFile) const override;

  /*!
   * @brief Get the channel icons the thumbnail for the given channel group is composed of.
   * @param groupPath The path of the channel group.
   * @return The icon paths.
   */
  static std::vector<std::string> GetChannelGroupIcons(const std::string& groupPath);
};

} // namespace PVR
