/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayList.h"

namespace KODI::PLAYLIST
{
class CPlayListURL : public CPlayList
{
public:
  CPlayListURL(void);
  ~CPlayListURL(void) override;
  bool Load(const std::string& strFileName) override;
};
}
