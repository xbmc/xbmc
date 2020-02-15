/*
 *  Copyright (c) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once


class CDVDCodecUtils
{
public:
  static bool IsVP3CompatibleWidth(int width);
  static double NormalizeFrameduration(double frameduration, bool *match = nullptr);
};

