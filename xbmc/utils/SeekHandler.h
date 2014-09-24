#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
  int        GetSeekSeconds(bool forward);
  int        m_time_before_seek;
  int        m_time_for_display;
  bool       m_requireSeek;
  float      m_percent; 
  float      m_percents_pro_sec;
  int        m_seek_step;
  CStopWatch m_timer;
};
