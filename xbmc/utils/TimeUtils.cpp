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

#include "system.h"
#include "TimeUtils.h"

int64_t CurrentHostCounter(void)
{
  LARGE_INTEGER PerformanceCount;
  QueryPerformanceCounter(&PerformanceCount);
  return( (int64_t)PerformanceCount.QuadPart );
}

int64_t CurrentHostFrequency(void)
{
  LARGE_INTEGER Frequency;
  QueryPerformanceFrequency(&Frequency);
  return( (int64_t)Frequency.QuadPart );
}

unsigned int CTimeUtils::frameTime = 0;

void CTimeUtils::UpdateFrameTime()
{
  frameTime = GetTimeMS();
}

unsigned int CTimeUtils::GetFrameTime()
{
  return frameTime;
}

unsigned int CTimeUtils::GetTimeMS()
{
  return timeGetTime();
}

