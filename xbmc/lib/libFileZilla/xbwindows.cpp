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

#include "stdafx.h"

#include "xbwindows.h"



// global CWindowManager object
CWindowManager gWindowManager;


BOOL TranslateMessage(CONST MSG *lpMsg)
{
  return gWindowManager.TranslateMessage(lpMsg);
}


BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return gWindowManager.PostMessage(hWnd, Msg, wParam, lParam);
}

LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return gWindowManager.SendMessage(hWnd, Msg, wParam, lParam);
}


BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
  return gWindowManager.PeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}


BOOL GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  return gWindowManager.GetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return gWindowManager.PostThreadMessage(idThread, Msg, wParam, lParam);
}

LONG DispatchMessage(CONST MSG *lpmsg)
{
  return gWindowManager.DispatchMessage(lpmsg);
}

VOID PostQuitMessage(int nExitCode)
{
  gWindowManager.PostThreadMessage(GetCurrentThreadId(), WM_QUIT, nExitCode, 0);
}

void RemoveMessageSinks()
{
  gWindowManager.Clear();
}

UINT RegisterWindowMessage(LPCTSTR lpString)
{
  static UINT NextMessageId = 0xC000;
  static std::map<UINT, LPCTSTR> MessageMap;

  std::map<UINT, LPCTSTR>::iterator it;
  for (it = MessageMap.begin(); it != MessageMap.end(); ++it)
    if (_tcscmp((*it).second, lpString) == 0)
      return (*it).first;

  NextMessageId++;
  MessageMap[NextMessageId] = lpString;
  return NextMessageId;
}

///////////////////////////////////////////////////////////////////////////////////
// CWindowManager

CWindowManager::CWindowManager()
{
}

CWindowManager::~CWindowManager()
{
  Clear();
}

void CWindowManager::Clear()
{
  mWindowListCS.Lock();
  std::list<CWindow*>::iterator itWindow;
  for (itWindow = mWindowList.begin(); itWindow != mWindowList.end(); ++itWindow)
    delete *itWindow;
  mWindowList.clear();
  mWindowListCS.Unlock();

  mMessageSinkListCS.Lock();
  std::list<CMessageSink*>::iterator itSink;
  for (itSink = mMessageSinkList.begin(); itSink != mMessageSinkList.end(); ++itSink)
    delete *itSink;
  mMessageSinkList.clear();
  mMessageSinkListCS.Unlock();
}

BOOL CWindowManager::TranslateMessage(CONST MSG *lpMsg)
{
  return FALSE;
}


BOOL CWindowManager::GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  CMessageSink* sink = GetMessageSink(hWnd);
  if (!sink)
    return FALSE;

  /*
  CStdString str;
  str.Format(_T("0x%X : CWindowManager::GetMessage() waiting...\n"), GetCurrentThreadId());
  OutputDebugString(str);
  */

  WaitForSingleObject(sink->mEventMessageAvailable, INFINITE);
  BOOL result = PeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE);
 
  /*
  str.Format(_T("0x%X : CWindowManager::GetMessage() message 0x%X\n"), GetCurrentThreadId(), lpMsg->message);
  OutputDebugString(str);
  */

  if (lpMsg->message == WM_QUIT)
    return FALSE;
  
  return TRUE;
}
 

BOOL CWindowManager::PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  CMessageSink* sink = GetMessageSink(hWnd);

  if (!sink)
    return FALSE;
 
  MSG msg;
  msg.hwnd = hWnd;
  msg.message = Msg;
  msg.wParam = wParam;
  msg.lParam = lParam;
  
  return sink->AddMessage(msg);
}

LRESULT CWindowManager::SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  MSG msg;
  msg.hwnd = hWnd;
  msg.message = Msg;
  msg.wParam = wParam;
  msg.lParam = lParam;

  return DispatchMessage(&msg);
}

BOOL CWindowManager::PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  CMessageSink* sink = GetMessageSink(idThread);

  if (!sink)
    return FALSE;
  
  MSG msg;
  msg.hwnd = 0;
  msg.message = Msg;
  msg.wParam = wParam;
  msg.lParam = lParam;

  return sink->AddMessage(msg);
}
 


BOOL CWindowManager::PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
  // get first message of thread or windows belonging to thread
  DWORD ThreadId = GetCurrentThreadId();
  CMessageSink* sink = GetMessageSink(ThreadId);
  if (!sink)
    return FALSE;

  if (sink->GetMessage(lpMsg, wRemoveMsg))
    return TRUE;
  else
  {
    BOOL result = FALSE;

    mWindowListCS.Lock();

    std::list<CWindow*>::iterator it;
    for (it = mWindowList.begin(); it != mWindowList.end(); ++it)
      if ((*it)->mThreadId == ThreadId)
      {
        sink = GetMessageSink((*it)->mHWnd);
        if (!sink)
          break;

        if (sink->GetMessage(lpMsg, wRemoveMsg))
        {
          result = TRUE;
          break;
        }

      }

    mWindowListCS.Unlock();

    return result;
  }

  return FALSE;
}


LONG CWindowManager::DispatchMessage(CONST MSG *lpmsg)
{
  if (lpmsg == NULL || lpmsg->hwnd == NULL)
    return 0;

  LRESULT result = 0;
  
  CWindow* window = GetWindow(lpmsg->hwnd);
  
  if (window && window->mWindowProc)
    result = window->mWindowProc(lpmsg->hwnd, lpmsg->message, lpmsg->wParam, lpmsg->lParam);
  
  return result;
}


BOOL CWindowManager::DestroyWindow(HWND hWnd)
{
  mWindowListCS.Lock();
  
  std::list<CWindow*>::iterator it;
  for (it = mWindowList.begin(); it != mWindowList.end(); ++it)
    if ((*it)->mHWnd == hWnd)
    {
      delete *it;
      mWindowList.erase(it);
      break;
    }

  mWindowListCS.Unlock();
  return TRUE;
}

HWND CWindowManager::CreateWindow(WindowProcCallback* WindowProc, LONG UserData)
{
  mWindowListCS.Lock();
  mWindowList.push_back(new CWindow(WindowProc, UserData));
  HWND hwnd = mWindowList.back()->mHWnd;
  mWindowListCS.Unlock();
  return hwnd;
}

CWindow* CWindowManager::GetWindow(HWND hWnd)
{
  if (hWnd == 0)
    return NULL;

  CWindow* window = NULL;
  
  mWindowListCS.Lock();
  
  std::list<CWindow*>::iterator it;
  for (it = mWindowList.begin(); it != mWindowList.end(); ++it)
    if ((*it)->mHWnd == hWnd)
    {
      window = *it;
      break;
    }

  mWindowListCS.Unlock();

  return window;
}


CMessageSink* CWindowManager::GetMessageSink(HWND hWnd)
{
  if (!hWnd)
    return GetMessageSink(GetCurrentThreadId());

  CWindow* window = GetWindow(hWnd);

  if (!window)
    return NULL;

  return GetMessageSink(window->mThreadId);
}

CMessageSink* CWindowManager::GetMessageSink(DWORD ThreadId)
{
  CMessageSink* sink = NULL;
  mMessageSinkListCS.Lock();
  std::list<CMessageSink*>::iterator it;
  for (it = mMessageSinkList.begin(); it != mMessageSinkList.end(); ++it)
    if ((*it)->mThreadId == ThreadId)
      break;
    
  if (it == mMessageSinkList.end())
  {
    sink = new CMessageSink(ThreadId);
    mMessageSinkList.push_back(sink);
  }
  else
    sink = *it;

  mMessageSinkListCS.Unlock();
  return sink;
}

HWND CWindowManager::CreateWindow(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle,
                    int x, int y, int nWidth, int nHeight, HWND hWndParent, 
                    HMENU hMenu, HANDLE hInstance, LPVOID lpParam)
{
  CStdString str(lpClassName);
  mRegisteredWindowClassMapCS.Lock();

  HWND hWnd = NULL;
  std::map<CStdString, WNDCLASSEX>::iterator it = mRegisteredWindowClassMap.find(str);
  if (it != mRegisteredWindowClassMap.end())
    hWnd = CreateWindow((*it).second.lpfnWndProc, 0);

  mRegisteredWindowClassMapCS.Unlock();

  return hWnd;
}

ATOM CWindowManager::RegisterClassEx(CONST WNDCLASSEX *lpwcx)
{
  ASSERT(lpwcx);
  if (!lpwcx)
    return NULL;

  CStdString str(lpwcx->lpszClassName);
  mRegisteredWindowClassMapCS.Lock();
  std::map<CStdString, WNDCLASSEX>::iterator it = mRegisteredWindowClassMap.find(str);
  if (it == mRegisteredWindowClassMap.end())
    mRegisteredWindowClassMap[str] = *lpwcx;

  mRegisteredWindowClassMapCS.Unlock();

  return 0;
}

////////////////////////////////////////////////////////////////////////
// CWindow

HWND CWindow::mNextHwnd = (HWND)1;

CWindow::CWindow(WindowProcCallback* WindowProc, LONG UserData)
: mHWnd(mNextHwnd++)
, mThreadId(GetCurrentThreadId())
, mWindowProc(WindowProc)
, mUserData(UserData)
{
}

////////////////////////////////////////////////////////////////////////
// CMessageSink

CMessageSink::CMessageSink(DWORD ThreadId)
: mThreadId(ThreadId)
{
  mEventMessageAvailable = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CMessageSink::~CMessageSink()
{
  mMessageQueueCS.Lock();

  mMessageQueue.clear();

  mMessageQueueCS.Unlock();
  CloseHandle(mEventMessageAvailable);
}



BOOL CMessageSink::AddMessage(MSG Msg)
{
  mMessageQueueCS.Lock();

  mMessageQueue.push_back(Msg);

  mMessageQueueCS.Unlock();

  SetEvent(mEventMessageAvailable);

  return TRUE;
}

BOOL CMessageSink::GetMessage(LPMSG lpMsg, UINT wRemoveMsg)
{
  mMessageQueueCS.Lock();
  if (mMessageQueue.size() == 0)
  {
    mMessageQueueCS.Unlock();
    return FALSE;
  }
  
  *lpMsg = mMessageQueue.front();
  if (wRemoveMsg == PM_REMOVE)
  {
    mMessageQueue.pop_front();

    if (mMessageQueue.size() == 0)
      ResetEvent(mEventMessageAvailable);
  }

  mMessageQueueCS.Unlock();
  return TRUE;
}

LRESULT DefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return 0;
}

BOOL DestroyWindow(HWND hWnd)
{
  return gWindowManager.DestroyWindow(hWnd);
}




 
/////////////////////////////////////////////////


HMODULE GetModuleHandle(LPCTSTR lpModuleName)
{
  return 0;
}

// used for retrieving the install dir
DWORD GetModuleFileName(HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
{
  // for now, only return full path
  ASSERT(hModule == 0); 

  const char* path = XBFILEZILLA(GetConfigurationPath());

  _tcsncpy(lpFilename, path, nSize);
  return _tcslen(path);
}

int MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
  // TODO
  return 0;
}
  
ATOM RegisterClassEx(CONST WNDCLASSEX *lpwcx)
{
  return gWindowManager.RegisterClassEx(lpwcx);
}


HWND CreateWindow(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle,
        int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
        HANDLE hInstance, LPVOID lpParam)
{
  return gWindowManager.CreateWindow(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight,
                              hWndParent, hMenu, hInstance, lpParam);
}

LONG SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
  if (nIndex == GWL_USERDATA)
  {
    CWindow* window = gWindowManager.GetWindow(hWnd);
    if (window)
    {
      LONG oldLong = window->mUserData;
      window->mUserData = dwNewLong;
      return oldLong;
    }
  }

  return 0;
}

LONG GetWindowLong(HWND hWnd, int nIndex)
{
  if (nIndex == GWL_USERDATA)
  {
    CWindow* window = gWindowManager.GetWindow(hWnd);
    if (window)
      return window->mUserData;
  }

  return 0;
}

 
