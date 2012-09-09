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

#ifndef WINDOW_SYSTEM_WIN32_H
#define WINDOW_SYSTEM_WIN32_H

#include "windowing/WinSystem.h"

struct MONITOR_DETAILS
{
  // Windows desktop info
  int       ScreenWidth;
  int       ScreenHeight;
  int       RefreshRate;
  int       Bpp;
  bool      Interlaced;

  HMONITOR  hMonitor;
  char      MonitorName[128];
  char      CardName[128];
  char      DeviceName[128];
  int       ScreenNumber; // XBMC POV, not Windows. Windows primary is XBMC #0, then each secondary is +1.
};

#ifndef WM_GESTURE

#define WM_GESTURE       0x0119
#define WM_GESTURENOTIFY 0x011A

// Gesture Information Flags
#define GF_BEGIN   0x00000001
#define GF_INERTIA 0x00000002
#define GF_END     0x00000004

// Gesture IDs
#define GID_BEGIN                       1
#define GID_END                         2
#define GID_ZOOM                        3
#define GID_PAN                         4
#define GID_ROTATE                      5
#define GID_TWOFINGERTAP                6
#define GID_PRESSANDTAP                 7
#define GID_ROLLOVER                    GID_PRESSANDTAP

#define GC_ALLGESTURES 0x00000001

// Zoom Gesture Confiration Flags
#define GC_ZOOM 0x00000001

// Pan Gesture Configuration Flags
#define GC_PAN 0x00000001
#define GC_PAN_WITH_SINGLE_FINGER_VERTICALLY 0x00000002
#define GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY 0x00000004
#define GC_PAN_WITH_GUTTER 0x00000008
#define GC_PAN_WITH_INERTIA 0x00000010

// Rotate Gesture Configuration Flags
#define GC_ROTATE 0x00000001

// Two finger tap configuration flags
#define GC_TWOFINGERTAP 0x00000001

// Press and tap Configuration Flags
#define GC_PRESSANDTAP 0x00000001
#define GC_ROLLOVER GC_PRESSANDTAP

typedef struct _GESTUREINFO {
  UINT      cbSize;
  DWORD     dwFlags;
  DWORD     dwID;
  HWND      hwndTarget;
  POINTS    ptsLocation;
  DWORD     dwInstanceID;
  DWORD     dwSequenceID;
  ULONGLONG ullArguments;
  UINT      cbExtraArgs;
}GESTUREINFO, *PGESTUREINFO;

// GESTURECONFIG struct defintion
typedef struct tagGESTURECONFIG {
    DWORD dwID;                     // gesture ID
    DWORD dwWant;                   // settings related to gesture ID that are to be turned on
    DWORD dwBlock;                  // settings related to gesture ID that are to be turned off
} GESTURECONFIG, *PGESTURECONFIG;

/*
 * Gesture notification structure
 *   - The WM_GESTURENOTIFY message lParam contains a pointer to this structure.
 *   - The WM_GESTURENOTIFY message notifies a window that gesture recognition is
 *     in progress and a gesture will be generated if one is recognized under the
 *     current gesture settings.
 */
typedef struct tagGESTURENOTIFYSTRUCT {
    UINT cbSize;                    // size, in bytes, of this structure
    DWORD dwFlags;                  // unused
    HWND hwndTarget;                // handle to window targeted by the gesture
    POINTS ptsLocation;             // starting location
    DWORD dwInstanceID;             // internally used
} GESTURENOTIFYSTRUCT, *PGESTURENOTIFYSTRUCT;

DECLARE_HANDLE(HGESTUREINFO);

#endif

class CWinSystemWin32 : public CWinSystemBase
{
public:
  CWinSystemWin32();
  virtual ~CWinSystemWin32();

  // CWinSystemBase
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction);
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop);
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays);
  virtual void UpdateResolutions();
  virtual bool CenterWindow();
  virtual void NotifyAppFocusChange(bool bGaining);
  virtual int  GetNumScreens() { return m_MonitorsInfo.size(); };
  virtual int  GetCurrentScreen();
  virtual void ShowOSMouse(bool show);
  virtual bool WindowedMode() { return true; }
  virtual bool HasInertialGestures(){ return true; }//if win32 has touchscreen - it uses the win32 gesture api for inertial scrolling

  virtual bool Minimize();
  virtual bool Restore();
  virtual bool Hide();
  virtual bool Show(bool raise = true);

  // CWinSystemWin32
  HWND GetHwnd() { return m_hWnd; }
  bool IsAlteringWindow() { return m_IsAlteringWindow; }

  // touchscreen support
  typedef BOOL (WINAPI *pGetGestureInfo)(HGESTUREINFO, PGESTUREINFO);
  typedef BOOL (WINAPI *pSetGestureConfig)(HWND, DWORD, UINT, PGESTURECONFIG, UINT);
  typedef BOOL (WINAPI *pCloseGestureInfoHandle)(HGESTUREINFO);
  pGetGestureInfo         PtrGetGestureInfo;
  pSetGestureConfig       PtrSetGestureConfig;
  pCloseGestureInfoHandle PtrCloseGestureInfoHandle;

protected:
  bool ChangeResolution(RESOLUTION_INFO res);
  virtual bool ResizeInternal(bool forceRefresh = false);
  virtual bool UpdateResolutionsInternal();
  virtual bool CreateBlankWindows();
  virtual bool BlankNonActiveMonitors(bool bBlank);
  const MONITOR_DETAILS &GetMonitor(int screen) const;
  void RestoreDesktopResolution(int screen);
  RECT ScreenRect(int screen);
  /*!
   \brief Adds a resolution to the list of resolutions if we don't already have it
   \param res resolution to add.
   */
  void AddResolution(const RESOLUTION_INFO &res);

  HWND m_hWnd;
  std::vector<HWND> m_hBlankWindows;
  HDC m_hDC;
  HINSTANCE m_hInstance;
  HICON m_hIcon;
  std::vector<MONITOR_DETAILS> m_MonitorsInfo;
  int m_nPrimary;
  bool m_ValidWindowedPosition;
  bool m_IsAlteringWindow;
};

extern HWND g_hWnd;

#endif // WINDOW_SYSTEM_H

