/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMHelpers.h"

#include <sstream>

namespace DRMHELPERS
{

std::string FourCCToString(uint32_t fourcc)
{
  std::stringstream ss;
  ss << static_cast<char>((fourcc & 0x000000FF));
  ss << static_cast<char>((fourcc & 0x0000FF00) >> 8);
  ss << static_cast<char>((fourcc & 0x00FF0000) >> 16);
  ss << static_cast<char>((fourcc & 0xFF000000) >> 24);

  return ss.str();
}

} // namespace DRMHELPERS
