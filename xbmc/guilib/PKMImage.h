/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class CPKMImage
{
public:
  CPKMImage() = default;
  ~CPKMImage() = default;

  unsigned int GetWidth() const { return m_width; }
  unsigned int GetHeight() const { return m_height; }
  unsigned int GetFormat() const { return m_format; }
  unsigned int GetSize() const { return m_size; }
  const char *GetData() const { return m_data.data(); }

  bool ReadFile(const std::string &file);

private:
  unsigned int m_width = 0;
  unsigned int m_height = 0;
  unsigned int m_size = 0;
  unsigned int m_format = 0;
  std::vector<char> m_data;
};
