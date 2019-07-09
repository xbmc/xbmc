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

class Crc32
{
public:
  Crc32();
  void Reset();
  void Compute(const char* buffer, size_t count);
  static uint32_t Compute(const std::string& strValue);
  static uint32_t ComputeFromLowerCase(const std::string& strValue);

  operator uint32_t () const
  {
    return m_crc;
  }

private:
  uint32_t m_crc;
};

