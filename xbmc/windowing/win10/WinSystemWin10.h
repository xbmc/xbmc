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

#include <vector>

#pragma pack(push)
#pragma pack(8)

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
  float     RefreshRate;
  int       Bpp;
  bool      Interlaced;
};

class CWinSystemWin10 : public CWinSystemBase
{
public:
  CWinSystemWin10();
  virtual ~CWinSystemWin10();

  // CWinSystemBase overrides
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void FinishWindowResize(int newWidth, int newHeight) override;
  void ForceFullScreen(const RESOLUTION_INFO& resInfo) override;
  void UpdateResolutions() override;
  void NotifyAppFocusChange(bool bGaining) override;
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

  bool WindowedMode() const { return m_state != WINDOW_STATE_FULLSCREEN; }
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  // CWinSystemWin10
  bool IsAlteringWindow() const { return m_IsAlteringWindow; }
  void SetAlteringWindow(bool altering) { m_IsAlteringWindow = altering; }
  bool IsTogglingHDR() const { return false; }
  void SetTogglingHDR(bool toggling) {}
  virtual bool DPIChanged(WORD dpi, RECT windowRect) const;
  bool IsMinimized() const { return m_bMinimized; }
  void SetMinimized(bool minimized) { m_bMinimized = minimized; }
  void CacheSystemSdrPeakLuminance();

  bool CanDoWindowed() override;

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
  virtual void AdjustWindow();

  virtual void Register(IDispResource *resource);
  virtual void Unregister(IDispResource *resource);

  bool ChangeResolution(const RESOLUTION_INFO& res, bool forceChange = false);
  const MONITOR_DETAILS* GetDefaultMonitor() const;
  void RestoreDesktopResolution();
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

  std::vector<MONITOR_DETAILS> m_displays;
  bool m_ValidWindowedPosition;
  bool m_IsAlteringWindow;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  bool m_delayDispReset;
  XbmcThreads::EndTime<> m_dispResetTimer;

  WINDOW_STATE m_state;                       // the state of the window
  WINDOW_FULLSCREEN_STATE m_fullscreenState;  // the state of the window when in fullscreen
  WINDOW_WINDOW_STATE m_windowState;          // the state of the window when in windowed
  bool m_inFocus;
  bool m_bMinimized;
  bool m_bFirstResChange = true;

  winrt::Windows::UI::Core::CoreWindow m_coreWindow = nullptr;

  bool m_validSystemSdrPeakLuminance{false};
  float m_systemSdrPeakLuminance{.0f};

  DWORD m_uiThreadId{0};
  HDR_STATUS m_cachedHdrStatus{HDR_STATUS::HDR_UNSUPPORTED};
  bool m_cachedHasSystemSdrPeakLum{false};
};

#pragma pack(pop)

