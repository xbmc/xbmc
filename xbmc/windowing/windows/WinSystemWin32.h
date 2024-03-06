/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DispResource.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "windowing/WinSystem.h"

#include <shellapi.h>
#include <vector>

static const DWORD WINDOWED_STYLE = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
static const DWORD WINDOWED_EX_STYLE = NULL;
static const DWORD FULLSCREEN_WINDOW_STYLE = WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN;
static const DWORD FULLSCREEN_WINDOW_EX_STYLE = WS_EX_APPWINDOW;
static const UINT ID_TIMER_HDR = 34U;

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
  int ScreenWidth;
  int ScreenHeight;
  int RefreshRate;
  int Bpp;
  int DisplayId;
  bool Interlaced;
  bool IsPrimary;

  HMONITOR  hMonitor;
  std::wstring MonitorNameW;
  std::wstring CardNameW;
  std::wstring DeviceNameW;
  std::wstring DeviceStringW; // GDI device, for migration of the monitor setting from Kodi < 21
};

class CIRServerSuite;

class CWinSystemWin32 : public CWinSystemBase
{
public:
  CWinSystemWin32();
  virtual ~CWinSystemWin32();

  // CWinSystemBase overrides
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void FinishWindowResize(int newWidth, int newHeight) override;
  void ForceFullScreen(const RESOLUTION_INFO& resInfo) override;
  void UpdateResolutions() override;
  bool CenterWindow() override;
  virtual void NotifyAppFocusChange(bool bGaining) override;
  void ShowOSMouse(bool show) override;
  bool HasInertialGestures() override { return true; }//if win32 has touchscreen - it uses the win32 gesture api for inertial scrolling
  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  std::string GetClipboardText() override;
  bool UseLimitedColor() override;
  float GetGuiSdrPeakLuminance() const override;
  bool HasSystemSdrPeakLuminance() override;

  // videosync
  std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  std::vector<std::string> GetConnectedOutputs() override;

  // CWinSystemWin32
  HWND GetHwnd() const { return m_hWnd; }
  bool IsAlteringWindow() const { return m_IsAlteringWindow; }
  void SetAlteringWindow(bool altering) { m_IsAlteringWindow = altering; }
  bool IsTogglingHDR() const { return m_IsTogglingHDR; }
  void SetTogglingHDR(bool toggling);
  virtual bool DPIChanged(WORD dpi, RECT windowRect) const;
  bool IsMinimized() const { return m_bMinimized; }
  void SetMinimized(bool minimized);
  void CacheSystemSdrPeakLuminance();

  void SetSizeMoveMode(bool mode) { m_bSizeMoveEnabled = mode; }
  bool IsInSizeMoveMode() const { return m_bSizeMoveEnabled; }

  // winevents override
  bool MessagePump() override;

protected:
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override = 0;
  virtual void UpdateStates(bool fullScreen);
  WINDOW_STATE GetState(bool fullScreen) const;
  virtual void SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res) = 0;
  virtual void ReleaseBackBuffer() = 0;
  virtual void CreateBackBuffer() = 0;
  virtual void ResizeDeviceBuffers() = 0;
  virtual bool IsStereoEnabled() = 0;
  virtual void OnScreenChange(HMONITOR monitor) = 0;
  virtual void AdjustWindow(bool forceResize = false);
  void CenterCursor() const;

  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

  virtual bool ChangeResolution(const RESOLUTION_INFO& res, bool forceChange = false);
  virtual bool CreateBlankWindows();
  virtual bool BlankNonActiveMonitors(bool bBlank);
  MONITOR_DETAILS* GetDisplayDetails(const std::string& name);
  MONITOR_DETAILS* GetDisplayDetails(HMONITOR handle);
  void RestoreDesktopResolution(MONITOR_DETAILS* details);
  RECT ScreenRect(HMONITOR handle);
  void GetConnectedDisplays(std::vector<MONITOR_DETAILS>& outputs);


  /*!
   \brief Adds a resolution to the list of resolutions if we don't already have it
   \param res resolution to add.
   */
  static bool AddResolution(const RESOLUTION_INFO &res);

  void OnDisplayLost();
  void OnDisplayReset();
  void OnDisplayBack();
  void ResolutionChanged();
  static void SetForegroundWindowInternal(HWND hWnd);
  static RECT GetVirtualScreenRect();
  /*!
   * Retrieve the work area of the screen (exclude task bar and other occlusions)
   */
  RECT GetScreenWorkArea(HMONITOR handle) const;
  /*!
   * Retrieve size of the title bar and borders
   * Add to coordinates to convert client coordinates to window coordinates
   * Substract from coordinates to convert from window coordinates to client coordinates
   */
  RECT GetNcAreaOffsets(DWORD dwStyle, BOOL bMenu, DWORD dwExStyle) const;

  HWND m_hWnd;
  HMONITOR m_hMonitor;

  std::vector<HWND> m_hBlankWindows;
  HINSTANCE m_hInstance;
  HICON m_hIcon;
  bool m_ValidWindowedPosition;
  bool m_IsAlteringWindow;
  bool m_IsTogglingHDR{false};

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  bool m_delayDispReset;
  XbmcThreads::EndTime<> m_dispResetTimer;

  WINDOW_STATE m_state;                       // the state of the window
  WINDOW_FULLSCREEN_STATE m_fullscreenState;  // the state of the window when in fullscreen
  WINDOW_WINDOW_STATE m_windowState;          // the state of the window when in windowed
  DWORD m_windowStyle;                        // the style of the window
  DWORD m_windowExStyle;                      // the ex style of the window
  bool m_inFocus;
  bool m_bMinimized;
  bool m_bSizeMoveEnabled = false;
  bool m_bFirstResChange = true;
  std::unique_ptr<CIRServerSuite> m_irss;
  std::vector<MONITOR_DETAILS> m_displays;

  NOTIFYICONDATA m_trayIcon = {};

  static const char* SETTING_WINDOW_TOP;
  static const char* SETTING_WINDOW_LEFT;

  bool m_validSystemSdrPeakLuminance{false};
  float m_systemSdrPeakLuminance{.0f};
};

extern HWND g_hWnd;

