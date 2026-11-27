/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayListFile.h"

namespace KODI::PLAYLIST
{
class CPlayListXSPF : public CPlayListFile
{
public:
  CPlayListXSPF(void);
  ~CPlayListXSPF(void) override;

  // Implementation of CPlayListFile
  bool Load(const std::string& strFileName) override;
};
}
