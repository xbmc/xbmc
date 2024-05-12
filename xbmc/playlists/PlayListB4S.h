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

class CPlayListB4S : public CPlayList
{
public:
  CPlayListB4S(void);
  ~CPlayListB4S(void) override;
  bool LoadData(std::istream& stream) override;
  void Save(const std::string& strFileName) const override;
};
} // namespace KODI::PLAYLIST
