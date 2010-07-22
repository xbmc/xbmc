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

#ifdef HAS_DX

#include "WinRenderer.h"
#include "Application.h"
#include "Util.h"
#include "Settings.h"
#include "GUISettings.h"
#include "Texture.h"
#include "WindowingFactory.h"
#include "AdvancedSettings.h"
#include "SingleLock.h"
#include "utils/log.h"
#include "FileSystem/File.h"
#include "MathUtils.h"
#include "cores/dvdplayer/DVDCodecs/Video/DXVA.h"
#include "VideoShaders/WinVideoFilter.h"
#include "../dvdplayer/Codecs/DllSwScale.h"
#include "../dvdplayer/Codecs/DllAvCodec.h"
#include "LocalizeStrings.h"

typedef struct {
  RenderMethod  method;
  const char   *name;
} RenderMethodDetail;

static RenderMethodDetail RenderMethodDetails[] = {
    { RENDER_SW     , "Software" },
    { RENDER_PS     , "Pixel Shaders" },
    { RENDER_DXVA   , "DXVA" },
    { RENDER_INVALID, NULL }
};

static RenderMethodDetail *FindRenderMethod(RenderMethod m)
{
  for (unsigned i = 0; RenderMethodDetails[i].method != RENDER_INVALID; i++) {
    if (RenderMethodDetails[i].method == m)
      return &RenderMethodDetails[i];
  }
  return NULL;
}

CWinRenderer::CWinRenderer()
{
  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;

  m_colorShader = NULL;
  m_scalerShader = NULL;

  m_renderMethod = RENDER_PS;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;
  m_StretchRectFilter = D3DTEXF_POINT;

  m_bUseHQScaler = false;
  m_bFilterInitialized = false;

  for (int i = 0; i<NUM_BUFFERS; i++)
    m_VideoBuffers[i] = NULL;

  m_sw_scale_ctx = NULL;
  // All three load together
  m_dllAvUtil = NULL;
  m_dllAvCodec = NULL;
  m_dllSwScale = NULL;
}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}

void CWinRenderer::ManageTextures()
{
  int neededbuffers = 2;

  if( m_NumYV12Buffers < neededbuffers )
  {
    for(int i = m_NumYV12Buffers; i<neededbuffers;i++)
      CreateYV12Texture(i);

    m_NumYV12Buffers = neededbuffers;
  }
  else if( m_NumYV12Buffers > neededbuffers )
  {
    m_NumYV12Buffers = neededbuffers;
    m_iYV12RenderBuffer = m_iYV12RenderBuffer % m_NumYV12Buffers;

    for(int i = m_NumYV12Buffers-1; i>=neededbuffers;i--)
      DeleteYV12Texture(i);
  }
}

void CWinRenderer::SelectRenderMethod()
{
  if (CONF_FLAGS_FORMAT_MASK(m_flags) == CONF_FLAGS_FORMAT_DXVA)
  {
    m_renderMethod = RENDER_DXVA;
  }
  else
  {
    int requestedMethod = g_guiSettings.GetInt("videoplayer.rendermethod");
    CLog::Log(LOGDEBUG, __FUNCTION__": Requested render method: %d", requestedMethod);

    switch(requestedMethod)
    {
      case RENDER_METHOD_AUTO:
      case RENDER_METHOD_D3D_PS:
        // Try the pixel shaders support
        if (m_deviceCaps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
        {
          CTestShader* shader = new CTestShader;
          if (shader->Create())
          {
            m_renderMethod = RENDER_PS;
            shader->Release();
            break;
          }
          else
          {
            CLog::Log(LOGNOTICE, "D3D: unable to load test shader - D3D setup is most likely incomplete");
            g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Warning, "DirectX", g_localizeStrings.Get(2101));
            shader->Release();
          }
        }
        else
        {
          CLog::Log(LOGNOTICE, "D3D: graphics adapter does not support Pixel Shaders 2.0");
        }
        CLog::Log(LOGNOTICE, "D3D: falling back to SW mode");
      // drop through to software
      case RENDER_METHOD_SOFTWARE:
      default:
        m_renderMethod = RENDER_SW;
        break;
    }
  }
  RenderMethodDetail *rmdet = FindRenderMethod(m_renderMethod);
  CLog::Log(LOGDEBUG, __FUNCTION__": Selected render method %d: %s", m_renderMethod, rmdet != NULL ? rmdet->name : "unknown");
}

bool CWinRenderer::UpdateRenderMethod()
{
  if (m_SWTarget.Get())
    m_SWTarget.Release();

  if (m_renderMethod == RENDER_SW)
  {
    m_dllAvUtil  = new DllAvUtil();
    m_dllAvCodec = new DllAvCodec();
    m_dllSwScale = new DllSwScale();

    if (!m_dllAvUtil->Load() || !m_dllAvCodec->Load() || !m_dllSwScale->Load())
      CLog::Log(LOGERROR,"CDVDDemuxFFmpeg::Open - failed to load ffmpeg libraries");

    m_dllSwScale->sws_rgb2rgb_init(SWS_CPU_CAPS_MMX2);

    if(!m_SWTarget.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGNOTICE, __FUNCTION__": Failed to create sw render target.");
      return false;
    }
  }
  return true;
}

bool CWinRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags)
{
  if(m_sourceWidth  != width
  || m_sourceHeight != height)
  {
    m_sourceWidth       = width;
    m_sourceHeight      = height;
    // need to recreate textures
    m_NumYV12Buffers    = 0;
    m_iYV12RenderBuffer = 0;
  }

  m_flags = flags;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  m_bConfigured = true;

  SelectRenderMethod();
  UpdateRenderMethod();

  return true;
}

int CWinRenderer::NextYV12Texture()
{
  if(m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  else
    return -1;
}

void CWinRenderer::AddProcessor(DXVA::CProcessor* processor, int64_t id)
{
  int source = NextYV12Texture();
  if(source < 0)
    return;
  DXVABuffer *buf = (DXVABuffer*)m_VideoBuffers[source];
  SAFE_RELEASE(buf->proc);
  buf->proc = processor->Acquire();
  buf->id   = id;
}

int CWinRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if( source < 0 )
    return -1;

  YUVBuffer *buf = (YUVBuffer*)m_VideoBuffers[source];

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_sourceHeight;
  image->width = m_sourceWidth;
  image->flags = 0;

  for(int i=0;i<3;i++)
  {
    image->stride[i] = buf->planes[i].rect.Pitch;
    image->plane[i]  = (BYTE*)buf->planes[i].rect.pBits;
  }

  return source;
}

void CWinRenderer::ReleaseImage(int source, bool preserve)
{
  // no need to release anything here since we're using system memory
}

void CWinRenderer::Reset()
{
}

void CWinRenderer::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

void CWinRenderer::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if (clear)
    pD3DDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, m_clearColour, 1.0f, 0L );

  if(alpha < 255)
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
  else
    pD3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

  if (!m_bConfigured) return;
  ManageTextures();

  CSingleLock lock(g_graphicsContext);

  ManageDisplay();

  Render(flags);
}

void CWinRenderer::FlipPage(int source)
{
  if(source == AUTOSOURCE)
    source = NextYV12Texture();

  if (m_VideoBuffers[m_iYV12RenderBuffer] != NULL)
    m_VideoBuffers[m_iYV12RenderBuffer]->StartDecode();

  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = 0;

  if (m_VideoBuffers[m_iYV12RenderBuffer] != NULL)
    m_VideoBuffers[m_iYV12RenderBuffer]->StartRender();

#ifdef MP_DIRECTRENDERING
  __asm wbinvd
#endif

  return;
}


unsigned int CWinRenderer::DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y)
{
  /*
  BYTE *s;
  BYTE *d;
  int i, p;

  int index = NextYV12Texture();
  if( index < 0 )
    return -1;

  D3DLOCKED_RECT rect;
  RECT target;

  target.left = x;
  target.right = x+w;
  target.top = y;
  target.bottom = y+h;

  YUVPLANES &planes = m_YUVTexture[index];

  // copy Y
  p = 0;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  w >>= 1; h >>= 1;
  x >>= 1; y >>= 1;
  target.top>>=1;
  target.bottom>>=1;
  target.left>>=1;
  target.right>>=1;

  // copy U
  p = 1;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);

  // copy V
  p = 2;
  planes[p]->LockRect(0, &rect, &target, D3DLOCK_DISCARD);
  d = (BYTE*)rect.pBits;
  s = src[p];
  for (i = 0;i < h;i++)
  {
    memcpy(d, s, w);
    s += stride[p];
    d += rect.Pitch;
  }
  planes[p]->UnlockRect(0);
  */

  return 0;
}

unsigned int CWinRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_resolution = RES_PAL_4x3;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  g_Windowing.Get3DDevice()->GetDeviceCaps(&m_deviceCaps);

  return 0;
}

void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  if (m_SWTarget.Get())
    m_SWTarget.Release();

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
  if (m_IntermediateStencilSurface.Get())
    m_IntermediateStencilSurface.Release();

  SAFE_RELEASE(m_colorShader)
  SAFE_RELEASE(m_scalerShader)
  
  m_bConfigured = false;
  m_bFilterInitialized = false;

  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);

  m_NumYV12Buffers = 0;

  if (m_sw_scale_ctx)
  {
    m_dllSwScale->sws_freeContext(m_sw_scale_ctx);
    m_sw_scale_ctx = NULL;
  }
  SAFE_DELETE(m_dllSwScale);
  SAFE_DELETE(m_dllAvCodec);
  SAFE_DELETE(m_dllAvUtil);
}

bool CWinRenderer::CreateIntermediateRenderTarget()
{
  // Initialize a render target for intermediate rendering - same size as the video source
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if(!m_IntermediateTarget.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2R10G10B10, D3DPOOL_DEFAULT))
  {
    CLog::Log(LOGNOTICE, __FUNCTION__": Failed to create 10 bit render target.  Trying 8 bit...");
    if(!m_IntermediateTarget.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Failed to create render target texture. Going back to bilinear scaling.");
      return false;
    }
  }

  //Pixel shaders need a matching depth-stencil surface.
  LPDIRECT3DSURFACE9 tmpSurface;
  D3DSURFACE_DESC tmpDesc;
  //Use the same depth stencil format as the backbuffer.
  pD3DDevice->GetDepthStencilSurface(&tmpSurface);
  tmpSurface->GetDesc(&tmpDesc);
  tmpSurface->Release();
  if (!m_IntermediateStencilSurface.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_DEPTHSTENCIL, tmpDesc.Format, D3DPOOL_DEFAULT))
  {
    CLog::Log(LOGERROR, __FUNCTION__": Failed to create depth stencil. Going back to bilinear scaling.");
    m_IntermediateTarget.Release();
    return false;
  }
  return true;
}

void CWinRenderer::SelectSWVideoFilter()
{
  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_AUTO:
  case VS_SCALINGMETHOD_LINEAR:
    if (Supports(VS_SCALINGMETHOD_LINEAR))
    {
      m_StretchRectFilter = D3DTEXF_LINEAR;
      break;
    }
    // fall through for fallback
  case VS_SCALINGMETHOD_NEAREST:
  default:
    m_StretchRectFilter = D3DTEXF_POINT;
    break;
  }
}

void CWinRenderer::SelectPSVideoFilter()
{
  m_bUseHQScaler = false;

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_NEAREST:
  case VS_SCALINGMETHOD_LINEAR:
    break;

  case VS_SCALINGMETHOD_CUBIC:
  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
    m_bUseHQScaler = true;
    break;

  case VS_SCALINGMETHOD_LANCZOS3:
    m_bUseHQScaler = true;
    break;

  case VS_SCALINGMETHOD_SINC8:
  case VS_SCALINGMETHOD_NEDI:
    CLog::Log(LOGERROR, "D3D: TODO: This scaler has not yet been implemented");
    break;

  case VS_SCALINGMETHOD_BICUBIC_SOFTWARE:
  case VS_SCALINGMETHOD_LANCZOS_SOFTWARE:
  case VS_SCALINGMETHOD_SINC_SOFTWARE:
    CLog::Log(LOGERROR, "D3D: TODO: Software scaling has not yet been implemented");
    break;

  default:
    break;
  }

  // Scaler auto + SD -> Lanczos3 optim. Otherwise bilinear.
  if(m_scalingMethod == VS_SCALINGMETHOD_AUTO && m_sourceWidth < 1280)
  {
    m_scalingMethod = VS_SCALINGMETHOD_LANCZOS3_FAST;
    m_bUseHQScaler = true;
  }
}

void CWinRenderer::UpdatePSVideoFilter()
{
  SAFE_RELEASE(m_scalerShader)

  if (m_bUseHQScaler)
  {
    m_scalerShader = new CConvolutionShader();
    if (!m_scalerShader->Create(m_scalingMethod))
    {
      SAFE_RELEASE(m_scalerShader);
      g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::Error, "Video Renderering", "Failed to init video scaler, falling back to bilinear scaling.");
      m_bUseHQScaler = false;
    }
  }

  if(m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();
  if (m_IntermediateStencilSurface.Get())
    m_IntermediateStencilSurface.Release();

  if (m_bUseHQScaler && !CreateIntermediateRenderTarget())
  {
    SAFE_RELEASE(m_scalerShader)
    m_bUseHQScaler = true;
  }

  SAFE_RELEASE(m_colorShader)

  if (m_bUseHQScaler)
  {
    m_colorShader = new CYUV2RGBShader();
    if (!m_colorShader->Create(false))
    {
      m_IntermediateTarget.Release();
      m_IntermediateStencilSurface.Release();
      SAFE_RELEASE(m_scalerShader)
      SAFE_RELEASE(m_colorShader);
      m_bUseHQScaler = false;
    }
  }

  if (!m_bUseHQScaler) //fallback from HQ scalers and multipass creation above
  {
    m_colorShader = new CYUV2RGBShader();
    if (!m_colorShader->Create(true))
      SAFE_RELEASE(m_colorShader);
    // we're in big trouble - should fallback on D3D accelerated or sw method
  }
}

void CWinRenderer::UpdateVideoFilter()
{
  if (m_scalingMethodGui == g_settings.m_currentVideoSettings.m_ScalingMethod && m_bFilterInitialized)
    return;

  m_bFilterInitialized = true;

  m_scalingMethodGui = g_settings.m_currentVideoSettings.m_ScalingMethod;
  m_scalingMethod    = m_scalingMethodGui;

  if (!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - chosen scaling method %d is not supported by renderer", (int)m_scalingMethod);
    m_scalingMethod = VS_SCALINGMETHOD_AUTO;
  }

  switch(m_renderMethod)
  {
  case RENDER_SW:
    SelectSWVideoFilter();
    break;

  case RENDER_PS:
    SelectPSVideoFilter();
    UpdatePSVideoFilter();
    break;

  case RENDER_DXVA:
    // Everything already setup, nothing to do.
    break;

  default:
    return;
  }
}

void CWinRenderer::CropSource(RECT& src, RECT& dst, const D3DSURFACE_DESC& desc)
{
  if(dst.left < 0)
  {
    src.left -= dst.left
              * (src.right - src.left)
              / (dst.right - dst.left);
    dst.left  = 0;
  }
  if(dst.top < 0)
  {
    src.top -= dst.top
             * (src.bottom - src.top)
             / (dst.bottom - dst.top);
    dst.top  = 0;
  }
  if(dst.right > (LONG)desc.Width)
  {
    src.right -= (dst.right - desc.Width)
               * (src.right - src.left)
               / (dst.right - dst.left);
    dst.right  = desc.Width;
  }
  if(dst.bottom > (LONG)desc.Height)
  {
    src.bottom -= (dst.bottom - desc.Height)
                * (src.bottom - src.top)
                / (dst.bottom - dst.top);
    dst.bottom  = desc.Height;
  }
}

void CWinRenderer::Render(DWORD flags)
{
  if(CONF_FLAGS_FORMAT_MASK(m_flags) == CONF_FLAGS_FORMAT_DXVA)
  {
    CWinRenderer::RenderProcessor(flags);
    return;
  }

  UpdateVideoFilter();

  // Optimize later? we could get by with bilinear under some circumstances
  /*if(!m_bUseHQScaler
    || !g_graphicsContext.IsFullScreenVideo()
    || g_graphicsContext.IsCalibrating()
    || (m_destRect.Width() == m_sourceWidth && m_destRect.Height() == m_sourceHeight))
    */
  CSingleLock lock(g_graphicsContext);

  // set scissors if we are not in fullscreen video
  if ( !(g_graphicsContext.IsFullScreenVideo() || g_graphicsContext.IsCalibrating() ))
    g_graphicsContext.ClipToViewWindow();

  if (m_renderMethod == RENDER_SW)
    RenderSW(flags);
  else if (m_renderMethod == RENDER_PS)
    RenderPS(flags);
}

void CWinRenderer::RenderSW(DWORD flags)
{
  // 1. convert yuv to rgb
  m_sw_scale_ctx = m_dllSwScale->sws_getCachedContext(m_sw_scale_ctx,
                                                      m_sourceWidth, m_sourceHeight, PIX_FMT_YUV420P,
                                                      m_sourceWidth, m_sourceHeight, PIX_FMT_BGRA,
                                                      SWS_FAST_BILINEAR, NULL, NULL, NULL);

  YUVBuffer* buf = (YUVBuffer*)m_VideoBuffers[m_iYV12RenderBuffer];

  D3DLOCKED_RECT srclr[3];
  if(!(buf->planes[0].texture.LockRect(0, &srclr[0], NULL, D3DLOCK_READONLY))
  || !(buf->planes[1].texture.LockRect(0, &srclr[1], NULL, D3DLOCK_READONLY))
  || !(buf->planes[2].texture.LockRect(0, &srclr[2], NULL, D3DLOCK_READONLY)))
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock yuv textures into memory");
  
  D3DLOCKED_RECT destlr = {0,0};
  if (!m_SWTarget.LockRect(0, &destlr, NULL, D3DLOCK_DISCARD))
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock swtarget texture into memory");

  uint8_t *src[]  = { (uint8_t*)srclr[0].pBits, (uint8_t*)srclr[1].pBits, (uint8_t*)srclr[2].pBits, 0 };
  int srcStride[] = { srclr[0].Pitch, srclr[1].Pitch, srclr[2].Pitch, 0 };
  uint8_t *dst[]  = { (uint8_t*) destlr.pBits, 0, 0, 0 };
  int dstStride[] = { destlr.Pitch, 0, 0, 0 };

  m_dllSwScale->sws_scale(m_sw_scale_ctx, src, srcStride, 0, m_sourceHeight, dst, dstStride);

  buf->planes[0].texture.UnlockRect(0);
  buf->planes[1].texture.UnlockRect(0);
  buf->planes[2].texture.UnlockRect(0);

  m_SWTarget.UnlockRect(0);

  // 2. scale to display

  // Don't know where this martian comes from but it can happen in the initial frames of a video
  if ((m_destRect.x1 < 0 && m_destRect.x2 < 0) || (m_destRect.y1 < 0 && m_destRect.y2 < 0))
    return;

  RECT srcRect = { m_sourceRect.x1, m_sourceRect.y1, m_sourceRect.x2, m_sourceRect.y2 };
  IDirect3DSurface9* source;
  if(!m_SWTarget.GetSurfaceLevel(0, &source))
    CLog::Log(LOGERROR, "CWinRenderer::Render - failed to get source");

  RECT dstRect = { m_destRect.x1, m_destRect.y1, m_destRect.x2, m_destRect.y2 };
  IDirect3DSurface9* target;
  if(FAILED(g_Windowing.Get3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &target)))
    CLog::Log(LOGERROR, "CWinRenderer::Render - failed to get back buffer");

  D3DSURFACE_DESC desc;
  if (FAILED(target->GetDesc(&desc)))
    CLog::Log(LOGERROR, "CWinRenderer::Render - failed to get back buffer description");

  // Need to manipulate the coordinates since StretchRect doesn't accept off-screen coordinates.
  CropSource(srcRect, dstRect, desc);

  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if(FAILED(pD3DDevice->StretchRect(source, &srcRect, target, &dstRect, m_StretchRectFilter)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - unable to StretchRect");
    if (!(m_deviceCaps.Caps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES))
      CLog::Log(LOGDEBUG, __FUNCTION__" - missing D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES");
  }

  target->Release();
  source->Release();
}

void CWinRenderer::RenderPS(DWORD flags)
{
  if (!m_bUseHQScaler)
  {
    Stage1(flags);
  }
  else
  {
    Stage1(flags);
    Stage2(flags);
  }
}

void CWinRenderer::Stage1(DWORD flags)
{
  if (!m_bUseHQScaler)
  {
      m_colorShader->Render(m_sourceWidth, m_sourceHeight, m_sourceRect, m_destRect,
                              g_settings.m_currentVideoSettings.m_Contrast,
                              g_settings.m_currentVideoSettings.m_Brightness,
                              m_flags,
                              (YUVBuffer*)m_VideoBuffers[m_iYV12RenderBuffer]);
  }
  else
  {
    // Switch the render target to the temporary destination
    LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
    LPDIRECT3DSURFACE9 newRT, oldRT, oldDS, newDS;
    m_IntermediateTarget.GetSurfaceLevel(0, &newRT);
    m_IntermediateStencilSurface.GetSurfaceLevel(0, &newDS);
    pD3DDevice->GetRenderTarget(0, &oldRT);
    pD3DDevice->SetRenderTarget(0, newRT);
    pD3DDevice->GetDepthStencilSurface(&oldDS);
    pD3DDevice->SetDepthStencilSurface(newDS);

    CRect rtRect(0.0f, 0.0f, m_sourceWidth, m_sourceHeight);

    m_colorShader->Render(m_sourceWidth, m_sourceHeight, m_sourceRect, rtRect,
                                g_settings.m_currentVideoSettings.m_Contrast,
                                g_settings.m_currentVideoSettings.m_Brightness,
                                m_flags,
                                (YUVBuffer*)m_VideoBuffers[m_iYV12RenderBuffer]);

    // Restore the render target
    pD3DDevice->SetRenderTarget(0, oldRT);
    pD3DDevice->SetDepthStencilSurface(oldDS);

    oldDS->Release();
    oldRT->Release();
    newDS->Release();
    newRT->Release();
  }
}

void CWinRenderer::Stage2(DWORD flags)
{
  m_scalerShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_sourceRect, m_destRect);
}

void CWinRenderer::RenderProcessor(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);
  RECT rect;
  rect.top    = m_destRect.y1;
  rect.bottom = m_destRect.y2;
  rect.left   = m_destRect.x1;
  rect.right  = m_destRect.x2;

  DXVABuffer *image = (DXVABuffer*)m_VideoBuffers[m_iYV12RenderBuffer];
  if(image->proc == NULL)
    return;

  IDirect3DSurface9* target;
  if(FAILED(g_Windowing.Get3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &target)))
  {
    CLog::Log(LOGERROR, "CWinRenderer::RenderSurface - failed to get back buffer");
    return;
  }

  image->proc->Render(rect, target, image->id);

  target->Release();
}

void CWinRenderer::CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height)
{
  CSingleLock lock(g_graphicsContext);

  // create a new render surface to copy out of - note, this may be slow on some hardware
  // due to the TRUE parameter - you're supposed to use GetRenderTargetData.
  LPDIRECT3DSURFACE9 surface = NULL;
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  if (D3D_OK == pD3DDevice->CreateRenderTarget(width, height, D3DFMT_LIN_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL))
  {
    LPDIRECT3DSURFACE9 oldRT;
    CRect saveSize = m_destRect;
    m_destRect.SetRect(0, 0, (float)width, (float)height);
    pD3DDevice->GetRenderTarget(0, &oldRT);
    pD3DDevice->SetRenderTarget(0, surface);
    pD3DDevice->BeginScene();
    Render(0);
    pD3DDevice->EndScene();
    m_destRect = saveSize;
    pD3DDevice->SetRenderTarget(0, oldRT);
    oldRT->Release();

    D3DLOCKED_RECT lockedRect;
    if (D3D_OK == surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY))
    {
      texture->LoadFromMemory(width, height, lockedRect.Pitch, XB_FMT_A8R8G8B8, (unsigned char *)lockedRect.pBits);
      surface->UnlockRect();
    }
    surface->Release();
  }
}


//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CWinRenderer::DeleteYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  if (m_VideoBuffers[index] != NULL)
    SAFE_DELETE(m_VideoBuffers[index]);
  m_NumYV12Buffers = 0;
}

bool CWinRenderer::CreateYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  DeleteYV12Texture(index);

  if (m_renderMethod == RENDER_DXVA)
  {
    m_VideoBuffers[index] = new DXVABuffer();
  }
  else
  {
    YUVBuffer *buf = new YUVBuffer();

    BufferMemoryType memoryType;

    if (m_renderMethod == RENDER_SW)
      memoryType = SystemMemory; // Need the textures in system memory for quick read and write
    else
      memoryType = DontCare;

    if (!buf->Create(memoryType, m_sourceWidth, m_sourceHeight))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Unable to create YV12 video texture %i", index);
      return false;
    }
    m_VideoBuffers[index] = buf;
  }

  SVideoBuffer *buf = m_VideoBuffers[index];

  buf->StartDecode();
  buf->Clear();

  if(index == m_iYV12RenderBuffer)
    buf->StartRender();

  CLog::Log(LOGDEBUG, "created video buffer %i", index);
  return true;
}

bool CWinRenderer::Supports(EINTERLACEMETHOD method)
{
  if(CONF_FLAGS_FORMAT_MASK(m_flags) == CONF_FLAGS_FORMAT_DXVA)
  {
    if(method == VS_INTERLACEMETHOD_NONE)
      return true;
    return false;
  }

  if(method == VS_INTERLACEMETHOD_NONE
  || method == VS_INTERLACEMETHOD_AUTO
  || method == VS_INTERLACEMETHOD_DEINTERLACE)
    return true;

  return false;
}

bool CWinRenderer::Supports(ERENDERFEATURE feature)
{
  if (m_renderMethod == RENDER_DXVA || m_renderMethod == RENDER_PS)
  {
    if(feature == RENDERFEATURE_BRIGHTNESS)
      return true;

    if(feature == RENDERFEATURE_CONTRAST)
      return true;
  }
  return false;
}

bool CWinRenderer::Supports(ESCALINGMETHOD method)
{
  if (m_renderMethod == RENDER_DXVA)
  {
    if(method == VS_SCALINGMETHOD_DXVA_HARDWARE)
      return true;
    return false;
  }
  else if(m_renderMethod == RENDER_PS)
  {
    if(m_deviceCaps.PixelShaderVersion >= D3DPS_VERSION(2, 0)
    && (   method == VS_SCALINGMETHOD_AUTO
        || method == VS_SCALINGMETHOD_LINEAR))
        return true;

    if(m_deviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
    {
      if(method == VS_SCALINGMETHOD_CUBIC
      || method == VS_SCALINGMETHOD_LANCZOS2
      || method == VS_SCALINGMETHOD_LANCZOS3_FAST)
        return true;

      //lanczos3 is only allowed through advancedsettings.xml because it's very slow
      if (g_advancedSettings.m_videoAllowLanczos3 && method == VS_SCALINGMETHOD_LANCZOS3)
        return true;
    }
  }
  else if(m_renderMethod == RENDER_SW)
  {
    if(method == VS_SCALINGMETHOD_NEAREST
    || method == VS_SCALINGMETHOD_AUTO )
      return true;

    if(method == VS_SCALINGMETHOD_LINEAR
    && m_deviceCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR
	  && m_deviceCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
	    return true;
  }
  return false;
}

//============================================

bool YUVBuffer::Create(BufferMemoryType memoryType, unsigned int width, unsigned int height)
{
  m_memoryType = memoryType;
  m_width = width;
  m_height = height;

  D3DPOOL pool;
  DWORD   usage;

  switch(m_memoryType)
  {
  case DontCare:
    pool  = g_Windowing.DefaultD3DPool();
    usage = g_Windowing.DefaultD3DUsage();
    break;

  case SystemMemory:
    pool  = D3DPOOL_SYSTEMMEM;
    usage = 0;
    break;

  case VideoMemory:
    pool  = D3DPOOL_DEFAULT;
    usage = D3DUSAGE_DYNAMIC;
    break;

  default:
    CLog::Log(LOGERROR, __FUNCTION__" - unknown memory type %d", m_memoryType);
    return false;
  }

  if ( !planes[PLANE_Y].texture.Create(m_width    , m_height    , 1, usage, D3DFMT_L8, pool)
    || !planes[PLANE_U].texture.Create(m_width / 2, m_height / 2, 1, usage, D3DFMT_L8, pool)
    || !planes[PLANE_V].texture.Create(m_width / 2, m_height / 2, 1, usage, D3DFMT_L8, pool))
    return false;

  return true;
}

void YUVBuffer::Release()
{
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    planes[i].texture.Release();
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void YUVBuffer::StartRender()
{
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if(planes[i].texture.Get() && planes[i].rect.pBits)
      planes[i].texture.UnlockRect(0);
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void YUVBuffer::StartDecode()
{
  for(unsigned i = 0; i < MAX_PLANES; i++)
  {
    if(planes[i].texture.Get()
    && planes[i].texture.LockRect(0, &planes[i].rect, NULL, D3DLOCK_DISCARD) == false)
    {
      memset(&planes[i].rect, 0, sizeof(planes[i].rect));
      CLog::Log(LOGERROR, "CWinRenderer::SVideoBuffer::StartDecode - failed to lock texture into memory");
    }
  }
}

void YUVBuffer::Clear()
{
    memset(planes[PLANE_Y].rect.pBits, 0,   planes[0].rect.Pitch * m_height);
    memset(planes[PLANE_U].rect.pBits, 128, planes[1].rect.Pitch * m_height>>1);
    memset(planes[PLANE_V].rect.pBits, 128, planes[2].rect.Pitch * m_height>>1);
}

//==================================

void DXVABuffer::Release()
{
  SAFE_RELEASE(proc);
  id = 0;
}

void DXVABuffer::StartDecode()
{
  Release();
}
#endif
