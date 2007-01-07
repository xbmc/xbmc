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

#ifndef __XBWINDOWS_H__
#define __XBWINDOWS_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning (disable: 4018)
#include <xtl.h>
#include <deque>
#include <list>
#include <map>

#include "..\..\..\guilib\stdstring.h"
#include "xbdefines.h"


typedef LRESULT (CALLBACK WindowProcCallback)( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
typedef WindowProcCallback* WNDPROC;

typedef struct _WNDCLASSEX { 
    UINT    cbSize; 
    UINT    style; 
    WNDPROC lpfnWndProc; 
    int     cbClsExtra; 
    int     cbWndExtra; 
    HANDLE  hInstance; 
    HICON   hIcon; 
    HCURSOR hCursor; 
    HBRUSH  hbrBackground; 
    LPCTSTR lpszMenuName; 
    LPCTSTR lpszClassName; 
    HICON   hIconSm; 
} WNDCLASSEX; 

/*
 * Message structure
 */
typedef struct tagMSG {
    HWND        hwnd;
    UINT        message;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       time;
    POINT       pt;
} MSG, *PMSG, NEAR *NPMSG, FAR *LPMSG;



BOOL PeekMessage(
  LPMSG lpMsg,         // pointer to structure for message
  HWND hWnd,           // handle to window
  UINT wMsgFilterMin,  // first message
  UINT wMsgFilterMax,  // last message
  UINT wRemoveMsg      // removal flags
);

BOOL GetMessage(
  LPMSG lpMsg,         // address of structure with message
  HWND hWnd,           // handle of window
  UINT wMsgFilterMin,  // first message
  UINT wMsgFilterMax   // last message
);
 
BOOL PostMessage(
  HWND hWnd,      // handle of destination window
  UINT Msg,       // message to post
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
);
 
BOOL PostThreadMessage(
  DWORD idThread, // thread identifier
  UINT Msg,       // message to post
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
);
 
BOOL TranslateMessage(
  CONST MSG *lpMsg   // address of structure with message
);

LONG DispatchMessage(
  CONST MSG *lpmsg   // pointer to structure with message
);

BOOL DestroyWindow(
  HWND hWnd   // handle to window to destroy
);

VOID PostQuitMessage(
  int nExitCode   // exit code
);

UINT RegisterWindowMessage(
  LPCTSTR lpString   // address of message string
);

LRESULT SendMessage(
  HWND hWnd,      // handle of destination window
  UINT Msg,       // message to send
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
);

void RemoveMessageSinks();

class CMessageSink
{
public:
  CMessageSink(DWORD ThreadId);
  ~CMessageSink();

  DWORD mThreadId;
  
  BOOL AddMessage(MSG Msg);
  BOOL GetMessage(LPMSG lpMsg, UINT wRemoveMsg);
  BOOL GetWndMessage(LPMSG lpMsg, HWND hWnd, UINT wRemoveMsg);

  std::deque<MSG> mMessageQueue;
  CCriticalSectionWrapper mMessageQueueCS;
  HANDLE mEventMessageAvailable;
};


class CWindow
{
public:
  CWindow(WindowProcCallback* WindowProc, LONG UserData);
  
  HWND mHWnd;
  DWORD mThreadId;
  
  WindowProcCallback* mWindowProc;
  LONG mUserData;

private:
  static HWND mNextHwnd;
};



class CWindowManager
{
public:
  CWindowManager();
  virtual ~CWindowManager();

  void Clear();

  BOOL GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
  BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
  BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
  LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
  BOOL PostThreadMessage(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);
  BOOL TranslateMessage(CONST MSG *lpMsg);
  LONG DispatchMessage(CONST MSG *lpmsg);

  HWND CreateWindow(WindowProcCallback* WindowProc, LONG UserData);
  BOOL DestroyWindow(HWND hWnd);

  CWindow* GetWindow(HWND hWnd);

  CMessageSink* GetMessageSink(HWND hWnd);
  CMessageSink* GetMessageSink(DWORD ThreadId);

    
  ATOM RegisterClassEx(CONST WNDCLASSEX *lpwcx);
  HWND CreateWindow(LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle,
                    int x, int y, int nWidth, int nHeight, HWND hWndParent, 
                    HMENU hMenu, HANDLE hInstance, LPVOID lpParam);

  std::list<CWindow*> mWindowList;
  CCriticalSectionWrapper mWindowListCS;

  std::list<CMessageSink*> mMessageSinkList;
  CCriticalSectionWrapper mMessageSinkListCS;

  std::map<CStdString, WNDCLASSEX> mRegisteredWindowClassMap;
  CCriticalSectionWrapper mRegisteredWindowClassMapCS;
};


LRESULT DefWindowProc(
  HWND hWnd,      // handle to window
  UINT Msg,       // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
);



int WSAAsyncSelect(
  SOCKET s,
  HWND hWnd,          
  unsigned int wMsg,  
  long lEvent         
);



//////////////////////////////////////////////////////////////
HMODULE GetModuleHandle(
  LPCTSTR lpModuleName   // address of module name to return handle 
                         // for
);

// used by FileZilla for retrieving the install dir
DWORD GetModuleFileName(
  HMODULE hModule,    // handle to module to find filename for
  LPTSTR lpFilename,  // pointer to buffer to receive module path
  DWORD nSize         // size of buffer, in characters
);
 



ATOM RegisterClassEx(
  CONST WNDCLASSEX *lpwcx  // address of structure with class data
);

HWND CreateWindow(
  LPCTSTR lpClassName,  // pointer to registered class name
  LPCTSTR lpWindowName, // pointer to window name
  DWORD dwStyle,        // window style
  int x,                // horizontal position of window
  int y,                // vertical position of window
  int nWidth,           // window width
  int nHeight,          // window height
  HWND hWndParent,      // handle to parent or owner window
  HMENU hMenu,          // handle to menu or child-window identifier
  HANDLE hInstance,     // handle to application instance
  LPVOID lpParam        // pointer to window-creation data
);

/*
 * Window field offsets for GetWindowLong()
 */
#define GWL_WNDPROC         (-4)
#define GWL_HINSTANCE       (-6)
#define GWL_HWNDPARENT      (-8)
#define GWL_STYLE           (-16)
#define GWL_EXSTYLE         (-20)
#define GWL_USERDATA        (-21)
#define GWL_ID              (-12)

LONG SetWindowLong(
  HWND hWnd,       // handle of window
  int nIndex,      // offset of value to set
  LONG dwNewLong   // new value
);

LONG GetWindowLong(
  HWND hWnd,  // handle of window
  int nIndex  // offset of value to retrieve
);
 
#define MB_ICONEXCLAMATION          0x00000030L

int MessageBox(
  HWND hWnd,          // handle of owner window
  LPCTSTR lpText,     // address of text in message box
  LPCTSTR lpCaption,  // address of title of message box
  UINT uType          // style of message box
);
 


#endif // __XBWINDOWS_H__
