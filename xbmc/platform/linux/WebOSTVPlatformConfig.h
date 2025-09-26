/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CVariant;

class WebOSTVPlatformConfig
{
public:
  static void Load();
  static int GetWebOSVersion();
  static bool SupportsDTS();
  static bool SupportsHDR();
};
