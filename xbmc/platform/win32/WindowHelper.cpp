/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WindowHelper.h"

extern HWND g_hWnd;

CWHelper g_windowHelper;

CWHelper::CWHelper(void) : CThread("WindowHelper")
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

  OutputDebugString(L"WindowHelper thread started\n");
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
  OutputDebugString(L"WindowHelper thread ended\n");
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

