#pragma once

#if defined(HAS_XBOX_HARDWARE)
#include "XBoxRenderer.h"
#elif defined (HAS_SDL_OPENGL)
#include "LinuxRendererGL.h"
#elif defined(HAS_SDL)
#include "LinuxRenderer.h"
#elif defined (WIN32)
#include "WinRenderManager.h"
#endif

#include "../../utils/SharedSection.h"

class CXBoxRenderManager : private CThread
{
public:
  CXBoxRenderManager();
  ~CXBoxRenderManager();

  void ChangeRenderers();

  // Functions called from the GUI
  void GetVideoRect(RECT &rs, RECT &rd) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->GetVideoRect(rs, rd); };
  float GetAspectRatio() { CSharedLock lock(m_sharedSection); if (m_pRenderer) return m_pRenderer->GetAspectRatio(); else return 1.0f; };
  void AutoCrop(bool bCrop = true) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->AutoCrop(bCrop); };
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();

#ifndef HAS_SDL
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);
#else
  void CreateThumbnail(SDL_Surface *surface, unsigned int width, unsigned int height);
#endif

  void SetViewMode(int iViewMode) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->SetViewMode(iViewMode); };

  // Functions called from mplayer
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

  // a call to GetImage must be followed by a call to releaseimage if getimage was successfull
  // failure to do so will result in deadlock
  inline int GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetImage(image, source, readonly);
    return -1;
  }
  inline void ReleaseImage(int source = AUTOSOURCE, bool preserve = false)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->ReleaseImage(source, preserve);
  }
  inline unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
    return 0;
  }

  void FlipPage(DWORD timestamp = 0L, int source = -1, EFIELDSYNC sync = FS_NONE);
  unsigned int PreInit();
  void UnInit();

  inline void DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->DrawAlpha(x0, y0, w, h, src, srca, stride);
  }
  inline void Reset()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->Reset();
  }
  RESOLUTION GetResolution()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetResolution();
    else
      return INVALID;
  }

  float GetMaximumFPS();
  inline DWORD GetPresentDelay() { return m_presentdelay;  }
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}

  #ifdef HAS_SDL_OPENGL
  CLinuxRendererGL *m_pRenderer;
  #elif defined(HAS_SDL)
  CLinuxRenderer *m_pRenderer;
  #else
  CXBoxRenderer *m_pRenderer;
  #endif

  void Present();

protected:

  void PresentSingle();
  void PresentWeave();
  void PresentBob();
  void PresentBlend();

  bool m_bPauseDrawing;   // true if we should pause rendering

  bool m_bIsStarted;
  CSharedSection m_sharedSection;

  int m_rendermethod;

  // render thread
  CEvent m_eventFrame;
  CEvent m_eventPresented;

  DWORD m_presentdelay;
  DWORD m_presenttime;
  EFIELDSYNC m_presentfield;

  virtual void Process();

};

extern CXBoxRenderManager g_renderManager;


