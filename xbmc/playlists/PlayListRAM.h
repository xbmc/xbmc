/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayListFile.h"

#include <iostream>

namespace KODI::PLAYLIST
{
class CPlayListRAM : public CPlayListFile
{
public:
  bool LoadData(std::istream& stream) override;
};
} // namespace KODI::PLAYLIST
