/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Job.h"

enum ERecentlyAddedFlag
{
  Audio = 0x1,
  Video = 0x2,
  Totals = 0x4
};

class CRecentlyAddedJob : public CJob
{
public:
  explicit CRecentlyAddedJob(int flag);
  static bool UpdateVideo();
  static bool UpdateMusic();
  static bool UpdateTotal();
  bool DoWork() override;
private:
  int m_flag;
};
