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

#if !defined(_LINUX) && !defined(HAS_GL)

#include "guilib/GraphicContext.h"
#include "RenderFlags.h"
#include "BaseRenderer.h"
#include "guilib/D3DResource.h"
#include "RenderCapture.h"
#include "settings/VideoSettings.h"
#include "cores/dvdplayer/DVDCodecs/Video/DXVA.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "cores/VideoRenderers/RenderFormats.h"

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

class CBaseTexture;
class CYUV2RGBShader;
class CConvolutionShader;

class DllAvUtil;
class DllAvCodec;
class DllSwScale;

struct DVDVideoPicture;

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

enum RenderMethod
{
  RENDER_INVALID = 0x00,
  RENDER_PS      = 0x01,
  RENDER_SW      = 0x02,
  RENDER_DXVA    = 0x03,
};

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2
#define PLANE_UV 1

#define FIELD_FULL 0
#define FIELD_TOP 1
#define FIELD_BOT 2

struct SVideoBuffer
{
  virtual ~SVideoBuffer() {}
  virtual void Release() {};            // Release any allocated resource
  virtual void StartDecode() {};        // Prepare the buffer to receive data from dvdplayer
  virtual void StartRender() {};        // dvdplayer finished filling the buffer with data
  virtual void Clear() {};              // clear the buffer with solid black
};

// YV12 decoder textures
struct SVideoPlane
{
  CD3DTexture    texture;
  D3DLOCKED_RECT rect;                  // rect.pBits != NULL is used to know if the texture is locked
};

struct YUVBuffer : SVideoBuffer
{
  ~YUVBuffer();
  bool Create(ERenderFormat format, unsigned int width, unsigned int height);
  virtual void Release();
  virtual void StartDecode();
  virtual void StartRender();
  virtual void Clear();
  unsigned int GetActivePlanes() { return m_activeplanes; }

  SVideoPlane planes[MAX_PLANES];

private:
  unsigned int     m_width;
  unsigned int     m_height;
  ERenderFormat    m_format;
  unsigned int     m_activeplanes;
};

struct DXVABuffer : SVideoBuffer
{
  DXVABuffer()
  {
    id   = 0;
  }
  ~DXVABuffer();
  virtual void Release();
  virtual void StartDecode();

  int64_t           id;
};

class CWinRenderer : public CBaseRenderer
{
public:
  CWinRenderer();
  ~CWinRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void SetupScreenshot() {};

  bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual bool         Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation);
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual bool         AddVideoPicture(DVDVideoPicture* picture);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();
  virtual void         Reset(); /* resets renderer after seek for example */
  virtual bool         IsConfigured() { return m_bConfigured; }

  virtual std::vector<ERenderFormat> SupportedFormats() { return m_formats; }

  virtual bool         Supports(ERENDERFEATURE feature);
  virtual bool         Supports(EDEINTERLACEMODE mode);
  virtual bool         Supports(EINTERLACEMETHOD method);
  virtual bool         Supports(ESCALINGMETHOD method);

  virtual EINTERLACEMETHOD AutoInterlaceMethod();

  void                 RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

  virtual unsigned int GetProcessorSize() { return m_processor.Size(); }

protected:
  virtual void Render(DWORD flags);
  void         RenderSW();
  void         RenderPS();
  void         Stage1();
  void         Stage2();
  void         ScaleFixedPipeline();
  void         CopyAlpha(int w, int h, unsigned char* src, unsigned char *srca, int srcstride, unsigned char* dst, unsigned char* dsta, int dststride);
  virtual void ManageTextures();
  void         DeleteYV12Texture(int index);
  bool         CreateYV12Texture(int index);
  void         CopyYV12Texture(int dest);
  int          NextYV12Texture();

  void SelectRenderMethod();
  bool UpdateRenderMethod();

  void UpdateVideoFilter();
  void SelectSWVideoFilter();
  void SelectPSVideoFilter();
  void UpdatePSVideoFilter();
  bool CreateIntermediateRenderTarget();

  void RenderProcessor(DWORD flags);
  int  m_iYV12RenderBuffer;
  int  m_NumYV12Buffers;

  bool                 m_bConfigured;
  SVideoBuffer        *m_VideoBuffers[NUM_BUFFERS];
  RenderMethod         m_renderMethod;
  DXVA::CProcessor     m_processor;
  std::vector<ERenderFormat> m_formats;

  // software scale libraries (fallback if required pixel shaders version is not available)
  DllAvUtil           *m_dllAvUtil;
  DllAvCodec          *m_dllAvCodec;
  DllSwScale          *m_dllSwScale;
  struct SwsContext   *m_sw_scale_ctx;

  // Software rendering
  D3DTEXTUREFILTERTYPE m_TextureFilter;
  CD3DTexture          m_SWTarget;

  // PS rendering
  bool                 m_bUseHQScaler;
  CD3DTexture          m_IntermediateTarget;

  CYUV2RGBShader*      m_colorShader;
  CConvolutionShader*  m_scalerShader;

  ESCALINGMETHOD       m_scalingMethod;
  ESCALINGMETHOD       m_scalingMethodGui;

  D3DCAPS9 m_deviceCaps;

  bool                 m_bFilterInitialized;

  int                  m_iRequestedMethod;

  // clear colour for "black" bars
  DWORD                m_clearColour;
  ERenderFormat        m_format;
  unsigned int         m_extended_format;

  // Width and height of the render target
  // the separable HQ scalers need this info, but could the m_destRect be used instead?
  unsigned int         m_destWidth;
  unsigned int         m_destHeight;
};

#else
#include "LinuxRenderer.h"
#endif


