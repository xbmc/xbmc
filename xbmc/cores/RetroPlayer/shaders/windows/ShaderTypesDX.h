/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <minwindef.h>

namespace KODI::SHADER
{
struct CUSTOMVERTEX
{
  FLOAT x;
  FLOAT y;
  FLOAT z;

  FLOAT tu;
  FLOAT tv;
};
} // namespace KODI::SHADER
