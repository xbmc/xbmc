/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CProcessInfo;
struct VideoPicture;

class IDVDVideoPP
{
public:
  virtual ~IDVDVideoPP() = default;

  virtual void SetType(const std::string& mType, bool deinterlace) = 0;
  virtual void Process(VideoPicture* picture) = 0;
};
