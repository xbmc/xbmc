/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/XBMCTinyXML.h"

#include <string_view>

class CNfoUtils
{
public:
  CNfoUtils() = delete;

  static bool Upgrade(TiXmlElement* root);
  static void SetVersion(TiXmlElement& elem, std::string_view tag);
};
