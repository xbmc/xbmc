/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoVersionHelper
{
public:
  static std::shared_ptr<CFileItem> ChooseVideoFromAssets(const std::shared_ptr<CFileItem>& item);
};
} // namespace GUILIB

/*!
 * \brief Is the item a video asset, excluding folders
 * \param[in] item the item
 * \return true if it is, false otherwise
 */
bool IsVideoAssetFile(const CFileItem& item);

} // namespace VIDEO
