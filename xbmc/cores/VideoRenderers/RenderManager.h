#ifndef XBOX_VIDEO_RENDERER
#define XBOX_VIDEO_RENDERER

#include "XBoxRenderer.h"
#include "..\..\utils\SharedSection.h"

class CXBoxRenderManager
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
  void RenderUpdate(bool clear) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->RenderUpdate(clear); };
  void CheckScreenSaver() { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->CheckScreenSaver(); };
  void SetupScreenshot();
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);
  void SetViewMode(int iViewMode) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->SetViewMode(iViewMode); };

  // Functions called from mplayer
  inline void WaitForFlip()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->WaitForFlip();
  }
  unsigned int Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps);
  inline unsigned int GetImage(YV12Image *image)
  {
    CSharedLock lock(m_sharedSection);
    if (m_bPauseDrawing) return 0;
    if (m_pRenderer)
      return m_pRenderer->GetImage(image);
    return 0;
  }
  inline void ReleaseImage()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->ReleaseImage();
  }
  inline unsigned int PutImage(YV12Image *image)
  {
    CSharedLock lock(m_sharedSection);
    if (m_bPauseDrawing) return 0;
    if (m_pRenderer)
      return m_pRenderer->PutImage(image);
    return 0;
  }
  inline unsigned int DrawFrame(unsigned char *src[])
  {
    CSharedLock lock(m_sharedSection);
    if (m_bPauseDrawing) return 0;
    if (m_pRenderer)
      return m_pRenderer->DrawFrame(src);
    return 0;
  }
  inline unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
  {
    CSharedLock lock(m_sharedSection);
    if (m_bPauseDrawing) return 0;
    if (m_pRenderer)
      return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
    return 0;
  }

  inline void PrepareDisplay()
  {
    CSharedLock lock(m_sharedSection);
    if (m_bPauseDrawing) return;
    if (m_pRenderer)
    {
      m_pRenderer->PrepareDisplay();
    }
  }

  inline void FlipPage(bool bAsync = false)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
    {
      if (m_bPauseDrawing)
        m_pRenderer->RenderBlank();
      else
        m_pRenderer->FlipPage(bAsync);
    }
  }

  inline int GetAsyncFlipTime()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetAsyncFlipTime();
    else
      return 0;
  }

  unsigned int PreInit();
  void UnInit();
  inline void DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
  {
    CSharedLock lock(m_sharedSection);
    if (m_bPauseDrawing) return ;
    if (m_pRenderer)
      m_pRenderer->DrawAlpha(x0, y0, w, h, src, srca, stride);
  }
  inline int GetOSDWidth() { CSharedLock lock(m_sharedSection); if (m_pRenderer) return m_pRenderer->GetNormalDisplayWidth(); else return 0;};
  inline int GetOSDHeight() { CSharedLock lock(m_sharedSection); if (m_pRenderer) return m_pRenderer->GetNormalDisplayHeight(); else return 0; };
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}

  inline void SetFieldSync(EFIELDSYNC mSync) 
  { 
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer) 
      m_pRenderer->SetFieldSync(mSync); 
  } ;

  CXBoxRenderer *m_pRenderer;
protected:
  float m_fSourceFrameRatio; // the frame aspect ratio of the source (corrected for pixel ratio)
  unsigned int m_iSourceWidth;    // width
  unsigned int m_iSourceHeight;   // height
  bool m_bPauseDrawing;   // true if we should pause rendering

  bool m_bIsStarted;
  CSharedSection m_sharedSection;
};

extern CXBoxRenderManager g_renderManager;

#endif
