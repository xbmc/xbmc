#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/Stopwatch.h"

class CSeekHandler
{
public:
  CSeekHandler();

  void Seek(bool forward, float amount, float duration = 0);
  void Process();
  void Reset();

  float GetPercent() const;
  bool InProgress() const;
private:
  static const int time_before_seek = 500;
  static const int time_for_display = 2000; // TODO: WTF?
  bool       m_requireSeek;
  float      m_percent;
  CStopWatch m_timer;
};
