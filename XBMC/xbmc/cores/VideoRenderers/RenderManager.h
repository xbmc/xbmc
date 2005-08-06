#ifndef XBOX_VIDEO_RENDERER
#define XBOX_VIDEO_RENDERER

#include "XBoxRenderer.h"

class CXBoxRenderManager
{
public:
  CXBoxRenderManager();
  ~CXBoxRenderManager();

  void ChangeRenderers();

  // Functions called from the GUI
  void GetVideoRect(RECT &rs, RECT &rd) { if (!m_bChanging && m_pRenderer) m_pRenderer->GetVideoRect(rs, rd); };
  float GetAspectRatio() { if (!m_bChanging && m_pRenderer) return m_pRenderer->GetAspectRatio(); else return 1.0f; };
  void AutoCrop(bool bCrop = true) { if (!m_bChanging && m_pRenderer) m_pRenderer->AutoCrop(bCrop); };
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear) { if (!m_bChanging && m_pRenderer) m_pRenderer->RenderUpdate(clear); };
  void CheckScreenSaver() { if (!m_bChanging && m_pRenderer) m_pRenderer->CheckScreenSaver(); };
  void SetupScreenshot();
  void CreateThumbnail(LPDIRECT3DSURFACE8 surface, unsigned int width, unsigned int height);
  void SetViewMode(int iViewMode) { if (!m_bChanging && m_pRenderer) m_pRenderer->SetViewMode(iViewMode); };

  // Functions called from mplayer
  inline void WaitForFlip()
  {
    if (!m_bChanging && m_pRenderer)
      m_pRenderer->WaitForFlip();
  }
  unsigned int Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps);
  inline unsigned int GetImage(YV12Image *image)
  {
    if (m_bPauseDrawing) return 0;
    if (!m_bChanging && m_pRenderer)
      return m_pRenderer->GetImage(image);
    return 0;
  }
  inline void ReleaseImage()
  {
    if (!m_bChanging && m_pRenderer)
      m_pRenderer->ReleaseImage();
  }
  inline unsigned int PutImage(YV12Image *image)
  {
    if (m_bPauseDrawing) return 0;
    if (!m_bChanging && m_pRenderer)
      return m_pRenderer->PutImage(image);
    return 0;
  }
  inline unsigned int DrawFrame(unsigned char *src[])
  {
    if (m_bPauseDrawing) return 0;
    if (!m_bChanging && m_pRenderer)
      return m_pRenderer->DrawFrame(src);
    return 0;
  }
  inline unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
  {
    if (m_bPauseDrawing) return 0;
    if (!m_bChanging && m_pRenderer)
      return m_pRenderer->DrawSlice(src, stride, w, h, x, y);
    return 0;
  }

  inline void FlipPageAsync()
  {
    if (!m_bChanging && m_pRenderer)
    {
      if (m_bPauseDrawing)
        m_pRenderer->RenderBlank();
      else
        m_pRenderer->FlipPageAsync();
    }
  }

  inline void FlipPage()
  {
    if (!m_bChanging && m_pRenderer)
    {
      if (m_bPauseDrawing)
        m_pRenderer->RenderBlank();
      else
        m_pRenderer->FlipPage();
    }
  }

  inline int GetAsyncFlipTime()
  {
    if (!m_bChanging && m_pRenderer)
      return m_pRenderer->GetAsyncFlipTime();
    else
      return 0;
  }

  unsigned int PreInit();
  void UnInit();
  inline void DrawAlpha(int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
  {
    if (m_bPauseDrawing) return ;
    if (!m_bChanging && m_pRenderer)
      m_pRenderer->DrawAlpha(x0, y0, w, h, src, srca, stride);
  }
  inline int GetOSDWidth() { if (!m_bChanging && m_pRenderer) return m_pRenderer->GetNormalDisplayWidth(); else return 0;};
  inline int GetOSDHeight() { if (!m_bChanging && m_pRenderer) return m_pRenderer->GetNormalDisplayHeight(); else return 0; };
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}

  inline void SetFieldSync(EFIELDSYNC mSync) 
  { 
    if (!m_bChanging && m_pRenderer) 
      m_pRenderer->SetFieldSync(mSync); 
  } ;

  CXBoxRenderer *m_pRenderer;
protected:
  float m_fSourceFrameRatio; // the frame aspect ratio of the source (corrected for pixel ratio)
  unsigned int m_iSourceWidth;    // width
  unsigned int m_iSourceHeight;   // height
  bool m_bPauseDrawing;   // true if we should pause rendering

  bool m_bChanging;     // true when we are changing renderers
  bool m_bIsStarted;
};

extern CXBoxRenderManager g_renderManager;

#endif
