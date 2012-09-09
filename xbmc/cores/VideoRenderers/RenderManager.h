#pragma once

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

#include <list>

#include "cores/VideoRenderers/BaseRenderer.h"
#include "guilib/Geometry.h"
#include "guilib/Resolution.h"
#include "threads/SharedSection.h"
#include "threads/Thread.h"
#include "settings/VideoSettings.h"
#include "OverlayRenderer.h"

class CRenderCapture;

namespace DXVA { class CProcessor; }
namespace VAAPI { class CSurfaceHolder; }
class CVDPAU;
struct DVDVideoPicture;

#define ERRORBUFFSIZE 30

class CWinRenderer;
class CLinuxRenderer;
class CLinuxRendererGL;
class CLinuxRendererGLES;

class CXBMCRenderManager
{
public:
  CXBMCRenderManager();
  ~CXBMCRenderManager();

  // Functions called from the GUI
  void GetVideoRect(CRect &source, CRect &dest);
  float GetAspectRatio();
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();

  CRenderCapture* AllocRenderCapture();
  void ReleaseRenderCapture(CRenderCapture* capture);
  void Capture(CRenderCapture *capture, unsigned int width, unsigned int height, int flags);
  void ManageCaptures();

  void SetViewMode(int iViewMode);

  // Functions called from mplayer
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format,  unsigned int orientation);
  bool IsConfigured();

  int AddVideoPicture(DVDVideoPicture& picture);

  void FlipPage(volatile bool& bStop, double timestamp = 0.0, int source = -1, EFIELDSYNC sync = FS_NONE);
  unsigned int PreInit();
  void UnInit();
  bool Flush();

  void AddOverlay(CDVDOverlay* o, double pts)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddOverlay(o, pts);
  }

  void AddCleanup(OVERLAY::COverlay* o)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddCleanup(o);
  }

  void Reset();

  RESOLUTION GetResolution();

  float GetMaximumFPS();
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}
  double GetDisplayLatency() { return m_displayLatency; }

  bool Supports(ERENDERFEATURE feature);
  bool Supports(EDEINTERLACEMODE method);
  bool Supports(EINTERLACEMETHOD method);
  bool Supports(ESCALINGMETHOD method);

  EINTERLACEMETHOD AutoInterlaceMethod(EINTERLACEMETHOD mInt);

  double GetPresentTime();
  void  WaitPresentTime(double presenttime);

  CStdString GetVSyncState();

  void UpdateResolution();

#ifdef HAS_GL
  CLinuxRendererGL    *m_pRenderer;
#elif HAS_GLES == 2
  CLinuxRendererGLES  *m_pRenderer;
#elif defined(HAS_DX)
  CWinRenderer        *m_pRenderer;
#elif defined(HAS_SDL)
  CLinuxRenderer      *m_pRenderer;
#endif

  unsigned int GetProcessorSize();

  // Supported pixel formats, can be called before configure
  std::vector<ERenderFormat> SupportedFormats();

  void Present();
  void Recover(); // called after resolution switch if something special is needed

  CSharedSection& GetSection() { return m_sharedSection; };

  void RegisterRenderUpdateCallBack(const void *ctx, RenderUpdateCallBackFn fn);

protected:
  void Render(bool clear, DWORD flags, DWORD alpha);

  void PresentSingle(bool clear, DWORD flags, DWORD alpha);
  void PresentFields(bool clear, DWORD flags, DWORD alpha);
  void PresentBlend(bool clear, DWORD flags, DWORD alpha);

  EINTERLACEMETHOD AutoInterlaceMethodInternal(EINTERLACEMETHOD mInt);

  bool m_bPauseDrawing;   // true if we should pause rendering

  bool m_bIsStarted;
  CSharedSection m_sharedSection;

  bool m_bReconfigured;

  int m_rendermethod;

  enum EPRESENTSTEP
  {
    PRESENT_IDLE     = 0
  , PRESENT_FLIP
  , PRESENT_FRAME
  , PRESENT_FRAME2
  };

  enum EPRESENTMETHOD
  {
    PRESENT_METHOD_SINGLE = 0,
    PRESENT_METHOD_BLEND,
    PRESENT_METHOD_WEAVE,
    PRESENT_METHOD_BOB,
  };

  double m_displayLatency;
  void UpdateDisplayLatency();

  double     m_presenttime;
  double     m_presentcorr;
  double     m_presenterr;
  double     m_errorbuff[ERRORBUFFSIZE];
  int        m_errorindex;
  EFIELDSYNC m_presentfield;
  EPRESENTMETHOD m_presentmethod;
  EPRESENTSTEP     m_presentstep;
  int        m_presentsource;
  CEvent     m_presentevent;
  CEvent     m_flushEvent;


  OVERLAY::CRenderer m_overlays;

  void RenderCapture(CRenderCapture* capture);
  void RemoveCapture(CRenderCapture* capture);
  CCriticalSection           m_captCritSect;
  std::list<CRenderCapture*> m_captures;
  //set to true when adding something to m_captures, set to false when m_captures is made empty
  //std::list::empty() isn't thread safe, using an extra bool will save a lock per render when no captures are requested
  bool                       m_hasCaptures; 
};

extern CXBMCRenderManager g_renderManager;
