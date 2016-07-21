#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#if !defined(TARGET_POSIX) && !defined(HAS_GL)
#include <vector>

#include "BaseRenderer.h"
#include "HwDecRender/DXVAHD.h"
#include "guilib/D3DResource.h"
#include "RenderFormats.h"
#include "RenderCapture.h"
#include "settings/VideoSettings.h"

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
  virtual void StartDecode() {};        // Prepare the buffer to receive data from VideoPlayer
  virtual void StartRender() {};        // VideoPlayer finished filling the buffer with data
  virtual void Clear() {};              // clear the buffer with solid black
  virtual bool IsReadyToRender() { return true; };
};

// YV12 decoder textures
struct SVideoPlane
{
  CD3DTexture    texture;
  D3D11_MAPPED_SUBRESOURCE rect;       // rect.pBits != NULL is used to know if the texture is locked
};

struct YUVBuffer : SVideoBuffer
{
  YUVBuffer() : m_width(0), m_height(0)
              , m_format(RENDER_FMT_NONE)
              , m_activeplanes(0)
              , m_locked(false)
              , m_staging(nullptr)
              , m_bPending(false)
  {
    memset(&m_sDesc, 0, sizeof(CD3D11_TEXTURE2D_DESC));
  }
  ~YUVBuffer();
  bool Create(ERenderFormat format, unsigned int width, unsigned int height, bool dynamic);
  unsigned int GetActivePlanes() { return m_activeplanes; }
  bool CopyFromPicture(DVDVideoPicture &picture);

  // SVideoBuffer overrides
  void Release() override;
  void StartDecode() override;
  void StartRender() override;
  void Clear() override;
  bool IsReadyToRender() override;

  SVideoPlane planes[MAX_PLANES];

private:
  bool CopyFromDXVA(ID3D11VideoDecoderOutputView* pView);
  void PerformCopy();

  unsigned int     m_width;
  unsigned int     m_height;
  ERenderFormat    m_format;
  unsigned int     m_activeplanes;
  bool             m_locked;
  D3D11_MAP        m_mapType;
  ID3D11Texture2D* m_staging;
  CD3D11_TEXTURE2D_DESC m_sDesc;
  bool             m_bPending;
};

struct DXVABuffer : SVideoBuffer
{
  DXVABuffer() { pic = nullptr; }
  ~DXVABuffer() { SAFE_RELEASE(pic); }
  DXVA::CRenderPicture *pic;
  unsigned int frameIdx;
};

class CWinRenderer : public CBaseRenderer
{
public:
  CWinRenderer();
  ~CWinRenderer();

  virtual void Update();

  bool RenderCapture(CRenderCapture* capture);

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation);
  virtual int GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
  virtual void ReleaseImage(int source, bool preserve = false);
  virtual void AddVideoPictureHW(DVDVideoPicture &picture, int index) override;
  virtual bool IsPictureHW(DVDVideoPicture &picture) override;
  virtual void FlipPage(int source);
  virtual void PreInit();
  virtual void UnInit();
  virtual void Reset(); /* resets renderer after seek for example */
  virtual bool IsConfigured() { return m_bConfigured; }
  virtual void Flush();
  virtual EINTERLACEMETHOD AutoInterlaceMethod();
  virtual CRenderInfo GetRenderInfo();
  virtual void RenderUpdate(bool clear, unsigned int flags = 0, unsigned int alpha = 255);
  virtual void SetBufferSize(int numBuffers) { m_neededBuffers = numBuffers; }
  virtual void ReleaseBuffer(int idx);
  virtual bool NeedBufferForRef(int idx);
  virtual bool HandlesRenderFormat(ERenderFormat format) override;

  // Feature support
  virtual bool SupportsMultiPassRendering() { return false; }
  virtual bool Supports(ERENDERFEATURE feature);
  virtual bool Supports(EINTERLACEMETHOD method);
  virtual bool Supports(ESCALINGMETHOD method);

protected:
  virtual void Render(DWORD flags);
  void RenderSW();
  void RenderHW(DWORD flags);
  void RenderPS();
  void RenderHQ();
  virtual void ManageTextures();
  void DeleteYV12Texture(int index);
  bool CreateYV12Texture(int index);
  int NextYV12Texture();
  void SelectRenderMethod();
  void UpdateVideoFilter();
  void SelectSWVideoFilter();
  void SelectPSVideoFilter();
  void UpdatePSVideoFilter();
  bool CreateIntermediateRenderTarget(unsigned int width, unsigned int height, bool dynamic);
  bool CopyDXVA2YUVBuffer(ID3D11VideoDecoderOutputView* pView, YUVBuffer *pBuf);

  int  m_iYV12RenderBuffer;
  int  m_NumYV12Buffers;

  bool                 m_bConfigured;
  SVideoBuffer        *m_VideoBuffers[NUM_BUFFERS];
  RenderMethod         m_renderMethod;
  DXVA::CProcessorHD  *m_processor;
  std::vector<ERenderFormat> m_formats;

  // software scale libraries (fallback if required pixel shaders version is not available)
  struct SwsContext   *m_sw_scale_ctx;
  SHADER_SAMPLER       m_TextureFilter;
  bool                 m_bUseHQScaler;
  CD3DTexture          m_IntermediateTarget;
  CYUV2RGBShader*      m_colorShader;
  CConvolutionShader*  m_scalerShader;

  ESCALINGMETHOD       m_scalingMethod;
  ESCALINGMETHOD       m_scalingMethodGui;

  bool                 m_bFilterInitialized;
  int                  m_iRequestedMethod;
  unsigned int         m_extended_format;

  // Width and height of the render target
  // the separable HQ scalers need this info, but could the m_destRect be used instead?
  unsigned int         m_destWidth;
  unsigned int         m_destHeight;

  int                  m_neededBuffers;
  unsigned int         m_frameIdx;
  CRenderCapture*      m_capture = nullptr;
};

#else
#include "LinuxRenderer.h"
#endif


