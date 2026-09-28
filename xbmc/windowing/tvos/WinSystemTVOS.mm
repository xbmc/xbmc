/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemTVOS.h"

#include "ServiceBroker.h"
#import "cores/AudioEngine/Sinks/AESinkDARWINTVOS.h"
#include "cores/RetroPlayer/process/ios/RPProcessInfoIOS.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VTB.h"
#include "cores/VideoPlayer/Process/ios/ProcessInfoIOS.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVTBGLES.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVTBDisplayLayer.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/DispResource.h"
#include "guilib/Texture.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/gles/ScreenshotSurfaceGLES.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/OSScreenSaver.h"
#include "windowing/WindowSystemFactory.h"
#import "windowing/tvos/OSScreenSaverTVOS.h"
#import "windowing/tvos/VideoSyncTVos.h"
#import "windowing/tvos/WinEventsTVOS.h"

#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/tvos/TVOSDisplayManager.h"
#import "platform/darwin/tvos/XBMCController.h"

#include <memory>
#include <mutex>
#include <vector>

#import <Foundation/Foundation.h>
#import <CoreMedia/CMFormatDescription.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/CADisplayLink.h>

using namespace std::chrono_literals;

#define CONST_HDMI "HDMI"

namespace
{
constexpr int TVOS_DYNAMIC_RANGE_SDR = 0;
constexpr int TVOS_DYNAMIC_RANGE_HDR10 = 2;
constexpr int TVOS_DYNAMIC_RANGE_HLG = 3;
} // namespace

// if there was a devicelost callback
// but no device reset for 3 secs
// a timeout fires the reset callback
// (for ensuring that e.x. AE isn't stuck)
constexpr auto LOST_DEVICE_TIMEOUT_MS{3000ms};

// TVOSDisplayLinkCallback is defined in the lower part of the file
@interface TVOSDisplayLinkCallback : NSObject
{
@private
  CVideoSyncTVos* videoSyncImpl;
}
@property(nonatomic, setter=SetVideoSyncImpl:) CVideoSyncTVos* videoSyncImpl;
- (void)runDisplayLink;
@end

using namespace KODI;
using namespace MESSAGING;

struct CADisplayLinkWrapper
{
  CADisplayLink* impl;
  TVOSDisplayLinkCallback* callbackClass;
};

void CWinSystemTVOS::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemTVOS::CreateWinSystem()
{
  return std::make_unique<CWinSystemTVOS>();
}

void CWinSystemTVOS::MessagePush(XBMC_Event* newEvent)
{
  dynamic_cast<CWinEventsTVOS&>(*m_winEvents).MessagePush(newEvent);
}

size_t CWinSystemTVOS::GetQueueSize()
{
  return dynamic_cast<CWinEventsTVOS&>(*m_winEvents).GetQueueSize();
}

void CWinSystemTVOS::AnnounceOnLostDevice()
{
  std::unique_lock lock(m_resourceSection);
  // tell any shared resources
  CLog::Log(LOGDEBUG, "CWinSystemTVOS::AnnounceOnLostDevice");
  for (auto dispResource : m_resources)
    dispResource->OnLostDisplay();
}

void CWinSystemTVOS::AnnounceOnResetDevice()
{
  std::unique_lock lock(m_resourceSection);
  // tell any shared resources
  CLog::Log(LOGDEBUG, "CWinSystemTVOS::AnnounceOnResetDevice");
  for (auto dispResource : m_resources)
    dispResource->OnResetDisplay();
}

void CWinSystemTVOS::StartLostDeviceTimer()
{
  if (m_lostDeviceTimer.IsRunning())
    m_lostDeviceTimer.Restart();
  else
    m_lostDeviceTimer.Start(LOST_DEVICE_TIMEOUT_MS, false);
}

void CWinSystemTVOS::StopLostDeviceTimer()
{
  m_lostDeviceTimer.Stop();
}


int CWinSystemTVOS::GetDisplayIndexFromSettings()
{
  // ATV only supports 1 screen with id = 0
  return 0;
}

CWinSystemTVOS::CWinSystemTVOS() : CWinSystemBase(), m_lostDeviceTimer(this)
{
  m_bIsBackgrounded = false;
  m_pDisplayLink = new CADisplayLinkWrapper;
  m_pDisplayLink->callbackClass = [[TVOSDisplayLinkCallback alloc] init];

  m_winEvents = std::make_unique<CWinEventsTVOS>();

  CAESinkDARWINTVOS::Register();
}

CWinSystemTVOS::~CWinSystemTVOS()
{
  if (m_hdrFormatDescription)
    CFRelease(m_hdrFormatDescription);
  m_pDisplayLink->callbackClass = nil;
  delete m_pDisplayLink;
}

bool CWinSystemTVOS::InitWindowSystem()
{
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemTVOS::DestroyWindowSystem()
{
  return true;
}

std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> CWinSystemTVOS::GetOSScreenSaverImpl()
{
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> screensaver =
      std::make_unique<COSScreenSaverTVOS>();
  return screensaver;
}

bool CWinSystemTVOS::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  if (!SetFullScreen(fullScreen, res, false))
    return false;

  [g_xbmcController setFramebuffer];

  m_bWindowCreated = true;

  m_eglext = " ";

  const char* tmpExtensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  if (tmpExtensions != nullptr)
  {
    m_eglext += tmpExtensions;
    m_eglext += " ";
  }

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS: {}", m_eglext);

  // register platform dependent objects
  CDVDFactoryCodec::ClearHWAccels();
  VTB::CDecoder::Register();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CLinuxRendererGLES::Register();
  CRendererVTBDisplayLayer::Register();
  CRendererVTB::Register();
  VIDEOPLAYER::CProcessInfoIOS::Register();
  RETRO::CRPProcessInfoIOS::Register();
  RETRO::CRPProcessInfoIOS::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);
  CScreenshotSurfaceGLES::Register();

  return true;
}

bool CWinSystemTVOS::DestroyWindow()
{
  return true;
}

bool CWinSystemTVOS::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  if (m_nWidth != newWidth || m_nHeight != newHeight)
  {
    m_nWidth = newWidth;
    m_nHeight = newHeight;
  }

  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);

  return true;
}

bool CWinSystemTVOS::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  m_nWidth = res.iWidth;
  m_nHeight = res.iHeight;
  m_bFullScreen = fullScreen;

  CLog::Log(LOGDEBUG, "About to switch to {} x {} @ {}", m_nWidth, m_nHeight, res.fRefreshRate);
  SwitchToVideoMode(res.iWidth, res.iHeight, static_cast<double>(res.fRefreshRate));
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);

  return true;
}

bool CWinSystemTVOS::SwitchToVideoMode(int width, int height, double refreshrate)
{
  m_requestedRefreshRate = static_cast<float>(refreshrate);
  if (m_hdrStatus == HDR_STATUS::HDR_ON && m_hdrFormatDescription)
  {
    if ([g_xbmcController.displayManager displayVideoFormatSwitch:m_hdrFormatDescription
                                                       refreshRate:m_requestedRefreshRate])
      return true;
    SetHDR(nullptr);
  }

  [g_xbmcController.displayManager displayRateSwitch:refreshrate
                                    withDynamicRange:TVOS_DYNAMIC_RANGE_SDR];
  return true;
}

int CWinSystemTVOS::GetDynamicRangeForHDR(const VideoPicture* videoPicture) const
{
  if (!videoPicture)
    return TVOS_DYNAMIC_RANGE_SDR;

  const CHDRCapabilities caps = GetDisplayHDRCapabilities();

  // A display may accept HLG sample buffers even when AVPlayer's deprecated
  // HLG mode bit is clear. Let AVDisplayCriteria select the actual HDMI mode.
  if (videoPicture->hdrType == StreamHdrType::HDR_TYPE_HLG &&
      (caps.SupportsHLG() || caps.SupportsHDR10()))
    return TVOS_DYNAMIC_RANGE_HLG;

  if (videoPicture->hdrType == StreamHdrType::HDR_TYPE_HDR10 && caps.SupportsHDR10())
  {
    return TVOS_DYNAMIC_RANGE_HDR10;
  }

  return TVOS_DYNAMIC_RANGE_SDR;
}

bool CWinSystemTVOS::SetHDR(const VideoPicture* videoPicture)
{
  const int dynamicRange = CanUseHDRVideoLayer() ? GetDynamicRangeForHDR(videoPicture)
                                                  : TVOS_DYNAMIC_RANGE_SDR;
  auto* buffer = videoPicture ? dynamic_cast<VTB::CVideoBufferVTB*>(videoPicture->videoBuffer)
                              : nullptr;
  CVPixelBufferRef pixelBuffer = buffer ? buffer->GetPB() : nullptr;
  if (dynamicRange == TVOS_DYNAMIC_RANGE_SDR || !pixelBuffer ||
      CVPixelBufferGetPixelFormatType(pixelBuffer) != kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange)
  {
    if (m_hdrFormatDescription)
    {
      CFRelease(m_hdrFormatDescription);
      m_hdrFormatDescription = nullptr;
    }
    if (m_hdrStatus == HDR_STATUS::HDR_ON)
      [g_xbmcController.displayManager displayDynamicRangeReset];
    m_hdrStatus = HDR_STATUS::HDR_OFF;
    m_requestedRefreshRate = 0.0f;
    return false;
  }

  CMVideoFormatDescriptionRef formatDescription = nullptr;
  if (CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, pixelBuffer,
                                                  &formatDescription) != noErr)
  {
    SetHDR(nullptr);
    return false;
  }

  const float rate = m_requestedRefreshRate > 0.0f ? m_requestedRefreshRate
                                                    : [g_xbmcController.displayManager getDisplayRate];
  const bool accepted = [g_xbmcController.displayManager displayVideoFormatSwitch:formatDescription
                                                                      refreshRate:rate];
  if (accepted)
  {
    if (m_hdrFormatDescription)
      CFRelease(m_hdrFormatDescription);
    m_hdrFormatDescription = formatDescription;
    m_hdrStatus = HDR_STATUS::HDR_ON;
    CLog::Log(LOGDEBUG, "CWinSystemTVOS::SetHDR: requesting {}",
              dynamicRange == TVOS_DYNAMIC_RANGE_HLG ? "HLG" : "HDR10");
  }
  else
  {
    CFRelease(formatDescription);
    SetHDR(nullptr);
  }
  return accepted;
}

bool CWinSystemTVOS::CanUseHDRVideoLayer()
{
#if __TV_OS_VERSION_MAX_ALLOWED >= 170000
  if (@available(tvOS 17.0, *))
  {
    const bool hdrEnabled = IsHDRDisplaySettingEnabled();
    const bool matchingEnabled = [g_xbmcController.displayManager canMatchVideoDynamicRange];
    CLog::Log(LOGDEBUG, "CWinSystemTVOS::CanUseHDRVideoLayer: HDR setting {}, display matching {}",
              hdrEnabled, matchingEnabled);
    return hdrEnabled && matchingEnabled;
  }
#endif
  return false;
}

bool CWinSystemTVOS::IsHDRDisplay()
{
  const CHDRCapabilities caps = GetDisplayHDRCapabilities();
  return caps.SupportsHDR10() || caps.SupportsHLG();
}

CHDRCapabilities CWinSystemTVOS::GetDisplayHDRCapabilities() const
{
  CHDRCapabilities caps;

  if ([g_xbmcController.displayManager supportsHDR])
    caps.SetHDR10();

  if ([g_xbmcController.displayManager supportsHLG])
    caps.SetHLG();

  return caps;
}

bool CWinSystemTVOS::GetScreenResolution(int* w, int* h, double* fps, int screenIdx)
{
  *w = [g_xbmcController.displayManager getScreenSize].width;
  *h = [g_xbmcController.displayManager getScreenSize].height;
  *fps = static_cast<double>([g_xbmcController.displayManager getDisplayRate]);

  CLog::Log(LOGDEBUG, "Current resolution Screen: {} with {} x {} @ {}", screenIdx, *w, *h, *fps);
  return true;
}

void CWinSystemTVOS::UpdateResolutions()
{
  // Add display resolution
  int w, h;
  double fps;
  CWinSystemBase::UpdateResolutions();

  int screenIdx = GetDisplayIndexFromSettings();

  //first screen goes into the current desktop mode
  if (GetScreenResolution(&w, &h, &fps, screenIdx))
    UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP),
                            CONST_HDMI, w, h, fps, 0);

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  //now just fill in the possible resolutions for the attached screens
  //and push to the resolution info vector
  FillInVideoModes(screenIdx);
}

void CWinSystemTVOS::FillInVideoModes(int screenIdx)
{
  // Potential refresh rates
  std::vector<float> supportedDispRefreshRates = {23.976f, 24.000f, 25.000f, 29.970f,
                                                  30.000f, 50.000f, 59.940f, 60.000f};

  UIScreen* aScreen = UIScreen.screens[screenIdx];
  UIScreenMode* mode = aScreen.currentMode;
  int w = mode.size.width;
  int h = mode.size.height;

  //! @Todo handle different resolutions than native (ie 720p/1080p on a 4k display)

  for (float refreshrate : supportedDispRefreshRates)
  {
    RESOLUTION_INFO res;
    UpdateDesktopResolution(res, CONST_HDMI, w, h, refreshrate, 0);
    CLog::Log(LOGINFO, "Found possible resolution for display {} with {} x {} RefreshRate:{} ",
              screenIdx, w, h, refreshrate);

    CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
    CDisplaySettings::GetInstance().AddResolutionInfo(res);
  }
}

bool CWinSystemTVOS::IsExtSupported(const char* extension) const
{
  if (strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGLES::IsExtSupported(extension);

  std::string name = ' ' + std::string(extension) + ' ';

  return m_eglext.find(name) != std::string::npos;
}


bool CWinSystemTVOS::BeginRender()
{
  bool rtn;

  [g_xbmcController setFramebuffer];

  rtn = CRenderSystemGLES::BeginRender();
  return rtn;
}

bool CWinSystemTVOS::EndRender()
{
  bool rtn;

  rtn = CRenderSystemGLES::EndRender();
  return rtn;
}

void CWinSystemTVOS::Register(IDispResource* resource)
{
  std::unique_lock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemTVOS::Unregister(IDispResource* resource)
{
  std::unique_lock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemTVOS::OnAppFocusChange(bool focus)
{
  std::unique_lock lock(m_resourceSection);
  m_bIsBackgrounded = !focus;
  CLog::Log(LOGDEBUG, "CWinSystemTVOS::OnAppFocusChange: {}", focus ? 1 : 0);
  for (auto dispResource : m_resources)
    dispResource->OnAppFocusChange(focus);
}

//--------------------------------------------------------------
//-------------------DisplayLink stuff
@implementation TVOSDisplayLinkCallback
@synthesize videoSyncImpl;
//--------------------------------------------------------------
- (void)runDisplayLink
{
  @autoreleasepool
  {
    if (videoSyncImpl != nullptr)
      videoSyncImpl->TVosVblankHandler();
  }
}
@end

bool CWinSystemTVOS::InitDisplayLink(CVideoSyncTVos* syncImpl)
{
  unsigned int currentScreenIdx = GetDisplayIndexFromSettings();
  UIScreen* currentScreen = UIScreen.screens[currentScreenIdx];
  m_pDisplayLink->callbackClass.videoSyncImpl = syncImpl;
  m_pDisplayLink->impl = [currentScreen displayLinkWithTarget:m_pDisplayLink->callbackClass
                                                     selector:@selector(runDisplayLink)];

  [m_pDisplayLink->impl addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  return m_pDisplayLink->impl != nil;
}

void CWinSystemTVOS::DeinitDisplayLink(void)
{
  if (m_pDisplayLink->impl)
  {
    [m_pDisplayLink->impl invalidate];
    m_pDisplayLink->impl = nil;
    [m_pDisplayLink->callbackClass SetVideoSyncImpl:nil];
  }
}
//------------DisplayLink stuff end
//--------------------------------------------------------------

void CWinSystemTVOS::PresentRenderImpl(bool rendered)
{
  //glFlush;
  if (rendered)
    [g_xbmcController presentFramebuffer];
}

bool CWinSystemTVOS::HasCursor()
{
  return false;
}

void CWinSystemTVOS::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize &&
      !CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);
}

bool CWinSystemTVOS::Minimize()
{
  m_bWasFullScreenBeforeMinimize =
      CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_TOGGLEFULLSCREEN);

  return true;
}

bool CWinSystemTVOS::Restore()
{
  return false;
}

bool CWinSystemTVOS::Hide()
{
  return true;
}

bool CWinSystemTVOS::Show(bool raise)
{
  return true;
}

CVEAGLContext CWinSystemTVOS::GetEAGLContextObj()
{
  return [g_xbmcController getEAGLContextObj];
}

std::vector<std::string> CWinSystemTVOS::GetConnectedOutputs()
{
  std::vector<std::string> outputs;
  outputs.emplace_back(OUTPUT_NAME_DEFAULT);
  outputs.emplace_back(CONST_HDMI);

  return outputs;
}

bool CWinSystemTVOS::MessagePump()
{
  return m_winEvents->MessagePump();
}
