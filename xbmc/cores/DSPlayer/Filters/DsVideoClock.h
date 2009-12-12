#ifndef _DSVIDEOCLOCK_H
#define _DSVIDEOCLOCK_H
#pragma once

using namespace std;

#include <d3d9.h>


#include "D3DResource.h"
#include "system.h" // for HAS_XRANDR, and Win32 types
#include "Thread.h"
//#include "event.h"
#include "utils/CriticalSection.h"


class CDSVideoCallback : public ID3DResource
{
public:
    void Reset();
    // ID3DResource
  void OnDestroyDevice();
  void OnCreateDevice();
  //void OnLostDevice();
  //void OnResetDevice();
  void Aquire();
  void Release();
  bool IsValid();

private:
  bool m_devicevalid;
  bool m_deviceused;

  CCriticalSection m_critsection;
  CEvent           m_createevent;
  CEvent           m_releaseevent;
};

class CDSVideoClock : public CThread
{
public:
    CDSVideoClock();

    int64_t GetTime();
    int64_t GetFrequency();
    void    SetSpeed(double Speed);
    double  GetSpeed();
    int     GetRefreshRate();
    int64_t Wait(int64_t Target);
    bool    WaitStarted(int MSecs);
    bool    GetClockInfo(int& MissedVblanks, double& ClockSpeed, int& RefreshRate);

  private:
    void    Process();
    bool    UpdateRefreshrate(bool Forced = false);
    void    SendVblankSignal();
    void    UpdateClock(int NrVBlanks, bool CheckMissed);
    int64_t TimeOfNextVblank();

    int64_t m_CurrTime;          //the current time of the clock when using vblank as clock source
    int64_t m_AdjustedFrequency; //the frequency of the clock set by dvdplayer
    int64_t m_ClockOffset;       //the difference between the vblank clock and systemclock, set when vblank clock is stopped
    int64_t m_LastRefreshTime;   //last time we updated the refreshrate
    int64_t m_SystemFrequency;   //frequency of the systemclock

    bool    m_UseVblank;         //set to true when vblank is used as clock source
    int64_t m_RefreshRate;       //current refreshrate
    int     m_PrevRefreshRate;   //previous refreshrate, used for log printing and getting refreshrate from nvidia-settings
    int     m_MissedVblanks;     //number of clock updates missed by the vblank clock
    int     m_TotalMissedVblanks;//total number of clock updates missed, used by codec information screen
    int64_t m_VblankTime;        //last time the clock was updated when using vblank as clock

    CEvent  m_Started;            //set when the vblank clock is started
    CEvent  m_VblankEvent;        //set when a vblank happens

    CCriticalSection m_CritSection;
    bool   SetupD3D();
    double MeasureRefreshrate(int MSecs);
    void   RunD3D();
    void   CleanupD3D();

    LPDIRECT3DDEVICE9 m_D3dDev;
    //CD3DCallback      m_D3dCallback;

    unsigned int  m_Width;
    unsigned int  m_Height;
    CDSVideoCallback  m_DsVideoCallback;

};

#endif //_DSVIDEOCLOCK_H