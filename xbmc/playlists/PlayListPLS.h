/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayList.h"

#include <string>
#include <vector>

namespace KODI::PLAYLIST
{
class CPlayListPLS : public CPlayList
{
public:
  CPlayListPLS(void);
  ~CPlayListPLS(void) override;
  bool Load(const std::string& strFileName) override;
  void Save(const std::string& strFileName) const override;
  virtual bool Resize(std::vector<int>::size_type newSize);
};
}
