/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


///////////////////////////////////////////////////////////////
// Timer

#include "stdafx.h"
#include "xbtimer.h"


class CTimerEntry 
{
public:
  CTimerEntry(HWND hWnd, UINT nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
    : mhWnd(hWnd)
    , mnIDEvent(nIDEvent)
    , muElapse(uElapse)
    , mlpTimerFunc(lpTimerFunc)
  {
    mTimerId = timeSetEvent(muElapse, 1, TimerEntryCallback, (DWORD)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
    if (!mhWnd)
      mThreadId = GetCurrentThreadId();
  }

  ~CTimerEntry()
  {
    if (mTimerId)
      timeKillEvent(mTimerId);
  }

  static void CALLBACK TimerEntryCallback(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
  {
    ((CTimerEntry*)dwUser)->Execute();
  }
 
  

  void Execute()
  {
    //OutputDebugString(_T("CTimerEntry::Execute()\n"));
    if (mTimerId)
      if (mlpTimerFunc)
        mlpTimerFunc(mhWnd, WM_TIMER, mnIDEvent, GetTickCount());
      else
      if (mhWnd)
        PostMessage(mhWnd, WM_TIMER, mTimerId, (LPARAM)mlpTimerFunc);
      else
        PostThreadMessage(mThreadId, WM_TIMER, mTimerId, (LPARAM)mlpTimerFunc);
  }
    

  UINT      mTimerId;
  HWND      mhWnd;
  UINT      mnIDEvent;
  UINT      muElapse;
  TIMERPROC mlpTimerFunc;
  DWORD     mThreadId;
};


std::list<CTimerEntry*> gTimerEntryList;
CCriticalSectionWrapper gTimerEntryListCS;

UINT SetTimer(HWND hWnd, UINT nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc)
{
  gTimerEntryListCS.Lock();
  gTimerEntryList.push_back(new CTimerEntry(hWnd, nIDEvent, uElapse, lpTimerFunc));
  UINT id = gTimerEntryList.back()->mTimerId;
  gTimerEntryListCS.Unlock();

  //CStdString str;
  //str.Format(_T("0x%X : SetTimer() hWnd 0x%X, mTimerId 0x%X\n"), GetCurrentThreadId(), hWnd, id);
  //OutputDebugString(str);

  return id;
}


BOOL KillTimer(HWND hWnd, UINT uIDEvent)
{
  gTimerEntryListCS.Lock();
  BOOL result = FALSE;
  std::list<CTimerEntry*>::iterator it;
  for (it = gTimerEntryList.begin(); it != gTimerEntryList.end(); ++it)
    if ((*it)->mTimerId == uIDEvent)
    {
      //CStdString str;
      //str.Format(_T("0x%X : KillTimer() hWnd 0x%X, mTimerId 0x%X\n"), GetCurrentThreadId(), hWnd, uIDEvent);
      //OutputDebugString(str);

      delete *it;
      gTimerEntryList.erase(it);
      result = TRUE;
      break;
    }

  gTimerEntryListCS.Unlock();
  return result;
}