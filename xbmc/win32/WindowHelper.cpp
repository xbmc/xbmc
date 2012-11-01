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

#include "WindowHelper.h"
#include "../Util.h"

extern HWND g_hWnd;

using namespace std;

CWHelper g_windowHelper;

CWHelper::CWHelper(void) : CThread("CWHelper")
{
  m_hwnd = NULL;
  m_hProcess = NULL;
}

CWHelper::~CWHelper(void)
{
  StopThread();
  m_hwnd = NULL;
  if(m_hProcess != NULL)
  {
    CloseHandle(m_hProcess);
    m_hProcess = NULL;
  }
}

void CWHelper::OnStartup()
{
  if((m_hwnd == NULL) && (m_hProcess == NULL))
    return;

  // Minimize XBMC if not already
  ShowWindow(g_hWnd,SW_MINIMIZE);
  if(m_hwnd != NULL)
    ShowWindow(m_hwnd,SW_RESTORE);

  OutputDebugString("WindowHelper thread started\n");
}

void CWHelper::OnExit()
{
  // Bring back XBMC window
  ShowWindow(g_hWnd,SW_RESTORE);
  SetForegroundWindow(g_hWnd);
  m_hwnd = NULL;
  if(m_hProcess != NULL)
  {
    CloseHandle(m_hProcess);
    m_hProcess = NULL;
  }
  LockSetForegroundWindow(LSFW_LOCK);
  OutputDebugString("WindowHelper thread ended\n");
}

void CWHelper::Process()
{
  while (( !m_bStop ))
  {
    if(WaitForSingleObject(m_hProcess,500) != WAIT_TIMEOUT)
      break;
    /*if((m_hwnd != NULL) && (IsIconic(m_hwnd) == TRUE))
      break;*/
  }
}

void CWHelper::SetHWND(HWND hwnd)
{
  m_hwnd = hwnd;
}

void CWHelper::SetHANDLE(HANDLE hProcess)
{
  m_hProcess = hProcess;
}

