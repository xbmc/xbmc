#pragma once

#if !defined(HAS_XBOX_HARDWARE)

// we use this manager instead
#include "WinRenderManager.h"

#else

#include "XBoxRenderer.h"
#include "../../utils/SharedSection.h"
#include "../../utils/Thread.h"

class CXBoxRenderManager : private CThread
{
public:
  CXBoxRenderManager();
  ~CXBoxRenderManager();

  void ChangeRenderers();

  // Functions called from the GUI
  void GetVideoRect(RECT &rs, RECT &rd) { CMediaSourcedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->GetVideoRect(rs, rd); };
  float GetAspectRatio() { CMediaSourcedLock lock(m_sharedSection); if (m_pRenderer) return m_pRenderer->GetAspectRatio(); else return 1.0f; };
  void AutoCrop(bool bCrop = true) { CMediaSourcedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->AutoCrop(bCrop); };
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);
  void SetViewMode(int iViewMode) { CMediaSourcedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->SetViewMode(iViewMode); };

  // Functions called from mplayer
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

  // a call to GetImage must be followed by a call to releaseimage if getimage was successfull
  // failure to do so will result in deadlock
  inline int GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false)
  {
    CMediaSourcedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetImage(image, source, readonly);
    return -1;
  }
  inline void ReleaseImage(int source = AUTOSOURCE, bool preserve = false)
  {
    CMediaSourcedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->ReleaseImage(source, preserve);
  }
  inline unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
  {
    CMediaSourcedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
    return 0;
  }

  void FlipPage(DWORD timestamp = 0L, int source = -1, EFIELDSYNC sync = FS_NONE);
  unsigned int PreInit();
  void UnInit();

  inline void DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
  {
    CMediaSourcedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->DrawAlpha(x0, y0, w, h, src, srca, stride);
  }
  inline void Reset()
  {
    CMediaSourcedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->Reset();
  }
  RESOLUTION GetResolution()
  {
    CMediaSourcedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetResolution();
    else
      return INVALID;
  }

  float GetMaximumFPS();
  inline DWORD GetPresentDelay() { return m_presentdelay;  }
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}

  CXBoxRenderer *m_pRenderer;
protected:
  inline void Present();
  void PresentSingle();
  void PresentWeave();
  void PresentBob();
  void PresentBlend();

  bool m_bPauseDrawing;   // true if we should pause rendering

  bool m_bIsStarted;
  CMediaSourcedSection m_sharedSection;

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

#endif
