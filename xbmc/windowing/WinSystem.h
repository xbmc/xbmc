/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "HDRStatus.h"
#include "OSScreenSaver.h"
#include "Resolution.h"
#include "VideoSync.h"
#include "WinEvents.h"
#include "cores/VideoPlayer/VideoRenderers/DebugInfo.h"
#include "guilib/DispResource.h"
#include "utils/HDRCapabilities.h"

#include <memory>
#include <string>
#include <vector>

struct RESOLUTION_WHR
{
  int width;
  int height;
  int m_screenWidth;
  int m_screenHeight;
  int flags; //< only D3DPRESENTFLAG_MODEMASK flags
  int ResInfo_Index;
  std::string id;
};

struct REFRESHRATE
{
  float RefreshRate;
  int   ResInfo_Index;
};

class CDPMSSupport;
class CGraphicContext;
class CRenderSystemBase;
class IRenderLoop;
class CVideoReferenceClock;

struct VideoPicture;

class CWinSystemBase
{
public:
  CWinSystemBase();
  virtual ~CWinSystemBase();

  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Access render system interface
  virtual CRenderSystemBase *GetRenderSystem() { return nullptr; }

  virtual const std::string GetName() { return "platform default"; }

  // windowing interfaces
  virtual bool InitWindowSystem();
  virtual bool DestroyWindowSystem();
  virtual bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) = 0;
  virtual bool DestroyWindow(){ return false; }
  virtual bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) = 0;
  virtual bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) = 0;
  virtual bool DisplayHardwareScalingEnabled() { return false; }
  virtual void UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo) { }
  virtual bool MoveWindow(int topLeft, int topRight){return false;}
  virtual void FinishModeChange(RESOLUTION res){}
  virtual void FinishWindowResize(int newWidth, int newHeight) {ResizeWindow(newWidth, newHeight, -1, -1);}
  virtual bool CenterWindow(){return false;}
  virtual bool IsCreated(){ return m_bWindowCreated; }
  virtual void NotifyAppFocusChange(bool bGaining) {}
  virtual void NotifyAppActiveChange(bool bActivated) {}
  virtual void ShowOSMouse(bool show) {}
  virtual bool HasCursor(){ return true; }
  //some platforms have api for gesture inertial scrolling - default to false and use the InertialScrollingHandler
  virtual bool HasInertialGestures(){ return false; }
  //does the output expect limited color range (ie 16-235)
  virtual bool UseLimitedColor();
  //the number of presentation buffers
  virtual int NoOfBuffers();

  /*!
   * \brief Forces the window to fullscreen provided the window resolution
   * \param resInfo - the resolution info
   */
  virtual void ForceFullScreen(const RESOLUTION_INFO& resInfo) {}

  /*!
   * \brief Get average display latency
   *
   * The latency should be measured as the time between finishing the rendering
   * of a frame, i.e. calling PresentRender, and the rendered content becoming
   * visible on the screen.
   *
   * \return average display latency in seconds, or negative value if unknown
   */
  virtual float GetDisplayLatency() { return -1.0f; }

  /*!
   * \brief Get time that should be subtracted from the display latency for this frame
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
  virtual std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) { return nullptr; }

  // notifications
  virtual void OnMove(int x, int y) {}

  /*!
   * \brief Get the screen ID provided the screen name
   *
   * \param screen the name of the screen as presented on the application display settings
   * \return the screen index as known by the windowing system implementation (or the default screen by default)
   */
  virtual unsigned int GetScreenId(const std::string& screen) { return 0; }

  /*!
   * \brief Window was requested to move to the given screen
   *
   * \param screenIdx the screen index as known by the windowing system implementation
   */
  virtual void MoveToScreen(unsigned int screenIdx) {}

  /*!
   * \brief Used to signal the windowing system about the change of the current screen
   *
   * \param screenIdx the screen index as known by the windowing system implementation
   */
  virtual void OnChangeScreen(unsigned int screenIdx) {}

  // OS System screensaver
  /*!
   * \brief Get OS screen saver inhibit implementation if available
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

  /*!
   * \brief Check if the windowing system supports moving windows across screens
   *
   * \return true if the windowing system supports moving windows across screens, false otherwise
   */
  virtual bool SupportsScreenMove() { return true; }

  virtual void UpdateResolutions();
  void SetWindowResolution(int width, int height);
  std::vector<RESOLUTION_WHR> ScreenResolutions(float refreshrate);
  std::vector<REFRESHRATE> RefreshRates(int width, int height, uint32_t dwFlags);
  REFRESHRATE DefaultRefreshRate(const std::vector<REFRESHRATE>& rates);
  virtual bool HasCalibration(const RESOLUTION_INFO& resInfo) { return true; }

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
  virtual CGraphicContext& GetGfxContext() const;

  /*!
   * \brief Get OS specific hardware context
   *
   * \return OS specific context or nullptr if OS not have
   *
   * \note This function is currently only related to Windows with DirectX,
   * all other OS where use GL returns nullptr.
   * Returned Windows class pointer is ID3D11DeviceContext1.
   */
  virtual void* GetHWContext() { return nullptr; }

  std::shared_ptr<CDPMSSupport> GetDPMSManager();

  /*!
   * \brief Set the HDR metadata. Passing nullptr as the parameter should
   * disable HDR.
   *
   */
  virtual bool SetHDR(const VideoPicture* videoPicture) { return false; }
  virtual bool IsHDRDisplay() { return false; }
  virtual HDR_STATUS ToggleHDR() { return HDR_STATUS::HDR_UNSUPPORTED; }
  virtual HDR_STATUS GetOSHDRStatus() { return HDR_STATUS::HDR_UNSUPPORTED; }
  virtual CHDRCapabilities GetDisplayHDRCapabilities() const { return {}; }
  static const char* SETTING_WINSYSTEM_IS_HDR_DISPLAY;
  virtual float GetGuiSdrPeakLuminance() const { return .0f; }
  virtual bool HasSystemSdrPeakLuminance() { return false; }

  /*!
   * \brief System supports Video Super Resolution HW upscaler i.e.:
   * "NVIDIA RTX Video Super Resolution" or "Intel Video Super Resolution"
   *
   */
  virtual bool SupportsVideoSuperResolution() { return false; }

  /*!
   * \brief Gets debug info from video renderer for use in "Debug Info OSD" (Alt + O)
   *
   */
  virtual DEBUG_INFO_RENDER GetDebugInfo() { return {}; }

  virtual std::vector<std::string> GetConnectedOutputs() { return {}; }

  /*!
   * \brief Return true when HDR display is available and enabled in settings
   *
   */
  bool IsHDRDisplaySettingEnabled();

  /*!
   * \brief Return true when "Video Super Resolution" is supported and enabled in settings
   *
   */
  bool IsVideoSuperResolutionSettingEnabled();

  /*!
   * \brief Return true when setting "High Precision Processing" is enabled
   *
   */
  bool IsHighPrecisionProcessingSettingEnabled();

  /*!
   * \brief Get dither settings
   *
   * \return std::pair containing dither enabled (bool) and dither depth (int)
   */
  std::pair<bool, int> GetDitherSettings();

protected:
  void UpdateDesktopResolution(RESOLUTION_INFO& newRes, const std::string &output, int width, int height, float refreshRate, uint32_t dwFlags);
  void UpdateDesktopResolution(RESOLUTION_INFO& newRes,
                               const std::string& output,
                               int width,
                               int height,
                               int screenWidth,
                               int screenHeight,
                               float refreshRate,
                               uint32_t dwFlags);
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
  std::shared_ptr<CDPMSSupport> m_dpms;
};
