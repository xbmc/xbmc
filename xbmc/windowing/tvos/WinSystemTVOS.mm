/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsTVOS.h"
#include "WinSystemTVOS.h"
#include "cores/AudioEngine/Sinks/AESinkDARWINIOS.h"
#include "cores/RetroPlayer/process/ios/RPProcessInfoIOS.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/DVDCodecs/Video/VTB.h"
#include "cores/VideoPlayer/Process/ios/ProcessInfoIOS.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVTBGLES.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/DisplaySettings.h"
#include "windowing/GraphicContext.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/Texture.h"
#include "utils/StringUtils.h"
#include "guilib/DispResource.h"
#include "threads/SingleLock.h"
#include "VideoSyncTVos.h"
#include <vector>

#import <Foundation/Foundation.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/CADisplayLink.h>

#import "platform/darwin/ios/XBMCController.h"
//#import "platform/darwin/ios/IOSScreenManager.h"
#include "platform/darwin/DarwinUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#import <dlfcn.h>

#define CONST_TOUCHSCREEN "Touchscreen"
#define CONST_EXTERNAL "External"

// if there was a devicelost callback
// but no device reset for 3 secs
// a timeout fires the reset callback
// (for ensuring that e.x. AE isn't stuck)
constexpr uint32_t LOST_DEVICE_TIMEOUT_MS{3000};

// IOSDisplayLinkCallback is declared in the lower part of the file
@interface IOSDisplayLinkCallback : NSObject
{
@private CVideoSyncTVos* _videoSyncImpl;
}
@property (nonatomic, setter=SetVideoSyncImpl:) CVideoSyncTVos* _videoSyncImpl;
- (void) runDisplayLink;
@end

using namespace KODI;
using namespace MESSAGING;

struct CADisplayLinkWrapper
{
  CADisplayLink* impl;
  IOSDisplayLinkCallback* callbackClass;
};


std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemTVOS());
  return winSystem;
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
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    CLog::Log(LOGDEBUG, "CWinSystemTVOS::AnnounceOnLostDevice");
    for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
        (*i)->OnLostDisplay();
}

void CWinSystemTVOS::AnnounceOnResetDevice()
{
    CSingleLock lock(m_resourceSection);
    // tell any shared resources
    CLog::Log(LOGDEBUG, "CWinSystemTVOS::AnnounceOnResetDevice");
    for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
        (*i)->OnResetDisplay();
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
  std::string currentScreen = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);

  int screenIdx = 0;
  if (currentScreen == CONST_EXTERNAL)
  {
    if ([[UIScreen screens] count] > 1)
    {
      screenIdx = 1;
    }
    else// screen 1 is setup but not connected
    {
      // force internal screen
      MoveToTouchscreen();
    }
  }

  return screenIdx;
}

CWinSystemTVOS::CWinSystemTVOS() : CWinSystemBase(), m_lostDeviceTimer(this)
{
  m_bIsBackgrounded = false;
  m_pDisplayLink = new CADisplayLinkWrapper;
  m_pDisplayLink->callbackClass = [[IOSDisplayLinkCallback alloc] init];

  m_winEvents.reset(new CWinEventsTVOS());

  CAESinkDARWINIOS::Register();
}

CWinSystemTVOS::~CWinSystemTVOS()
{
  [m_pDisplayLink->callbackClass release];
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

bool CWinSystemTVOS::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  
  if(!SetFullScreen(fullScreen, res, false))
    return false;
  
  
  [g_xbmcController setFramebuffer];

  m_bWindowCreated = true;

  m_eglext  = " ";

  const char* tmpExtensions = (const char*) glGetString(GL_EXTENSIONS);
  if (tmpExtensions != NULL)
  {
    m_eglext += tmpExtensions;
  }

  m_eglext += " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:%s", m_eglext.c_str());

  // register platform dependent objects
  CDVDFactoryCodec::ClearHWAccels();
  VTB::CDecoder::Register();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CLinuxRendererGLES::Register();
  CRendererVTB::Register();
  VIDEOPLAYER::CProcessInfoIOS::Register();
  RETRO::CRPProcessInfoIOS::Register();
  RETRO::CRPProcessInfoIOS::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);

  return true;
}

bool CWinSystemTVOS::DestroyWindow()
{
  return true;
}

bool CWinSystemTVOS::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  if (m_nWidth != newWidth || m_nHeight != newHeight)
  {
    m_nWidth  = newWidth;
    m_nHeight = newHeight;
  }

  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);

  return true;
}


bool CWinSystemTVOS::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  CLog::Log(LOGDEBUG, "About to switch to %i x %i",m_nWidth, m_nHeight);
  SwitchToVideoMode(res.iWidth, res.iHeight, res.fRefreshRate);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);

  return true;
}



UIScreenMode* getModeForResolution(int width, int height, unsigned int screenIdx)
{
  UIScreen* aScreen = [[UIScreen screens]objectAtIndex:screenIdx];
  
  // availableModes not avaible on tvOS
  UIScreenMode* mode = [aScreen currentMode];
  return mode;
}



bool CWinSystemTVOS::SwitchToVideoMode(int width, int height, double refreshrate)
{
  bool ret = false;
  int screenIdx = GetDisplayIndexFromSettings();

  //get the mode to pass to the controller
  UIScreenMode* newMode = getModeForResolution(width, height, screenIdx);

  if(newMode)
  {
    ret = [g_xbmcController changeScreen:screenIdx withMode:newMode];
  }
  return ret;
}



bool CWinSystemTVOS::GetScreenResolution(int* w, int* h, double* fps, int screenIdx)
{
  UIScreen* screen = [[UIScreen screens] objectAtIndex:screenIdx];
  CGSize screenSize = [screen currentMode].size;
  *w = screenSize.width;
  *h = screenSize.height;
  *fps = 0.0;

  //if current mode is 0x0 (happens with external screens which aren't active)
  //then use the preferred mode
  if(*h == 0 || *w ==0)
  {
    // preferredMode not avaible on tvOS
    UIScreenMode* firstMode = [screen currentMode];
    //UIScreenMode* firstMode = [screen preferredMode];
    *w = firstMode.size.width;
    *h = firstMode.size.height;
  }
  
  // for mainscreen use the eagl bounds from xbmcController
  // because mainscreen is might be 90° rotate dependend on
  // the device and eagl gives the correct values in all cases.
  if(screenIdx == 0)
  {
    // at very first start up we cache the internal screen resolution
    // because when using external screens and need to go back
    // to internal we are not able to determine the eagl bounds
    // before we really switched back to internal
    // but display settings ask for the internal resolution before
    // switching. So we give the cached values back in that case.
    if (m_internalTouchscreenResolutionWidth == -1 &&
        m_internalTouchscreenResolutionHeight == -1)
    {
      m_internalTouchscreenResolutionWidth = [g_xbmcController getScreenSize].width;
      m_internalTouchscreenResolutionHeight = [g_xbmcController getScreenSize].height;
    }
    
    *w = m_internalTouchscreenResolutionWidth;
    *h = m_internalTouchscreenResolutionHeight;
  }
  CLog::Log(LOGDEBUG,"Current resolution Screen: %i with %i x %i",screenIdx, *w, *h);
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
  if(GetScreenResolution(&w, &h, &fps, screenIdx))
    UpdateDesktopResolution(CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP), screenIdx == 0 ? CONST_TOUCHSCREEN : CONST_EXTERNAL, w, h, fps, 0);

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  //now just fill in the possible resolutions for the attached screens
  //and push to the resolution info vector
  FillInVideoModes(screenIdx);
}



void CWinSystemTVOS::FillInVideoModes(int screenIdx)
{
  // Add full screen settings for additional monitors
  RESOLUTION_INFO res;
  int w, h;
  // atm we don't get refreshrate info from iOS
  // but this may change in the future. In that case
  // we will adapt this code for filling some
  // useful info into this local var :)
  double refreshrate = 0.0;
  //screen 0 is mainscreen - 1 has to be the external one...
  UIScreen* aScreen = [[UIScreen screens]objectAtIndex:screenIdx];
  UIScreenMode* mode = [aScreen currentMode];
  w = mode.size.width;
  h = mode.size.height;
  
  UpdateDesktopResolution(res, screenIdx == 0 ? CONST_TOUCHSCREEN : CONST_EXTERNAL, w, h, refreshrate, 0);
  CLog::Log(LOGNOTICE, "Found possible resolution for display %d with %d x %d\n", screenIdx, w, h);
  
  CServiceBroker::GetWinSystem()->GetGfxContext().ResetOverscan(res);
  CDisplaySettings::GetInstance().AddResolutionInfo(res);
  
}




bool CWinSystemTVOS::IsExtSupported(const char* extension) const
{
  if(strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGLES::IsExtSupported(extension);

  std::string name;

  name  = " ";
  name += extension;
  name += " ";

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
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CWinSystemTVOS::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<IDispResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void CWinSystemTVOS::OnAppFocusChange(bool focus)
{
  CSingleLock lock(m_resourceSection);
  m_bIsBackgrounded = !focus;
  CLog::Log(LOGDEBUG, "CWinSystemTVOS::OnAppFocusChange: %d", focus ? 1 : 0);
  for (std::vector<IDispResource*>::iterator i = m_resources.begin(); i != m_resources.end(); i++)
    (*i)->OnAppFocusChange(focus);
}

//--------------------------------------------------------------
//-------------------DisplayLink stuff
@implementation IOSDisplayLinkCallback
@synthesize _videoSyncImpl;
//--------------------------------------------------------------
- (void) runDisplayLink
{
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  if (_videoSyncImpl != nil)
  {
    _videoSyncImpl->TVosVblankHandler();
  }
  [pool release];
}
@end

bool CWinSystemTVOS::InitDisplayLink(CVideoSyncTVos* syncImpl)
{
  unsigned int currentScreenIdx = GetDisplayIndexFromSettings();
  UIScreen* currentScreen = [[UIScreen screens] objectAtIndex:currentScreenIdx];
  [m_pDisplayLink->callbackClass SetVideoSyncImpl:syncImpl];
  m_pDisplayLink->impl = [currentScreen displayLinkWithTarget:m_pDisplayLink->callbackClass selector:@selector(runDisplayLink)];

  [m_pDisplayLink->impl setFrameInterval:1];
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
  // apple touch devices
  return false;
}

void CWinSystemTVOS::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
    CApplicationMessenger::GetInstance().PostMsg(TMSG_TOGGLEFULLSCREEN);
}

bool CWinSystemTVOS::Minimize()
{
  m_bWasFullScreenBeforeMinimize = CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    CApplicationMessenger::GetInstance().PostMsg(TMSG_TOGGLEFULLSCREEN);

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

void* CWinSystemTVOS::GetEAGLContextObj()
{
  return [g_xbmcController getEAGLContextObj];
}

void CWinSystemTVOS::GetConnectedOutputs(std::vector<std::string>* outputs)
{
  outputs->push_back("Default");
  outputs->push_back(CONST_TOUCHSCREEN);
  if ([[UIScreen screens] count] > 1)
  {
    outputs->push_back(CONST_EXTERNAL);
  }
}

void CWinSystemTVOS::MoveToTouchscreen()
{
  CDisplaySettings::GetInstance().SetMonitor(CONST_TOUCHSCREEN);
}

/*
std::unique_ptr<CVideoSync> CWinSystemTVOS::GetVideoSync(void* clock)
{
  std::unique_ptr<CVideoSync> pVSync(new CVideoSyncIos(clock, *this));
  return pVSync;
}
 */

bool CWinSystemTVOS::MessagePump()
{
  return m_winEvents->MessagePump();
}
