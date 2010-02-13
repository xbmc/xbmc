#pragma once

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

#if !defined(_LINUX) && !defined(HAS_GL)

#include "GraphicContext.h"
#include "RenderFlags.h"
#include "BaseRenderer.h"
#include "D3DResource.h"
#include "settings/VideoSettings.h"

//#define MP_DIRECTRENDERING

#ifdef MP_DIRECTRENDERING
#define NUM_BUFFERS 3
#else
#define NUM_BUFFERS 2
#endif

#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */

#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)


#define RENDER_FLAG_EVEN        0x01
#define RENDER_FLAG_ODD         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_EVEN | RENDER_FLAG_ODD)
#define RENDER_FLAG_FIELDMASK   0x03

#define RENDER_FLAG_NOOSD       0x04 /* don't draw any osd */

/* these two flags will be used if we need to render same image twice (bob deinterlacing) */
#define RENDER_FLAG_NOLOCK      0x10   /* don't attempt to lock texture before rendering */
#define RENDER_FLAG_NOUNLOCK    0x20   /* don't unlock texture after rendering */

/* this defines what color translation coefficients */
#define CONF_FLAGS_YUVCOEF_MASK(a) ((a) & 0x07)
#define CONF_FLAGS_YUVCOEF_BT709 0x01
#define CONF_FLAGS_YUVCOEF_BT601 0x02
#define CONF_FLAGS_YUVCOEF_240M  0x03
#define CONF_FLAGS_YUVCOEF_EBU   0x04

#define CONF_FLAGS_YUV_FULLRANGE 0x08
#define CONF_FLAGS_FULLSCREEN    0x10

class CBaseTexture;

namespace DXVA { class CProcessor; }

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

enum EFIELDSYNC
{
  FS_NONE,
  FS_ODD,
  FS_EVEN,
  FS_BOTH,
};


struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;

class CWinRenderer : public CBaseRenderer
{
public:
  CWinRenderer();
  ~CWinRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};
  void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y);
  virtual void         AddProcessor(DXVA::CProcessor* processor, int64_t id);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual bool         IsConfigured() { return m_bConfigured; }

  // TODO:DIRECTX - implement these
  virtual bool         SupportsBrightness() { return true; }
  virtual bool         SupportsContrast() { return true; }
  virtual bool         SupportsGamma() { return false; }
  virtual bool         Supports(EINTERLACEMETHOD method);
  virtual bool         Supports(ESCALINGMETHOD method);

  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

protected:
  virtual void Render(DWORD flags);
  void CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride);
  virtual void ManageTextures();
  void DeleteYV12Texture(int index);
  void ClearYV12Texture(int index);
  bool CreateYV12Texture(int index);
  void CopyYV12Texture(int dest);
  int  NextYV12Texture();

  void UpdateVideoFilter();

  bool LoadEffect(CD3DEffect &effect, CStdString filename);

  // low memory renderer (default PixelShaderRenderer)
  void RenderLowMem(CD3DEffect &effect, DWORD flags);
  void RenderProcessor(DWORD flags);
  int m_iYV12RenderBuffer;
  int m_NumYV12Buffers;

  bool m_bConfigured;

  typedef CD3DTexture             YUVVIDEOPLANES[MAX_PLANES];
  typedef BYTE*                   YUVMEMORYPLANES[MAX_PLANES];
  typedef YUVVIDEOPLANES          YUVVIDEOBUFFERS[NUM_BUFFERS];
  typedef YUVMEMORYPLANES         YUVMEMORYBUFFERS[NUM_BUFFERS];

  #define PLANE_Y 0
  #define PLANE_U 1
  #define PLANE_V 2

  #define FIELD_FULL 0
  #define FIELD_ODD 1
  #define FIELD_EVEN 2

  // YV12 decoder textures
  // field index 0 is full image, 1 is odd scanlines, 2 is even scanlines
  // Since DX is single threaded, we will render all video into system memory
  // We will them copy in into the device when rendering from main thread
  YUVVIDEOBUFFERS m_YUVVideoTexture;
  YUVMEMORYBUFFERS m_YUVMemoryTexture;

  struct SProcessImage
  {
    SProcessImage()
    {
      proc = NULL;
      id   = 0;
    }

   ~SProcessImage()
    {
      Clear();
    }
    void Clear();

    DXVA::CProcessor* proc;
    int64_t           id;
  } m_Processor[NUM_BUFFERS];

  CD3DTexture m_HQKernelTexture;
  CD3DEffect  m_YUV2RGBEffect;
  CD3DEffect  m_YUV2RGBHQScalerEffect;

  ESCALINGMETHOD m_scalingMethod;
  ESCALINGMETHOD m_scalingMethodGui;

  D3DCAPS9 m_deviceCaps;

  bool m_bUseHQScaler;
  bool m_bFilterInitialized;

  // clear colour for "black" bars
  DWORD m_clearColour;
  unsigned int m_flags;
};


class CPixelShaderRenderer : public CWinRenderer
{
public:
  CPixelShaderRenderer();
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);

protected:
  virtual void Render(DWORD flags);
};

#else
#include "LinuxRenderer.h"
#endif


