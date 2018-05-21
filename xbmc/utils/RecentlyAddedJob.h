#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
