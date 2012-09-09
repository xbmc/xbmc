#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include <stdint.h>

class CStopWatch
{
public:
  CStopWatch(bool useFrameTime=false);
  ~CStopWatch();

  bool IsRunning() const;
  void StartZero();          ///< Resets clock to zero and starts running
  void Start();              ///< Sets clock to zero if not running and starts running.
  void Stop();               ///< Stops clock and sets to zero if running.
  void Reset();              ///< Resets clock to zero - does not alter running state.

  float GetElapsedSeconds() const;
  float GetElapsedMilliseconds() const;
private:
  int64_t GetTicks() const;
  float m_timerPeriod;        // to save division in GetElapsed...()
  int64_t m_startTick;
  bool m_isRunning;
  bool m_useFrameTime;
};
