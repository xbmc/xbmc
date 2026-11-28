/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <vector>

class CCaptionBlock
{
public:
  explicit CCaptionBlock(int size) : m_data(size) {}
  double m_pts = 0.0;
  std::vector<uint8_t> m_data;
};
