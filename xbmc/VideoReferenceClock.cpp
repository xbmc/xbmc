/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "system.h"
#include <list>
#include "StdString.h"
#include "VideoReferenceClock.h"
#include "MathUtils.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/SingleLock.h"

#if defined(HAS_GLX) && defined(HAS_XRANDR)
  #include <sstream>
  #include <X11/extensions/Xrandr.h>
  #define NVSETTINGSCMD "nvidia-settings -nt -q RefreshRate 2>&1"
#elif defined(__APPLE__)
  #include <QuartzCore/CVDisplayLink.h>
  #include "CocoaInterface.h"
#elif defined(_WIN32) && defined(HAS_DX)
  #pragma comment (lib,"d3d9.lib")
  #if (D3DX_SDK_VERSION >= 42) //aug 2009 sdk and up there is no dxerr9 anymore
    #include <Dxerr.h>
    #pragma comment (lib,"DxErr.lib")
  #else
    #include <dxerr9.h>
    #define DXGetErrorString(hr)      DXGetErrorString9(hr)
    #define DXGetErrorDescription(hr) DXGetErrorDescription9(hr)
    #pragma comment (lib,"Dxerr9.lib")
  #endif
  #include "WindowingFactory.h"
  #include "AdvancedSettings.h"
#endif

using namespace std;

#if defined(_WIN32) && defined(HAS_DX)

  void CD3DCallback::Reset()
  {
    m_devicevalid = true;
    m_deviceused = false;
  }

  void CD3DCallback::OnDestroyDevice()
  {
    CSingleLock lock(m_critsection);
    m_devicevalid = false;
    while (m_deviceused)
    {
      lock.Leave();
      m_releaseevent.Wait();
      lock.Enter();
    }
  }

  void CD3DCallback::OnCreateDevice()
  {
    CSingleLock lock(m_critsection);
    m_devicevalid = true;
    m_createevent.Set();
  }

  void CD3DCallback::Aquire()
  {
    CSingleLock lock(m_critsection);
    while(!m_devicevalid)
    {
      lock.Leave();
      m_createevent.Wait();
      lock.Enter();
    }
    m_deviceused = true;
  }

  void CD3DCallback::Release()
  {
    CSingleLock lock(m_critsection);
    m_deviceused = false;
    m_releaseevent.Set();
  }

  bool CD3DCallback::IsValid()
  {
    return m_devicevalid;
  }

#endif

CVideoReferenceClock::CVideoReferenceClock()
{
  m_SystemFrequency = CurrentHostFrequency();
  m_AdjustedFrequency = m_SystemFrequency;
  m_ClockOffset = 0;
  m_TotalMissedVblanks = 0;
  m_UseVblank = false;
  m_Started.Reset();
}

void CVideoReferenceClock::Process()
{
  bool SetupSuccess = false;
  int64_t Now;

#if defined(_WIN32) && defined(HAS_DX)
  //register callback
  m_D3dCallback.Reset();
  g_Windowing.Register(&m_D3dCallback);
#endif

  while(!m_bStop)
  {
    //set up the vblank clock
#if defined(HAS_GLX) && defined(HAS_XRANDR)
    SetupSuccess = SetupGLX();
#elif defined(_WIN32) && defined(HAS_DX)
    SetupSuccess = SetupD3D();
#elif defined(__APPLE__)
    SetupSuccess = SetupCocoa();
#elif defined(HAS_GLX)
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: compiled without RandR support");
#elif defined(_WIN32)
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: only available on directx build");
#else
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: no implementation available");
#endif

    CSingleLock SingleLock(m_CritSection);
    Now = CurrentHostCounter();
    m_CurrTime = Now + m_ClockOffset; //add the clock offset from the previous time we stopped
    m_AdjustedFrequency = m_SystemFrequency;
    m_TotalMissedVblanks = 0;
    m_Started.Set();

    if (SetupSuccess)
    {
      m_UseVblank = true;          //tell other threads we're using vblank as clock
      m_VblankTime = Now;          //initialize the timestamp of the last vblank
      SingleLock.Leave();

      //run the clock
#if defined(HAS_GLX) && defined(HAS_XRANDR)
      RunGLX();
#elif defined(_WIN32) && defined(HAS_DX)
      RunD3D();
#elif defined(__APPLE__)
      RunCocoa();
#endif

    }
    else
    {
      SingleLock.Leave();
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setup failed, falling back to CurrentHostCounter()");
    }

    SingleLock.Enter();
    m_UseVblank = false;                       //we're back to using the systemclock
    Now = CurrentHostCounter();                //set the clockoffset between the vblank clock and systemclock
    m_ClockOffset = m_CurrTime - Now;
    SingleLock.Leave();

    //clean up the vblank clock
#if defined(HAS_GLX) && defined(HAS_XRANDR)
    CleanupGLX();
#elif defined(_WIN32) && defined(HAS_DX)
    CleanupD3D();
#elif defined(__APPLE__)
    CleanupCocoa();
#endif
    if (!SetupSuccess) break;
  }

#if defined(_WIN32) && defined(HAS_DX)
  g_Windowing.Unregister(&m_D3dCallback);
#endif
}

bool CVideoReferenceClock::WaitStarted(int MSecs)
{
  //not waiting here can cause issues with alsa
  return m_Started.WaitMSec(MSecs);
}

#if defined(HAS_GLX) && defined(HAS_XRANDR)
bool CVideoReferenceClock::SetupGLX()
{
  int singleBufferAttributes[] = {
    GLX_RGBA,
    GLX_RED_SIZE,      0,
    GLX_GREEN_SIZE,    0,
    GLX_BLUE_SIZE,     0,
    None
  };

  int ReturnV, SwaMask;
  unsigned int GlxTest;
  XSetWindowAttributes Swa;

  m_Dpy = NULL;
  m_vInfo = NULL;
  m_Context = NULL;
  m_Window = NULL;

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up GLX");

  m_Dpy = XOpenDisplay(NULL);
  if (!m_Dpy)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Unable to open display");
    return false;
  }

  if (!glXQueryExtension(m_Dpy, NULL, NULL))
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: X server does not support GLX");
    return false;
  }

  istringstream Extensions(strstr(glXQueryExtensionsString(m_Dpy, DefaultScreen(m_Dpy)), "SGI_video_sync"));
  string SgiVideoSync;
  Extensions >> SgiVideoSync;
  if (SgiVideoSync != "SGI_video_sync")
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: X server does not support SGI_video_sync");
    return false;
  }

  m_vInfo = glXChooseVisual(m_Dpy, DefaultScreen(m_Dpy), singleBufferAttributes);
  if (!m_vInfo)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXChooseVisual returned NULL");
    return false;
  }

  Swa.border_pixel = 0;
  Swa.event_mask = StructureNotifyMask;
  Swa.colormap = XCreateColormap(m_Dpy, RootWindow(m_Dpy, m_vInfo->screen), m_vInfo->visual, AllocNone );
  SwaMask = CWBorderPixel | CWColormap | CWEventMask;

  m_Window = XCreateWindow(m_Dpy, RootWindow(m_Dpy, m_vInfo->screen), 0, 0, 256, 256, 0,
                           m_vInfo->depth, InputOutput, m_vInfo->visual, SwaMask, &Swa);

  m_Context = glXCreateContext(m_Dpy, m_vInfo, NULL, True);
  if (!m_Context)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXCreateContext returned NULL");
    return false;
  }

  ReturnV = glXMakeCurrent(m_Dpy, m_Window, m_Context);
  if (ReturnV != True)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
    return false;
  }

  m_glXWaitVideoSyncSGI = (int (*)(int, int, unsigned int*))glXGetProcAddress((const GLubyte*)"glXWaitVideoSyncSGI");
  if (!m_glXWaitVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI not found");
    return false;
  }

  ReturnV = m_glXWaitVideoSyncSGI(2, 0, &GlxTest);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
    return false;
  }

  m_glXGetVideoSyncSGI = (int (*)(unsigned int*))glXGetProcAddress((const GLubyte*)"glXGetVideoSyncSGI");
  if (!m_glXGetVideoSyncSGI)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI not found");
    return false;
  }

  ReturnV = m_glXGetVideoSyncSGI(&GlxTest);
  if (ReturnV)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXGetVideoSyncSGI returned %i", ReturnV);
    return false;
  }

  XRRSizes(m_Dpy, m_vInfo->screen, &ReturnV);
  if (ReturnV == 0)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: RandR not supported");
    return false;
  }

  //set up receiving of RandR events, we'll get one when the refreshrate changes
  XRRQueryExtension(m_Dpy, &m_RREventBase, &ReturnV);
  XRRSelectInput(m_Dpy, RootWindow(m_Dpy, m_vInfo->screen), RRScreenChangeNotifyMask);

  UpdateRefreshrate(true); //forced refreshrate update
  m_MissedVblanks = 0;

  return true;
}

bool CVideoReferenceClock::ParseNvSettings(int& RefreshRate)
{
  double fRefreshRate;
  char   Buff[255];
  int    ReturnV;
  struct lconv *Locale = localeconv();
  FILE*  NvSettings;

  const char* VendorPtr = (const char*)glGetString(GL_VENDOR);
  if (!VendorPtr)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: glGetString(GL_VENDOR) returned NULL, not using nvidia-settings");
    return false;
  }

  CStdString Vendor = VendorPtr;
  Vendor.ToLower();
  if (Vendor.find("nvidia") == std::string::npos)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GL_VENDOR:%s, not using nvidia-settings", Vendor.c_str());
    return false;
  }

  NvSettings = popen(NVSETTINGSCMD, "r");
  if (!NvSettings)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: %s: %s", NVSETTINGSCMD, strerror(errno));
    return false;
  }

  ReturnV = fscanf(NvSettings, "%254[^\n]", Buff);
  pclose(NvSettings);
  if (ReturnV != 1)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: %s produced no output", NVSETTINGSCMD);
    return false;
  }

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: output of %s: %s", NVSETTINGSCMD, Buff);

  for (int i = 0; i < 255 && Buff[i]; i++)
  {
      //workaround for locale mismatch
    if (Buff[i] == '.' || Buff[i] == ',')
      Buff[i] = *Locale->decimal_point;
  }

  ReturnV = sscanf(Buff, "%lf", &fRefreshRate);
  if (ReturnV != 1 || fRefreshRate <= 0.0)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: can't make sense of that");
    return false;
  }

  RefreshRate = MathUtils::round_int(fRefreshRate);
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate by nvidia-settings: %f hertz, rounding to %i hertz",
            fRefreshRate, RefreshRate);

  return true;
}

int CVideoReferenceClock::GetRandRRate()
{
  int RefreshRate;
  XRRScreenConfiguration *CurrInfo;

  CurrInfo = XRRGetScreenInfo(m_Dpy, RootWindow(m_Dpy, m_vInfo->screen));
  RefreshRate = XRRConfigCurrentRate(CurrInfo);
  XRRFreeScreenConfigInfo(CurrInfo);

  return RefreshRate;
}

void CVideoReferenceClock::CleanupGLX()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up GLX");

  if (m_vInfo)
  {
    XFree(m_vInfo);
    m_vInfo = NULL;
  }
  if (m_Context)
  {
    glXMakeCurrent(m_Dpy, None, NULL);
    glXDestroyContext(m_Dpy, m_Context);
    m_Context = NULL;
  }
  if (m_Window)
  {
    XDestroyWindow(m_Dpy, m_Window);
    m_Window = NULL;
  }
  if (m_Dpy)
  {
    XCloseDisplay(m_Dpy);
    m_Dpy = NULL;
  }
}

void CVideoReferenceClock::RunGLX()
{
  unsigned int  PrevVblankCount;
  unsigned int  VblankCount;
  int           ReturnV;
  bool          IsReset = false;
  int64_t       Now;

  CSingleLock SingleLock(m_CritSection);
  SingleLock.Leave();

  //get the current vblank counter
  m_glXGetVideoSyncSGI(&VblankCount);
  PrevVblankCount = VblankCount;

  while(!m_bStop)
  {
    //wait for the next vblank
    ReturnV = m_glXWaitVideoSyncSGI(2, (VblankCount + 1) % 2, &VblankCount);
    m_glXGetVideoSyncSGI(&VblankCount); //the vblank count returned by glXWaitVideoSyncSGI is not always correct
    Now = CurrentHostCounter();         //get the timestamp of this vblank

    if(ReturnV)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXWaitVideoSyncSGI returned %i", ReturnV);
      return;
    }

    if (VblankCount > PrevVblankCount)
    {
      //update the vblank timestamp, update the clock and send a signal that we got a vblank
      SingleLock.Enter();
      m_VblankTime = Now;
      UpdateClock((int)(VblankCount - PrevVblankCount), true);
      SingleLock.Leave();
      SendVblankSignal();
      UpdateRefreshrate();

      IsReset = false;
    }
    else
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Vblank counter has reset");

      //only try reattaching once
      if (IsReset)
        return;

      //because of a bug in the nvidia driver, glXWaitVideoSyncSGI breaks when the vblank counter resets
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detaching glX context");
      ReturnV = glXMakeCurrent(m_Dpy, None, NULL);
      if (ReturnV != True)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
        return;
      }

      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Attaching glX context");
      glXMakeCurrent(m_Dpy, m_Window, m_Context);
      if (ReturnV != True)
      {
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: glXMakeCurrent returned %i", ReturnV);
        return;
      }

      m_glXGetVideoSyncSGI(&VblankCount);

      IsReset = true;
    }
    PrevVblankCount = VblankCount;
  }
}

#elif defined(_WIN32) && defined(HAS_DX)

void CVideoReferenceClock::RunD3D()
{
  D3DRASTER_STATUS RasterStatus;
  int64_t       Now;
  int64_t       LastVBlankTime;
  unsigned int  LastLine;
  int           NrVBlanks;
  double        VBlankTime;
  int           ReturnV;

  CSingleLock SingleLock(m_CritSection);
  SingleLock.Leave();

  //get the scanline we're currently at
  m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (RasterStatus.InVBlank) LastLine = 0;
  else LastLine = RasterStatus.ScanLine;

  //init the vblanktime
  Now = CurrentHostCounter();
  LastVBlankTime = Now;

  while(!m_bStop && m_D3dCallback.IsValid())
  {
    //get the scanline we're currently at
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned returned %s: %s",
                DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return;
    }

    //if InVBlank is set, or the current scanline is lower than the previous scanline, a vblank happened
    if ((RasterStatus.InVBlank && LastLine > 0) || (RasterStatus.ScanLine < LastLine))
    {
      //calculate how many vblanks happened
      Now = CurrentHostCounter();
      VBlankTime = (double)(Now - LastVBlankTime) / (double)m_SystemFrequency;
      NrVBlanks = MathUtils::round_int(VBlankTime * (double)m_RefreshRate);

      //update the vblank timestamp, update the clock and send a signal that we got a vblank
      SingleLock.Enter();
      m_VblankTime = Now;
      UpdateClock(NrVBlanks, true);
      SingleLock.Leave();
      SendVblankSignal();

      if (UpdateRefreshrate())
      {
        //we have to measure the refreshrate again
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: Displaymode changed");
        return;
      }

      //save the timestamp of this vblank so we can calculate how many vblanks happened next time
      LastVBlankTime = Now;

      //because we had a vblank, sleep until half the refreshrate period
      Now = CurrentHostCounter();
      int SleepTime = (int)((LastVBlankTime + (m_SystemFrequency / m_RefreshRate / 2) - Now) * 1000 / m_SystemFrequency);
      if (SleepTime > 100) SleepTime = 100; //failsafe
      if (SleepTime > 0) ::Sleep(SleepTime);
    }
    else
    {
      ::Sleep(1);
    }

    if (RasterStatus.InVBlank) LastLine = 0;
    else LastLine = RasterStatus.ScanLine;
  }
}

//how many times we measure the refreshrate
#define NRMEASURES 6
//how long to measure in milliseconds
#define MEASURETIME 250

bool CVideoReferenceClock::SetupD3D()
{
  int ReturnV;

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Setting up Direct3d");

  m_D3dCallback.Aquire();

  //get d3d device
  m_D3dDev = g_Windowing.Get3DDevice();

  //we need a high priority thread to get accurate timing
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: SetThreadPriority failed");

  D3DCAPS9 DevCaps;
  ReturnV = m_D3dDev->GetDeviceCaps(&DevCaps);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDeviceCaps returned %s: %s",
                         DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  if (DevCaps.Caps != D3DCAPS_READ_SCANLINE)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Hardware does not support GetRasterStatus");
    return false;
  }

  D3DRASTER_STATUS RasterStatus;
  ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned returned %s: %s",
              DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  D3DDISPLAYMODE DisplayMode;
  ReturnV = m_D3dDev->GetDisplayMode(0, &DisplayMode);
  if (ReturnV != D3D_OK)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetDisplayMode returned returned %s: %s",
              DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
    return false;
  }

  //forced update of windows refreshrate
  UpdateRefreshrate(true);

  if (g_advancedSettings.m_measureRefreshrate)
  {
    //measure the refreshrate a couple times
    list<double> Measures;
    for (int i = 0; i < NRMEASURES; i++)
      Measures.push_back(MeasureRefreshrate(MEASURETIME));

    //build up a string of measured rates
    CStdString StrRates;
    for (list<double>::iterator it = Measures.begin(); it != Measures.end(); it++)
      StrRates.AppendFormat("%.2f ", *it);

    //get the top half of the measured rates
    Measures.sort();
    double RefreshRate = 0.0;
    int    NrMeasurements = 0;
    while (NrMeasurements < NRMEASURES / 2 && !Measures.empty())
    {
      if (Measures.back() > 0.0)
      {
        RefreshRate += Measures.back();
        NrMeasurements++;
      }
      Measures.pop_back();
    }

    if (NrMeasurements < NRMEASURES / 2)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: refreshrate measurements: %s, unable to get a good measurement",
        StrRates.c_str(), m_RefreshRate);
      return false;
    }

    RefreshRate /= NrMeasurements;
    m_RefreshRate = MathUtils::round_int(RefreshRate);

    CLog::Log(LOGDEBUG, "CVideoReferenceClock: refreshrate measurements: %s, assuming %i hertz", StrRates.c_str(), m_RefreshRate);
  }
  else
  {
    m_RefreshRate = m_PrevRefreshRate;
    if (m_RefreshRate == 23 || m_RefreshRate == 29 || m_RefreshRate == 59)
      m_RefreshRate++;

    CLog::Log(LOGDEBUG, "CVideoReferenceClock: detected refreshrate: %i hertz, assuming %i hertz", m_PrevRefreshRate, (int)m_RefreshRate);
  }

  m_MissedVblanks = 0;

  return true;
}

double CVideoReferenceClock::MeasureRefreshrate(int MSecs)
{
  D3DRASTER_STATUS RasterStatus;
  int64_t          Now;
  int64_t          Target;
  int64_t          Prev;
  int64_t          AvgInterval;
  int64_t          MeasureCount;
  unsigned int     LastLine;
  int              ReturnV;

  Now = CurrentHostCounter();
  Target = Now + (m_SystemFrequency * MSecs / 1000);
  Prev = -1;
  AvgInterval = 0;
  MeasureCount = 0;

  //start measuring vblanks
  LastLine = 0;
  while(Now <= Target)
  {
    ReturnV = m_D3dDev->GetRasterStatus(0, &RasterStatus);
    Now = CurrentHostCounter();
    if (ReturnV != D3D_OK)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: GetRasterStatus returned returned %s: %s",
                DXGetErrorString(ReturnV), DXGetErrorDescription(ReturnV));
      return -1.0;
    }

    if ((RasterStatus.InVBlank && LastLine != 0) || (!RasterStatus.InVBlank && RasterStatus.ScanLine < LastLine))
    { //we got a vblank
      if (Prev != -1) //need two for a measurement
      {
        AvgInterval += Now - Prev; //save how long this vblank lasted
        MeasureCount++;
      }
      Prev = Now; //save this time for the next measurement
    }

    //save the current scanline
    if (RasterStatus.InVBlank)
      LastLine = 0;
    else
      LastLine = RasterStatus.ScanLine;

    ::Sleep(1);
  }

  if (MeasureCount < 1)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Didn't measure any vblanks");
    return -1.0;
  }

  double fRefreshRate = 1.0 / ((double)AvgInterval / (double)MeasureCount / (double)m_SystemFrequency);

  return fRefreshRate;
}

void CVideoReferenceClock::CleanupD3D()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cleaning up Direct3d");
  m_D3dCallback.Release();
}

#elif defined(__APPLE__)

// Called by the Core Video Display Link whenever it's appropriate to render a frame.
static CVReturn DisplayLinkCallBack(CVDisplayLinkRef displayLink, const CVTimeStamp* inNow, const CVTimeStamp* inOutputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
  double fps = 60.0;

  if (inNow->videoRefreshPeriod > 0)
    fps = (double)inOutputTime->videoTimeScale / (double)inOutputTime->videoRefreshPeriod;

  // Create an autorelease pool (necessary to call into non-Obj-C code from Obj-C code)
  void* pool = Cocoa_Create_AutoReleasePool();

  CVideoReferenceClock *VideoReferenceClock = reinterpret_cast<CVideoReferenceClock*>(displayLinkContext);
  VideoReferenceClock->VblankHandler(inNow->hostTime, fps);

  // Destroy the autorelease pool
  Cocoa_Destroy_AutoReleasePool(pool);

  return kCVReturnSuccess;
}

bool CVideoReferenceClock::SetupCocoa()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: setting up up Cocoa");

  //init the vblank timestamp
  m_LastVBlankTime = CurrentHostCounter();
  m_MissedVblanks = 0;
  m_RefreshRate = 60;              //init the refreshrate so we don't get any division by 0 errors

  if (!Cocoa_CVDisplayLinkCreate((void*)DisplayLinkCallBack, reinterpret_cast<void*>(this)))
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Cocoa_CVDisplayLinkCreate failed");
    return false;
  }
  else
  {
    UpdateRefreshrate(true);
    return true;
  }
}

void CVideoReferenceClock::RunCocoa()
{
  //because cocoa has a vblank callback, we just keep sleeping until we're asked to stop the thread
  while(!m_bStop)
  {
    Sleep(1000);
  }
}

void CVideoReferenceClock::CleanupCocoa()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: cleaning up Cocoa");
  Cocoa_CVDisplayLinkRelease();
}

void CVideoReferenceClock::VblankHandler(int64_t nowtime, double fps)
{
  int           NrVBlanks;
  double        VBlankTime;
  int           RefreshRate = MathUtils::round_int(fps);

  CSingleLock SingleLock(m_CritSection);

  if (RefreshRate != m_RefreshRate)
  {
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", RefreshRate);
    m_RefreshRate = RefreshRate;
  }
  m_LastRefreshTime = m_CurrTime;

  //calculate how many vblanks happened
  VBlankTime = (double)(nowtime - m_LastVBlankTime) / (double)m_SystemFrequency;
  NrVBlanks = MathUtils::round_int(VBlankTime * (double)m_RefreshRate);

  //save the timestamp of this vblank so we can calculate how many happened next time
  m_LastVBlankTime = nowtime;

  //update the vblank timestamp, update the clock and send a signal that we got a vblank
  m_VblankTime = nowtime;
  UpdateClock(NrVBlanks, true);

  SingleLock.Leave();

  SendVblankSignal();
}
#endif

//this is called from the vblank run function and from CVideoReferenceClock::Wait in case of a late update
void CVideoReferenceClock::UpdateClock(int NrVBlanks, bool CheckMissed)
{
  if (CheckMissed) //set to true from the vblank run function, set to false from Wait and GetTime
  {
    if (NrVBlanks < m_MissedVblanks) //if this is true the vblank detection in the run function is wrong
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: detected %i vblanks, missed %i", NrVBlanks, m_MissedVblanks);

    NrVBlanks -= m_MissedVblanks; //subtract the vblanks we missed
    m_MissedVblanks = 0;
  }
  else
  {
    m_MissedVblanks += NrVBlanks;      //tell the vblank clock how many vblanks it missed
    m_TotalMissedVblanks += NrVBlanks; //for the codec information screen
    m_VblankTime += m_SystemFrequency * (int64_t)NrVBlanks / m_RefreshRate; //set the vblank time forward
  }

  if (NrVBlanks > 0) //update the clock with the adjusted frequency if we have any vblanks
    m_CurrTime += (int64_t)NrVBlanks * m_AdjustedFrequency / m_RefreshRate;
}

//called from dvdclock to get the time
int64_t CVideoReferenceClock::GetTime()
{
  CSingleLock SingleLock(m_CritSection);

  //when using vblank, get the time from that, otherwise use the systemclock
  if (m_UseVblank)
  {
    int64_t  NextVblank;
    int64_t  Now;

    Now = CurrentHostCounter();        //get current system time
    NextVblank = TimeOfNextVblank();   //get time when the next vblank should happen

    while(Now >= NextVblank)  //keep looping until the next vblank is in the future
    {
      UpdateClock(1, false);           //update clock when next vblank should have happened already
      NextVblank = TimeOfNextVblank(); //get time when the next vblank should happen
    }

    return m_CurrTime;
  }
  else
  {
    int64_t ClockOffset = m_ClockOffset; //get offset of clock
    SingleLock.Leave();

    return CurrentHostCounter() + ClockOffset;
  }
}

//called from dvdclock to get the clock frequency
int64_t CVideoReferenceClock::GetFrequency()
{
  return m_SystemFrequency;
}

void CVideoReferenceClock::SetSpeed(double Speed)
{
  CSingleLock SingleLock(m_CritSection);
  //dvdplayer can change the speed to fit the rereshrate
  if (m_UseVblank)
  {
    int64_t Frequency = (int64_t)((double)m_SystemFrequency * Speed);
    if (Frequency != m_AdjustedFrequency)
    {
      m_AdjustedFrequency = Frequency;
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Clock speed %f%%", GetSpeed() * 100);
    }
  }
}

double CVideoReferenceClock::GetSpeed()
{
  double Speed = 1.0;
  CSingleLock SingleLock(m_CritSection);

  //dvdplayer needs to know the speed for the resampler
  if (m_UseVblank) Speed = (double)m_AdjustedFrequency / (double)m_SystemFrequency;

  return Speed;
}

bool CVideoReferenceClock::UpdateRefreshrate(bool Forced /*= false*/)
{
  //update the refreshrate about once a second, or update immediately if a forced update is required
  if (m_CurrTime - m_LastRefreshTime < m_SystemFrequency && !Forced)
    return false;

  if (Forced)
    m_LastRefreshTime = 0;
  else
    m_LastRefreshTime = m_CurrTime;

#if defined(HAS_GLX) && defined(HAS_XRANDR)

  bool   GotEvent = Forced;
  XEvent Event;
  //check for RandR events
  while (XCheckTypedEvent(m_Dpy, m_RREventBase + RRScreenChangeNotify, &Event))
  {
    if (Event.type == m_RREventBase + RRScreenChangeNotify)
    {
      CLog::Log(LOGDEBUG, "CVideoReferenceClock: Received RandR event %i", Event.type);
      GotEvent = true;
    }
    XRRUpdateConfiguration(&Event);
  }

  if (!GotEvent) //refreshrate did not change
    return false;

  //the refreshrate can be wrong on nvidia drivers, so read it from nvidia-settings when it's available
  if (m_UseNvSettings || Forced)
  {
    int NvRefreshRate;
    //if this fails we can't get the refreshrate from nvidia-settings
    m_UseNvSettings = ParseNvSettings(NvRefreshRate);

    if (m_UseNvSettings)
    {
      CSingleLock SingleLock(m_CritSection);
      m_RefreshRate = NvRefreshRate;
      return true;
    }

    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Using RandR for refreshrate detection");
  }

  CSingleLock SingleLock(m_CritSection);
  m_RefreshRate = GetRandRRate();

  CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", (int)m_RefreshRate);

  return true;

#elif defined(_WIN32) && defined(HAS_DX)

  D3DDISPLAYMODE DisplayMode;
  m_D3dDev->GetDisplayMode(0, &DisplayMode);

  //0 indicates adapter default
  if (DisplayMode.RefreshRate == 0)
    DisplayMode.RefreshRate = 60;

  if (m_PrevRefreshRate != DisplayMode.RefreshRate  || m_Width != DisplayMode.Width || m_Height != DisplayMode.Height || Forced)
  {
    m_PrevRefreshRate = DisplayMode.RefreshRate;
    m_Width = DisplayMode.Width;
    m_Height = DisplayMode.Height;
    return true;
  }

  return false;

#elif defined(__APPLE__)
  int RefreshRate = MathUtils::round_int(Cocoa_GetCVDisplayLinkRefreshPeriod());

  if (RefreshRate != m_RefreshRate || Forced)
  {
    CSingleLock SingleLock(m_CritSection);
    CLog::Log(LOGDEBUG, "CVideoReferenceClock: Detected refreshrate: %i hertz", RefreshRate);
    m_RefreshRate = RefreshRate;
    return true;
  }
  return false;
#endif

  return false;
}

//dvdplayer needs to know the refreshrate for matching the fps of the video playing to it
int CVideoReferenceClock::GetRefreshRate()
{
  CSingleLock SingleLock(m_CritSection);

  if (m_UseVblank)
    return (int)m_RefreshRate;
  else
    return -1;
}


//this is called from CDVDClock::WaitAbsoluteClock, which is called from CXBMCRenderManager::WaitPresentTime
//it waits until a certain timestamp has passed, used for displaying videoframes at the correct moment
int64_t CVideoReferenceClock::Wait(int64_t Target)
{
  int64_t       Now;
  int           SleepTime;
  int64_t       NextVblank;
  bool          Late;

  CSingleLock SingleLock(m_CritSection);

  if (m_UseVblank) //when true the vblank is used as clock source
  {
    while (m_CurrTime < Target)
    {
      //calculate how long to sleep before we should have gotten a signal that a vblank happened
      Now = CurrentHostCounter();
      NextVblank = TimeOfNextVblank();
      SleepTime = (int)((NextVblank - Now) * 1000 / m_SystemFrequency);

      int64_t CurrTime = m_CurrTime; //save current value of the clock

      Late = false;
      if (SleepTime <= 0) //if sleeptime is 0 or lower, the vblank clock is already late in updating
      {
        Late = true;
      }
      else
      {
        m_VblankEvent.Reset();
        SingleLock.Leave();
        if (!m_VblankEvent.WaitMSec(SleepTime)) //if this returns false, it means the vblank event was not set within
          Late = true;                          //the required time
        SingleLock.Enter();
      }

      //if the vblank clock was late with its update, we update the clock ourselves
      if (Late && CurrTime == m_CurrTime)
      {
#ifndef HAVE_LIBVDPAU
        // vdpau spams the log with missed vblanks so only log if vdpau is not compiled in.
        // actually checking for vdpau enabled is too messy to be used in this routine.
        CLog::Log(LOGDEBUG, "CVideoReferenceClock: vblank clock was late: SleepTime %i", SleepTime);
#endif
        UpdateClock(1, false); //update the clock by 1 vblank
      }
    }
    return m_CurrTime;
  }
  else
  {
    int64_t ClockOffset = m_ClockOffset;
    SingleLock.Leave();
    Now = CurrentHostCounter();
    //sleep until the timestamp has passed
    SleepTime = (int)((Target - (Now + ClockOffset)) * 1000 / m_SystemFrequency);
    if (SleepTime > 0)
      ::Sleep(SleepTime);

    Now = CurrentHostCounter();
    return Now + ClockOffset;
  }
}


void CVideoReferenceClock::SendVblankSignal()
{
  m_VblankEvent.Set();
}

#define MAXVBLANKDELAY 13LL
//guess when the next vblank should happen,
//based on the refreshrate and when the previous one happened
//increase that by 30% to allow for errors
int64_t CVideoReferenceClock::TimeOfNextVblank()
{
  return m_VblankTime + (m_SystemFrequency / m_RefreshRate * MAXVBLANKDELAY / 10LL);
}

//for the codec information screen
bool CVideoReferenceClock::GetClockInfo(int& MissedVblanks, double& ClockSpeed, int& RefreshRate)
{
  if (m_UseVblank)
  {
    MissedVblanks = m_TotalMissedVblanks;
    ClockSpeed = (double)m_AdjustedFrequency / (double)m_SystemFrequency * 100.0;
    RefreshRate = (int)m_RefreshRate;
    return true;
  }
  return false;
}

CVideoReferenceClock g_VideoReferenceClock;
