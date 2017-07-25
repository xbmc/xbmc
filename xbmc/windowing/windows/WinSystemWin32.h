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

#ifndef WINDOW_SYSTEM_WIN32_H
#define WINDOW_SYSTEM_WIN32_H

#include "guilib/DispResource.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "windowing/WinSystem.h"
#include <vector>

static const DWORD WINDOWED_STYLE = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
static const DWORD WINDOWED_EX_STYLE = NULL;
static const DWORD FULLSCREEN_WINDOW_STYLE = WS_POPUP | WS_CLIPCHILDREN;
static const DWORD FULLSCREEN_WINDOW_EX_STYLE = WS_EX_APPWINDOW;

/* Controls the way the window appears and behaves. */
enum WINDOW_STATE 
{
  WINDOW_STATE_FULLSCREEN = 1,    // Exclusive fullscreen
  WINDOW_STATE_FULLSCREEN_WINDOW, // Non-exclusive fullscreen window
  WINDOW_STATE_WINDOWED,          //Movable window with border
  WINDOW_STATE_BORDERLESS         //Non-movable window with no border
};

static const char* window_state_names[] =
{
  "unknown",
  "true fullscreen",
  "windowed fullscreen",
  "windowed",
  "borderless"
};

/* WINDOW_STATE restricted to fullscreen modes. */
enum WINDOW_FULLSCREEN_STATE 
{
  WINDOW_FULLSCREEN_STATE_FULLSCREEN = WINDOW_STATE_FULLSCREEN,
  WINDOW_FULLSCREEN_STATE_FULLSCREEN_WINDOW = WINDOW_STATE_FULLSCREEN_WINDOW
};

/* WINDOW_STATE restricted to windowed modes. */
enum WINDOW_WINDOW_STATE 
{
  WINDOW_WINDOW_STATE_WINDOWED = WINDOW_STATE_WINDOWED,
  WINDOW_WINDOW_STATE_BORDERLESS = WINDOW_STATE_BORDERLESS
};

struct MONITOR_DETAILS
{
  // Windows desktop info
  int       ScreenWidth;
  int       ScreenHeight;
  int       RefreshRate;
  int       Bpp;
  bool      Interlaced;

  HMONITOR  hMonitor;
  std::wstring MonitorNameW;
  std::wstring CardNameW;
  std::wstring DeviceNameW;
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

// Zoom Gesture Configuration Flags
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

#define GID_ROTATE_ANGLE_TO_ARGUMENT(_arg_)     ((USHORT)((((_arg_) + 2.0 * 3.14159265) / (4.0 * 3.14159265)) * 65535.0))
#define GID_ROTATE_ANGLE_FROM_ARGUMENT(_arg_)   ((((double)(_arg_) / 65535.0) * 4.0 * 3.14159265) - 2.0 * 3.14159265)

DECLARE_HANDLE(HGESTUREINFO);

#endif

class CWinSystemWin32 : public CWinSystemBase
{
public:
  CWinSystemWin32();
  virtual ~CWinSystemWin32();

  // CWinSystemBase overrides
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void UpdateResolutions() override;
  bool CenterWindow() override;
  virtual void NotifyAppFocusChange(bool bGaining) override;
  int  GetNumScreens() override { return m_MonitorsInfo.size(); };
  int  GetCurrentScreen() override;
  void ShowOSMouse(bool show) override;
  bool HasInertialGestures() override { return true; }//if win32 has touchscreen - it uses the win32 gesture api for inertial scrolling
  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  std::string GetClipboardText() override;
  // videosync
  std::unique_ptr<CVideoSync> GetVideoSync(void *clock) override;

  bool WindowedMode() const { return m_state != WINDOW_STATE_FULLSCREEN; }
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  // CWinSystemWin32
  HWND GetHwnd() const { return m_hWnd; }
  bool IsAlteringWindow() const { return m_IsAlteringWindow; }
  virtual bool DPIChanged(WORD dpi, RECT windowRect) const;
  bool IsMinimized() const { return m_bMinimized; }
  void SetMinimized(bool minimized) { m_bMinimized = minimized; }

  // touchscreen support
  typedef BOOL(WINAPI *pGetGestureInfo)(HGESTUREINFO, PGESTUREINFO);
  typedef BOOL(WINAPI *pSetGestureConfig)(HWND, DWORD, UINT, PGESTURECONFIG, UINT);
  typedef BOOL(WINAPI *pCloseGestureInfoHandle)(HGESTUREINFO);
  typedef BOOL(WINAPI *pEnableNonClientDpiScaling)(HWND);
  pGetGestureInfo         PtrGetGestureInfo;
  pSetGestureConfig       PtrSetGestureConfig;
  pCloseGestureInfoHandle PtrCloseGestureInfoHandle;
  pEnableNonClientDpiScaling PtrEnableNonClientDpiScaling;

protected:
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override = 0;
  virtual void UpdateStates(bool fullScreen);
  WINDOW_STATE GetState(bool fullScreen) const;
  virtual void SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res) = 0;
  virtual void ReleaseBackBuffer() = 0;
  virtual void CreateBackBuffer() = 0;
  virtual void ResizeDeviceBuffers() = 0;
  virtual bool IsStereoEnabled() = 0;
  virtual void AdjustWindow(bool forceResize = false);
  void CenterCursor() const;

  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

  bool ChangeResolution(const RESOLUTION_INFO& res, bool forceChange = false);
  virtual bool UpdateResolutionsInternal();
  virtual bool CreateBlankWindows();
  virtual bool BlankNonActiveMonitors(bool bBlank);
  const MONITOR_DETAILS* GetMonitor(int screen) const;
  void RestoreDesktopResolution(int screen);
  RECT ScreenRect(int screen) const;

  /*!
   \brief Adds a resolution to the list of resolutions if we don't already have it
   \param res resolution to add.
   */
  static void AddResolution(const RESOLUTION_INFO &res);

  void OnDisplayLost();
  void OnDisplayReset();
  void OnDisplayBack();
  void ResolutionChanged();
  static void SetForegroundWindowInternal(HWND hWnd);

  HWND m_hWnd;
  std::vector<HWND> m_hBlankWindows;
  HINSTANCE m_hInstance;
  HICON m_hIcon;
  std::vector<MONITOR_DETAILS> m_MonitorsInfo;
  int m_nPrimary;
  bool m_ValidWindowedPosition;
  bool m_IsAlteringWindow;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  bool m_delayDispReset;
  XbmcThreads::EndTime m_dispResetTimer;

  WINDOW_STATE m_state;                       // the state of the window
  WINDOW_FULLSCREEN_STATE m_fullscreenState;  // the state of the window when in fullscreen
  WINDOW_WINDOW_STATE m_windowState;          // the state of the window when in windowed
  DWORD m_windowStyle;                        // the style of the window
  DWORD m_windowExStyle;                      // the ex style of the window
  bool m_inFocus;
  bool m_bMinimized;
};

extern HWND g_hWnd;

#endif // WINDOW_SYSTEM_H

