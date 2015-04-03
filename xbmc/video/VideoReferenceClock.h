#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "threads/Thread.h"
#include "threads/CriticalSection.h"

class CVideoSync;

class CVideoReferenceClock : public CThread
{
  public:
    CVideoReferenceClock();
    virtual ~CVideoReferenceClock();

    int64_t GetTime(bool interpolated = true);
    int64_t GetFrequency();
    void    SetSpeed(double Speed);
    double  GetSpeed();
    double  GetRefreshRate(double* interval = NULL);
    int64_t Wait(int64_t Target);
    bool    GetClockInfo(int& MissedVblanks, double& ClockSpeed, double& RefreshRate);
    void    SetFineAdjust(double fineadjust);
    void    RefreshChanged();
    void    Start();
    void    Stop();

  private:
    void    Process();
    void    UpdateRefreshrate();
    void    SendVblankSignal();
    void    UpdateClock(int NrVBlanks, bool CheckMissed);
    double  UpdateInterval();
    int64_t TimeOfNextVblank();
    static void CBUpdateClock(int NrVBlanks, uint64_t time);

    int64_t m_CurrTime;          //the current time of the clock when using vblank as clock source
    int64_t m_LastIntTime;       //last interpolated clock value, to make sure the clock doesn't go backwards
    double  m_CurrTimeFract;     //fractional part that is lost due to rounding when updating the clock
    double  m_ClockSpeed;        //the frequency of the clock set by dvdplayer
    int64_t m_ClockOffset;       //the difference between the vblank clock and systemclock, set when vblank clock is stopped
    int64_t m_SystemFrequency;   //frequency of the systemclock
    double  m_fineadjust;

    bool    m_UseVblank;         //set to true when vblank is used as clock source
    double  m_RefreshRate;       //current refreshrate
    int     m_MissedVblanks;     //number of clock updates missed by the vblank clock
    int     m_TotalMissedVblanks;//total number of clock updates missed, used by codec information screen
    int64_t m_VblankTime;        //last time the clock was updated when using vblank as clock

    CEvent  m_VblankEvent;        //set when a vblank happens

    CCriticalSection m_CritSection;

    CVideoSync *m_pVideoSync;
};

extern CVideoReferenceClock g_VideoReferenceClock;
