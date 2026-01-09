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
  CCaptionBlock(const CCaptionBlock&) = delete;
  CCaptionBlock& operator=(const CCaptionBlock&) = delete;

public:
  explicit CCaptionBlock(int size) : m_pts(0.0), m_data(size) {}
  virtual ~CCaptionBlock() = default;
  double m_pts;
  std::vector<uint8_t> m_data;
};
