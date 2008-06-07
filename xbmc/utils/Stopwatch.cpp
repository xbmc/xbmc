/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Stopwatch.h"

CStopWatch::CStopWatch()
{
  m_timerPeriod      = 0.0f;
  m_startTick        = 0;
  m_isRunning        = false;

  // Get the timer frequency (ticks per second)
  LARGE_INTEGER timerFreq;
  QueryPerformanceFrequency( &timerFreq );
  m_timerPeriod = 1.0f / (float)timerFreq.QuadPart;
}

CStopWatch::~CStopWatch()
{
}

bool CStopWatch::IsRunning() const
{
  return m_isRunning;
}

void CStopWatch::StartZero()
{
  m_startTick = GetTicks();
  m_isRunning = true;
}

void CStopWatch::Stop()
{
  if( m_isRunning )
  {
    m_startTick = 0;
    m_isRunning = false;
  }
}

void CStopWatch::Reset()
{
  if (m_isRunning)
    m_startTick = GetTicks();
}

float CStopWatch::GetElapsedSeconds() const
{
  LONGLONG totalTicks = m_isRunning ? (GetTicks() - m_startTick) : 0;
  return (FLOAT)totalTicks * m_timerPeriod;
}

float CStopWatch::GetElapsedMilliseconds() const
{
  return GetElapsedSeconds() * 1000.0f;
}

LONGLONG CStopWatch::GetTicks() const
{
  LARGE_INTEGER currTicks;
  QueryPerformanceCounter( &currTicks );
  return currTicks.QuadPart;
}
