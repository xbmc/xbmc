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
struct Cut;
}

namespace PVR
{

class CPVREdl
{
public:
  /*!
   * @brief Get the cuts for the given item.
   * @param item The item.
   * @return The EDL cuts or enpty vector if no cuts exist.
   */
  static std::vector<EDL::Cut> GetCuts(const CFileItem& item);
};

} // namespace PVR
