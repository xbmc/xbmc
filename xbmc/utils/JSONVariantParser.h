/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include "utils/Variant.h"

class CJSONVariantParser
{
public:
  CJSONVariantParser() = delete;

  static bool Parse(const char* json, CVariant& data);
  static bool Parse(const std::string& json, CVariant& data);
};
