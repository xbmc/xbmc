/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <cstring>

class CAEDeviceInfo;

class CAEELDParser {
public:
  static void Parse(const uint8_t *data, size_t length, CAEDeviceInfo& info);
};

