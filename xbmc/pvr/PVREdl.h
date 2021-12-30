/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

class CFileItem;

namespace EDL
{
struct Edit;
}

namespace PVR
{

class CPVREdl
{
public:
  /*!
   * @brief Get the EDL edits for the given item.
   * @param item The item.
   * @return The EDL edits or an empty vector if no edits exist.
   */
  static std::vector<EDL::Edit> GetEdits(const CFileItem& item);
};

} // namespace PVR
