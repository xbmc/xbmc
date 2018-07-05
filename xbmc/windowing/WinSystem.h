/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include "OSScreenSaver.h"
#include "VideoSync.h"
#include "WinEvents.h"
#include "guilib/DispResource.h"
#include "Resolution.h"
#include <memory>
#include <vector>

struct RESOLUTION_WHR
{
  int width;
  int height;
  int flags; //< only D3DPRESENTFLAG_MODEMASK flags
  int ResInfo_Index;
};

struct REFRESHRATE
{
  float RefreshRate;
  int   ResInfo_Index;
};

class CGraphicContext;
class CRenderSystemBase;
class IRenderLoop;

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();

  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Access render system interface
  virtual CRenderSystemBase *GetRenderSystem() { return nullptr; }

  // windowing interfaces
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) = 0;
  virtual bool DestroyWindow(){ return false; }
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) = 0;
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) = 0;
  virtual bool MoveWindow(int topLeft, int topRight){return false;}
  virtual void FinishModeChange(RESOLUTION res){}
  virtual void FinishWindowResize(int newWidth, int newHeight) {ResizeWindow(newWidth, newHeight, -1, -1);}
  virtual bool CenterWindow(){return false;}
  virtual bool IsCreated(){ return m_bWindowCreated; }
  virtual void NotifyAppFocusChange(bool bGaining) {}
  virtual void NotifyAppActiveChange(bool bActivated) {}
  virtual void ShowOSMouse(bool show) {};
  virtual bool HasCursor(){ return true; }
  //some platforms have api for gesture inertial scrolling - default to false and use the InertialScrollingHandler
  virtual bool HasInertialGestures(){ return false; }
  //does the output expect limited color range (ie 16-235)
  virtual bool UseLimitedColor();
  //the number of presentation buffers
  virtual int NoOfBuffers();
  /**
   * Get average display latency
   *
   * The latency should be measured as the time between finishing the rendering
   * of a frame, i.e. calling PresentRender, and the rendered content becoming
   * visible on the screen.
   *
   * \return average display latency in seconds, or negative value if unknown
   */
  virtual float GetDisplayLatency() { return -1.0f; }
  /**
   * Get time that should be subtracted from the display latency for this frame
   * in milliseconds
   *
   * Contrary to \ref GetDisplayLatency, this value is calculated ad-hoc
   * for the frame currently being rendered and not a value that is calculated/
   * averaged from past frames and their presentation times
   */
  virtual float GetFrameLatencyAdjustment() { return 0.0; }

  virtual bool Minimize() { return false; }
  virtual bool Restore() { return false; }
  virtual bool Hide() { return false; }
  virtual bool Show(bool raise = true) { return false; }

  // videosync
  virtual std::unique_ptr<CVideoSync> GetVideoSync(void *clock) { return nullptr; }

  // notifications
  virtual void OnMove(int x, int y) {}

  // OS System screensaver
  /**
   * Get OS screen saver inhibit implementation if available
   *
   * \return OS screen saver implementation that can be used with this windowing system
   *         or nullptr if unsupported.
   *         Lifetime of the returned object will usually end with \ref DestroyWindowSystem, so
   *         do not use any more after calling that.
   */
  KODI::WINDOWING::COSScreenSaverManager* GetOSScreenSaver();

  // resolution interfaces
  unsigned int GetWidth() { return m_nWidth; }
  unsigned int GetHeight() { return m_nHeight; }
  virtual bool CanDoWindowed() { return true; }
  bool IsFullScreen() { return m_bFullScreen; }
  virtual void UpdateResolutions();
  void SetWindowResolution(int width, int height);
  std::vector<RESOLUTION_WHR> ScreenResolutions(float refreshrate);
  std::vector<REFRESHRATE> RefreshRates(int width, int height, uint32_t dwFlags);
  REFRESHRATE DefaultRefreshRate(std::vector<REFRESHRATE> rates);
  virtual bool HasCalibration(const RESOLUTION_INFO &resInfo) { return true; };

  // text input interface
  virtual std::string GetClipboardText(void);

  // Display event callback
  virtual void Register(IDispResource *resource) = 0;
  virtual void Unregister(IDispResource *resource) = 0;

  // render loop
  void RegisterRenderLoop(IRenderLoop *client);
  void UnregisterRenderLoop(IRenderLoop *client);
  void DriveRenderLoop();

  // winsystem events
  virtual bool MessagePump() { return false; }

  // Access render system interface
  CGraphicContext& GetGfxContext();

protected:
  void UpdateDesktopResolution(RESOLUTION_INFO& newRes, int width, int height, float refreshRate, uint32_t dwFlags);
  virtual std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() { return nullptr; }

  int m_nWidth = 0;
  int m_nHeight = 0;
  int m_nTop = 0;
  int m_nLeft = 0;
  bool m_bWindowCreated = false;
  bool m_bFullScreen = false;
  bool m_bBlankOtherDisplay = false;
  float m_fRefreshRate = 0.0f;
  std::unique_ptr<KODI::WINDOWING::COSScreenSaverManager> m_screenSaverManager;
  CCriticalSection m_renderLoopSection;
  std::vector<IRenderLoop*> m_renderLoopClients;

  std::unique_ptr<IWinEvents> m_winEvents;
  std::unique_ptr<CGraphicContext> m_gfxContext;
};
