/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayList.h"

namespace PLAYLIST
{
class CPlayListXSPF : public CPlayList
{
public:
  CPlayListXSPF(void);
  ~CPlayListXSPF(void) override;

  // Implementation of CPlayList
  bool Load(const std::string& strFileName) override;
};
}
