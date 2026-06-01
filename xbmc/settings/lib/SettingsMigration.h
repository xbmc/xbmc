/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/XBMCTinyXML.h"

class CSettingsMigration
{
public:
  CSettingsMigration() = delete;

  static bool UpdateXMLSettings(TiXmlElement* root,
                                int currentVersion,
                                int targetVersion,
                                bool& updated);

private:
  static bool Upgrade(TiXmlElement* root, int currentVersion, bool& updated);
};
