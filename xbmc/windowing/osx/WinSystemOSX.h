/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "threads/Timer.h"
#include "windowing/WinSystem.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

typedef struct _CGLContextObject* CGLContextObj;
typedef struct CGRect NSRect;
class IDispResource;
class CWinEventsOSX;
#ifdef __OBJC__
@class NSWindowController;
@class NSWindow;
@class OSXGLView;
@class NSEvent;
#else
struct NSWindow;
struct OSXGLView;
struct NSEvent;
struct NSWindowController;

#endif

class CWinSystemOSX : public CWinSystemBase, public ITimerCallback
{
public:
  CWinSystemOSX();
  ~CWinSystemOSX() override;

  struct ScreenResolution
  {
    bool interlaced{false};
    size_t resWidth{0};
    size_t resHeight{0};
    size_t pixelWidth{0};
    size_t pixelHeight{0};
    double refreshrate{0.0};
  };

  // ITimerCallback interface
  void OnTimeout() override;

  // CWinSystemBase
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool HasCursor() override;
  bool Show(bool raise = true) override;
  unsigned int GetScreenId(const std::string& screen) override;
  void MoveToScreen(unsigned int screenIdx) override;
  void OnMove(int x, int y) override;
  void OnChangeScreen(unsigned int screenIdx) override;
  bool HasValidResolution() const;

  std::string GetClipboardText() override;

  void Register(IDispResource* resource) override;
  void Unregister(IDispResource* resource) override;

  std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  void WindowChangedScreen();

  void AnnounceOnLostDevice();
  void AnnounceOnResetDevice();
  void HandleOnResetDevice();
  void StartLostDeviceTimer();
  void StopLostDeviceTimer();

  int CheckDisplayChanging(uint32_t flags);
  void SetFullscreenWillToggle(bool toggle) { m_fullscreenWillToggle = toggle; }
  bool GetFullscreenWillToggle() { return m_fullscreenWillToggle; }
  void SignalFullScreenStateChanged(bool fullscreenState);

  CGLContextObj GetCGLContextObj();

  std::vector<std::string> GetConnectedOutputs() override;

  // winevents override
  bool MessagePump() override;

  NSRect GetWindowDimensions();
  void enableInputEvents();
  void disableInputEvents();

  void signalMouseEntered();
  void signalMouseExited();
  void SendInputEvent(NSEvent* nsEvent);

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;

  ScreenResolution GetScreenResolution(unsigned long screenIdx);
  void EnableVSync(bool enable);
  bool SwitchToVideoMode(RESOLUTION_INFO& res);
  void FillInVideoModes();
  bool FlushBuffer();

  bool DestroyWindowInternal();

  std::unique_ptr<CWinEventsOSX> m_winEvents;

  std::string m_name;
  NSWindow* m_appWindow;
  OSXGLView* m_glView;
  unsigned long m_lastDisplayNr;
  double m_refreshRate;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;
  CTimer m_lostDeviceTimer;
  bool m_delayDispReset;
  XbmcThreads::EndTime<> m_dispResetTimer;
  bool m_fullscreenWillToggle;
  bool m_hasCursor{false};
  //! Set while moving the fullscreen window to another screen. Stores the target screen id.
  std::optional<unsigned long> m_fullScreenMovingToScreen;
  CCriticalSection m_critSection;

private:
  NSWindowController* m_appWindowController;
};
