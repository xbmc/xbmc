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

#ifdef HAS_DX

#include "WinRenderer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/FFmpeg.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/CPUInfo.h"
#include "utils/log.h"
#include "utils/win32/gpu_memcpy_sse4.h"
#include "VideoShaders/WinVideoFilter.h"
#include "platform/win32/WIN32Util.h"
#include "windowing/WindowingFactory.h"

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
  return nullptr;
}

CWinRenderer::CWinRenderer()
{
  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;

  m_colorShader = nullptr;
  m_scalerShader = nullptr;
  m_extended_format = 0;

  m_iRequestedMethod = RENDER_METHOD_AUTO;

  m_renderMethod = RENDER_PS;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;
  m_TextureFilter = SHADER_SAMPLER_POINT;

  m_bUseHQScaler = false;
  m_bFilterInitialized = false;

  for (int i = 0; i<NUM_BUFFERS; i++)
    m_VideoBuffers[i] = nullptr;

  m_sw_scale_ctx = nullptr;
  m_destWidth = 0;
  m_destHeight = 0;
  m_bConfigured = false;
  m_format = RENDER_FMT_NONE;
  m_processor = nullptr;
  m_neededBuffers = 0;
}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}

static enum AVPixelFormat PixelFormatFromFormat(ERenderFormat format)
{
  if (format == RENDER_FMT_DXVA)      return AV_PIX_FMT_NV12;
  if (format == RENDER_FMT_YUV420P)   return AV_PIX_FMT_YUV420P;
  if (format == RENDER_FMT_YUV420P10) return AV_PIX_FMT_YUV420P10;
  if (format == RENDER_FMT_YUV420P16) return AV_PIX_FMT_YUV420P16;
  if (format == RENDER_FMT_NV12)      return AV_PIX_FMT_NV12;
  if (format == RENDER_FMT_UYVY422)   return AV_PIX_FMT_UYVY422;
  if (format == RENDER_FMT_YUYV422)   return AV_PIX_FMT_YUYV422;
  return AV_PIX_FMT_NONE;
}

bool CWinRenderer::HandlesRenderFormat(ERenderFormat format)
{
  if(format == RENDER_FMT_DXVA
  || format == RENDER_FMT_YUV420P
  || format == RENDER_FMT_YUV420P10
  || format == RENDER_FMT_YUV420P16
  || format == RENDER_FMT_NV12
  || format == RENDER_FMT_UYVY422
  || format == RENDER_FMT_YUYV422)
  {
    return true;
  }
  return false;
}

void CWinRenderer::ManageTextures()
{
  if( m_NumYV12Buffers < m_neededBuffers )
  {
    for(int i = m_NumYV12Buffers; i<m_neededBuffers;i++)
      CreateYV12Texture(i);

    m_NumYV12Buffers = m_neededBuffers;
  }
  else if( m_NumYV12Buffers > m_neededBuffers )
  {
    for (int i = m_NumYV12Buffers - 1; i >= m_neededBuffers; i--)
      DeleteYV12Texture(i);

    m_NumYV12Buffers = m_neededBuffers;
    m_iYV12RenderBuffer = m_iYV12RenderBuffer % m_NumYV12Buffers;
  }
}

void CWinRenderer::SelectRenderMethod()
{
  // Set rendering to dxva before trying it, in order to open the correct processor immediately, when deinterlacing method is auto.

  // Force dxva renderer after dxva decoding: PS and SW renderers have performance issues after dxva decode.
  if (g_advancedSettings.m_DXVAForceProcessorRenderer && m_format == RENDER_FMT_DXVA)
  {
    CLog::Log(LOGNOTICE, "D3D: rendering method forced to DXVA processor");
    m_renderMethod = RENDER_DXVA;
    if (!m_processor || !m_processor->Open(m_sourceWidth, m_sourceHeight, m_iFlags, m_format, m_extended_format))
    {
      CLog::Log(LOGNOTICE, "D3D: unable to open DXVA processor");
      if (m_processor)  
        m_processor->Close();
      m_renderMethod = RENDER_INVALID;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Requested render method: %d", m_iRequestedMethod);

    switch(m_iRequestedMethod)
    {
      case RENDER_METHOD_DXVA:
        if (!m_processor || !m_processor->Open(m_sourceWidth, m_sourceHeight, m_iFlags, m_format, m_extended_format))
        {
          CLog::Log(LOGNOTICE, "D3D: unable to open DXVA processor");
          if (m_processor)
            m_processor->Close();
        }
        else
        {
          m_renderMethod = RENDER_DXVA;
          break;
        }
      // Drop through to pixel shader
      case RENDER_METHOD_AUTO:
      case RENDER_METHOD_D3D_PS:
        {
          CTestShader shader;
          if (shader.Create())
          {
            m_renderMethod = RENDER_PS;
            if (m_format == RENDER_FMT_DXVA)
              m_format = RENDER_FMT_NV12;
            break;
          }
          else
          {
            CLog::Log(LOGNOTICE, "D3D: unable to load test shader - D3D installation is most likely incomplete");
            CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "DirectX", g_localizeStrings.Get(2101));
          }
          CLog::Log(LOGNOTICE, "D3D: falling back to SW mode");
        }
      // drop through to software
      case RENDER_METHOD_SOFTWARE:
      default:
        // So we'll do the color conversion in software.
        m_renderMethod = RENDER_SW;
        break;
    }
  }

  RenderMethodDetail *rmdet = FindRenderMethod(m_renderMethod);
  CLog::Log(LOGDEBUG, __FUNCTION__": Selected render method %d: %s", m_renderMethod, rmdet != nullptr ? rmdet->name : "unknown");
  m_frameIdx = 0;
}

bool CWinRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  m_sourceWidth       = width;
  m_sourceHeight      = height;
  m_renderOrientation = orientation;
  // need to recreate textures
  m_NumYV12Buffers    = 0;
  m_iYV12RenderBuffer = 0;
  // reinitialize the filters/shaders
  m_bFilterInitialized = false;

  m_fps = fps;
  m_iFlags = flags;
  m_format = format;
  m_extended_format = extended_format;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  SetViewMode(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ViewMode);
  ManageRenderArea();

  SelectRenderMethod();
  m_bConfigured = true;

  return true;
}

int CWinRenderer::NextYV12Texture()
{
  if(m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  else
    return -1;
}

bool CWinRenderer::IsPictureHW(DVDVideoPicture &picture)
{
  if (m_renderMethod == RENDER_DXVA
    || picture.format == RENDER_FMT_DXVA)
  {
    return true;
  }
  return false;
}

void CWinRenderer::AddVideoPictureHW(DVDVideoPicture &picture, int index)
{
  if (m_renderMethod == RENDER_DXVA)
  {
    DXVABuffer *buf = reinterpret_cast<DXVABuffer*>(m_VideoBuffers[index]);
    SAFE_RELEASE(buf->pic);
    buf->pic = m_processor->Convert(picture);
    buf->frameIdx = m_frameIdx;
    m_frameIdx += 2;
  }
  else if (picture.format == RENDER_FMT_DXVA)
  {
    YUVBuffer *buf = reinterpret_cast<YUVBuffer*>(m_VideoBuffers[index]);
    if (buf->IsReadyToRender())
      return;
    buf->CopyFromPicture(picture);
  }
}

int CWinRenderer::GetImage(YV12Image *image, int source, bool readonly)
{
  if (!image) return -1;
  if (!m_NumYV12Buffers) return -1;

  /* take next available buffer */
  if( source == AUTOSOURCE )
    source = NextYV12Texture();

  if (source < 0 || NextYV12Texture() < 0)
    return -1;

  if (m_renderMethod == RENDER_DXVA)
    return source;

  YUVBuffer *buf = reinterpret_cast<YUVBuffer*>(m_VideoBuffers[source]);
  if (!buf)
    return -1;

  image->cshift_x = 1;
  image->cshift_y = 1;
  image->height = m_sourceHeight;
  image->width = m_sourceWidth;
  image->flags = 0;
  if(m_format == RENDER_FMT_YUV420P10
  || m_format == RENDER_FMT_YUV420P16)
    image->bpp = 2;
  else
    image->bpp = 1;

  for(int i=0;i<3;i++)
  {
    image->stride[i] = buf->planes[i].rect.RowPitch;
    image->plane[i]  = (BYTE*)buf->planes[i].rect.pData;
  }

  return source;
}

void CWinRenderer::ReleaseImage(int source, bool preserve)
{
}

void CWinRenderer::Reset()
{
}

void CWinRenderer::Update()
{
  if (!m_bConfigured) 
    return;
  ManageRenderArea();
  ManageTextures();
}

void CWinRenderer::RenderUpdate(bool clear, unsigned int flags, unsigned int alpha)
{
  if (clear)
    g_graphicsContext.Clear(g_Windowing.UseLimitedColor() ? 0x101010 : 0);

  if (!m_bConfigured)
    return;

  g_Windowing.SetAlphaBlendEnable(alpha < 255);
  ManageTextures();
  ManageRenderArea();
  Render(flags);
}

void CWinRenderer::FlipPage(int source)
{
  if (m_VideoBuffers[m_iYV12RenderBuffer] != nullptr)
    m_VideoBuffers[m_iYV12RenderBuffer]->StartDecode();

  if( source >= 0 && source < m_NumYV12Buffers )
    m_iYV12RenderBuffer = source;
  else
    m_iYV12RenderBuffer = NextYV12Texture();;

  if (m_VideoBuffers[m_iYV12RenderBuffer] != nullptr)
    m_VideoBuffers[m_iYV12RenderBuffer]->StartRender();

  return;
}

void CWinRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();

  m_formats.clear();
  m_formats.push_back(RENDER_FMT_YUV420P);

  m_iRequestedMethod = CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD);

  if (g_advancedSettings.m_DXVAForceProcessorRenderer
  ||  m_iRequestedMethod == RENDER_METHOD_DXVA)
  {
    m_processor = new DXVA::CProcessorHD();
    if (!m_processor->PreInit())
    {
      CLog::Log(LOGNOTICE, "CWinRenderer::Preinit - could not init DXVA processor - skipping");
      SAFE_DELETE(m_processor);
    }
    else
      m_processor->ApplySupportedFormats(&m_formats);
  }

  // allow other color spaces besides YV12 in case DXVA rendering is not used or not available
  if (!m_processor || (m_iRequestedMethod != RENDER_METHOD_DXVA))
  {
    if ( g_Windowing.IsFormatSupport(DXGI_FORMAT_R16_UNORM, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
      || g_Windowing.IsFormatSupport(DXGI_FORMAT_R16_UNORM, D3D11_FORMAT_SUPPORT_SHADER_LOAD))
    {
      m_formats.push_back(RENDER_FMT_YUV420P10);
      m_formats.push_back(RENDER_FMT_YUV420P16);
    }
    m_formats.push_back(RENDER_FMT_NV12);
    m_formats.push_back(RENDER_FMT_YUYV422);
    m_formats.push_back(RENDER_FMT_UYVY422);
  }
}

void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  SAFE_DELETE(m_colorShader);
  SAFE_DELETE(m_scalerShader);
  
  m_bConfigured = false;
  m_bFilterInitialized = false;

  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);

  m_NumYV12Buffers = 0;

  if (m_sw_scale_ctx)
  {
    sws_freeContext(m_sw_scale_ctx);
    m_sw_scale_ctx = nullptr;
  }

  if (m_processor)
  {
    m_processor->UnInit();
    SAFE_DELETE(m_processor);
  }
}

void CWinRenderer::Flush()
{
  if (!m_bConfigured)
    return;

  for (int i = 0; i < NUM_BUFFERS; i++)
    DeleteYV12Texture(i);

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;
  m_bFilterInitialized = false;
}

bool CWinRenderer::CreateIntermediateRenderTarget(unsigned int width, unsigned int height, bool dynamic)
{
  unsigned int usage = D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;

  DXGI_FORMAT format = DXGI_FORMAT_B8G8R8X8_UNORM;
  if      (m_renderMethod == RENDER_DXVA)                                   format = DXGI_FORMAT_B8G8R8X8_UNORM;
  else if (g_Windowing.IsFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, usage))  format = DXGI_FORMAT_B8G8R8A8_UNORM;
  else if (g_Windowing.IsFormatSupport(DXGI_FORMAT_B8G8R8X8_UNORM, usage))  format = DXGI_FORMAT_B8G8R8X8_UNORM;

  // don't create new one if it exists with requested size and format
  if ( m_IntermediateTarget.Get() && m_IntermediateTarget.GetFormat() == format
    && m_IntermediateTarget.GetWidth() == width && m_IntermediateTarget.GetHeight() == height)
    return true;

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  CLog::Log(LOGDEBUG, __FUNCTION__": format %i", format);

  if (!m_IntermediateTarget.Create(width, height, 1, dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT, format))
  {
    CLog::Log(LOGERROR, __FUNCTION__": intermediate render target creation failed.");
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
      m_TextureFilter = SHADER_SAMPLER_LINEAR;
      break;
    }
    // fall through for fallback
  case VS_SCALINGMETHOD_NEAREST:
  default:
    m_TextureFilter = SHADER_SAMPLER_POINT;
    break;
  }

  CreateIntermediateRenderTarget(m_sourceWidth, m_sourceHeight, true);
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
  case VS_SCALINGMETHOD_SPLINE36_FAST:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
    m_bUseHQScaler = true;
    break;

  case VS_SCALINGMETHOD_SPLINE36:
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

  if (m_scalingMethod == VS_SCALINGMETHOD_AUTO)
  {
    bool scaleSD = m_sourceHeight < 720 && m_sourceWidth < 1280;
    bool scaleUp = (int)m_sourceHeight < g_graphicsContext.GetHeight() && (int)m_sourceWidth < g_graphicsContext.GetWidth();
    bool scaleFps = m_fps < (g_advancedSettings.m_videoAutoScaleMaxFps + 0.01f);

    if (m_renderMethod == RENDER_DXVA)
    {
      m_scalingMethod = VS_SCALINGMETHOD_DXVA_HARDWARE;
      m_bUseHQScaler = false;
    }
    else if (scaleSD && scaleUp && scaleFps && Supports(VS_SCALINGMETHOD_LANCZOS3_FAST))
    {
      m_scalingMethod = VS_SCALINGMETHOD_LANCZOS3_FAST;
      m_bUseHQScaler = true;
    }
  }
  if (m_renderOrientation)
    m_bUseHQScaler = false;
}

void CWinRenderer::UpdatePSVideoFilter()
{
  RESOLUTION_INFO res = g_graphicsContext.GetResInfo();
  if (!res.bFullScreen)
    res = g_graphicsContext.GetResInfo(RES_DESKTOP);

  m_destWidth = res.iScreenWidth;
  m_destHeight = res.iScreenHeight;

  SAFE_DELETE(m_scalerShader);

  if (m_bUseHQScaler)
  {
    // First try the more efficient two pass convolution scaler
    m_scalerShader = new CConvolutionShaderSeparable();

    if (!m_scalerShader->Create(m_scalingMethod))
    {
      SAFE_DELETE(m_scalerShader);
      CLog::Log(LOGNOTICE, __FUNCTION__": two pass convolution shader init problem, falling back to one pass.");
    }

    // Fallback on the one pass version
    if (m_scalerShader == nullptr)
    {
      m_scalerShader = new CConvolutionShader1Pass();

      if (!m_scalerShader->Create(m_scalingMethod))
      {
        SAFE_DELETE(m_scalerShader);
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(34400), g_localizeStrings.Get(34401));
        m_bUseHQScaler = false;
      }
    }
  }

  if (m_bUseHQScaler && !CreateIntermediateRenderTarget(m_sourceWidth, m_sourceHeight, false))
  {
    SAFE_DELETE(m_scalerShader);
    m_bUseHQScaler = false;
  }

  SAFE_DELETE(m_colorShader);

  if (m_renderMethod == RENDER_DXVA)
  {
    // we'd use m_IntermediateTarget as rendering target for possible anaglyph stereo with dxva processor.
    if (!m_bUseHQScaler)
      CreateIntermediateRenderTarget(m_destWidth, m_destHeight, false);
    // When using DXVA, we are already setup at this point, color shader is not needed
    return;
  }

  m_colorShader = new CYUV2RGBShader();
  if (!m_colorShader->Create(m_sourceWidth, m_sourceHeight, m_format))
  {
    if (m_bUseHQScaler)
    {
      m_IntermediateTarget.Release();
      SAFE_DELETE(m_scalerShader);
    }
    SAFE_DELETE(m_colorShader);
    m_bUseHQScaler = false;

    // we're in big trouble - fallback to sw method
    m_renderMethod = RENDER_SW;
    if (m_NumYV12Buffers)
    {
      m_NumYV12Buffers = 0;
      ManageTextures();
    }
    SelectSWVideoFilter();
  }
}

void CWinRenderer::UpdateVideoFilter()
{
  if (m_scalingMethodGui == CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ScalingMethod && m_bFilterInitialized)
    return;

  m_bFilterInitialized = true;

  m_scalingMethodGui = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_ScalingMethod;
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
  case RENDER_DXVA:
    SelectPSVideoFilter();
    UpdatePSVideoFilter();
    break;

  default:
    return;
  }
}

void CWinRenderer::Render(DWORD flags)
{
  if (!m_VideoBuffers[m_iYV12RenderBuffer]->IsReadyToRender())
    return;

  UpdateVideoFilter();

  switch (m_renderMethod)
  {
  case RENDER_DXVA:
    RenderHW(flags);
    break;
  case RENDER_PS:
    RenderPS();
    break;
  case RENDER_SW:
    RenderSW();
    break;
  default:
    return;
  }

  if (m_bUseHQScaler)
    RenderHQ();
  g_Windowing.ApplyStateBlock();
}

void CWinRenderer::RenderSW()
{
  enum AVPixelFormat format = PixelFormatFromFormat(m_format);

  // 1. convert yuv to rgb
  m_sw_scale_ctx = sws_getCachedContext(m_sw_scale_ctx,
                                        m_sourceWidth, m_sourceHeight, format,
                                        m_sourceWidth, m_sourceHeight, AV_PIX_FMT_BGRA,
                                        SWS_FAST_BILINEAR, NULL, NULL, NULL);

  YUVBuffer* buf = reinterpret_cast<YUVBuffer*>(m_VideoBuffers[m_iYV12RenderBuffer]);

  D3D11_MAPPED_SUBRESOURCE srclr[MAX_PLANES];
  uint8_t*                 src[MAX_PLANES];
  int                      srcStride[MAX_PLANES];

  for (unsigned int idx = 0; idx < buf->GetActivePlanes(); idx++)
  {
    if(!(buf->planes[idx].texture.LockRect(0, &srclr[idx], D3D11_MAP_READ)))
      CLog::Log(LOGERROR, __FUNCTION__" - failed to lock yuv textures into memory");
    else
    {
      src[idx] = (uint8_t*)srclr[idx].pData;
      srcStride[idx] = srclr[idx].RowPitch;
    }
  }
  
  D3D11_MAPPED_SUBRESOURCE destlr;
  if (!m_IntermediateTarget.LockRect(0, &destlr, D3D11_MAP_WRITE_DISCARD))
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock swtarget texture into memory");

  uint8_t *dst[] = { (uint8_t*)destlr.pData, 0, 0, 0 };
  int dstStride[] = { destlr.RowPitch, 0, 0, 0 };

  sws_scale(m_sw_scale_ctx, src, srcStride, 0, m_sourceHeight, dst, dstStride);

  for (unsigned int idx = 0; idx < buf->GetActivePlanes(); idx++)
    if(!(buf->planes[idx].texture.UnlockRect(0)))
      CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock yuv textures");

  if (!m_IntermediateTarget.UnlockRect(0))
    CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock swtarget texture");

  // 2. scale to display

  // Don't know where this martian comes from but it can happen in the initial frames of a video
  if ((m_destRect.x1 < 0 && m_destRect.x2 < 0) || (m_destRect.y1 < 0 && m_destRect.y2 < 0))
    return;

  D3D11_TEXTURE2D_DESC srcDesc;
  m_IntermediateTarget.Get()->GetDesc(&srcDesc);

  float srcWidth  = (float)srcDesc.Width;
  float srcHeight = (float)srcDesc.Height;

  bool cbcontrol          = (CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast != 50.0f || CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness != 50.0f);
  unsigned int contrast   = (unsigned int)(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast *.01f * 255.0f); // we have to divide by two here/multiply by two later
  unsigned int brightness = (unsigned int)(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness * .01f * 255.0f);

  g_Windowing.GetGUIShader()->SetSampler(m_TextureFilter);
  CRect tu = { m_sourceRect.x1 / srcWidth, m_sourceRect.y1 / srcHeight, m_sourceRect.x2 / srcWidth, m_sourceRect.y2 / srcHeight };
  // pass contrast and brightness as diffuse color elements (see shader code)
  CD3DTexture::DrawQuad(m_rotatedDestCoords, D3DCOLOR_ARGB(255, contrast, brightness, 255), &m_IntermediateTarget, &tu,
                       !cbcontrol ? SHADER_METHOD_RENDER_VIDEO : SHADER_METHOD_RENDER_VIDEO_CONTROL);
}

void CWinRenderer::RenderPS()
{
  CD3D11_VIEWPORT viewPort(0.0f, 0.0f, 0.0f, 0.0f);
  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();

  ID3D11RenderTargetView *oldRTView = nullptr; ID3D11DepthStencilView* oldDSView = nullptr;
  if (m_bUseHQScaler)
  {
    // store current render target and depth view.
    pContext->OMGetRenderTargets(1, &oldRTView, &oldDSView);
    // change destination for HQ scallers
    ID3D11RenderTargetView* pRTView = m_IntermediateTarget.GetRenderTarget();
    pContext->OMSetRenderTargets(1, &pRTView, nullptr);
    // viewport equals intermediate target size
    viewPort = CD3D11_VIEWPORT(0.0f, 0.0f, 
                               static_cast<float>(m_IntermediateTarget.GetWidth()), 
                               static_cast<float>(m_IntermediateTarget.GetHeight()));
    g_Windowing.ResetScissors();
  }
  else
  {
    // viewport equals full backbuffer size
    CRect bbSize = g_Windowing.GetBackBufferRect();
    viewPort = CD3D11_VIEWPORT(0.f, 0.f, bbSize.Width(), bbSize.Height());
  }
  // reset view port
  pContext->RSSetViewports(1, &viewPort);
  // select destination rectangle 
  CPoint destPoints[4];
  if (m_renderOrientation)
  {
    for (size_t i = 0; i < 4; i++)
      destPoints[i] = m_rotatedDestCoords[i];
  }
  else
  {
    CRect destRect = m_bUseHQScaler ? m_sourceRect : g_graphicsContext.StereoCorrection(m_destRect);
    destPoints[0] = { destRect.x1, destRect.y1 };
    destPoints[1] = { destRect.x2, destRect.y1 };
    destPoints[2] = { destRect.x2, destRect.y2 };
    destPoints[3] = { destRect.x1, destRect.y2 };
  }

  // render video frame
  m_colorShader->Render(m_sourceRect, destPoints,
                        CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast,
                        CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness,
                        m_iFlags, reinterpret_cast<YUVBuffer*>(m_VideoBuffers[m_iYV12RenderBuffer]));
  // Restore our view port.
  g_Windowing.RestoreViewPort();
  // Restore the render target and depth view.
  if (m_bUseHQScaler)
  {
    pContext->OMSetRenderTargets(1, &oldRTView, oldDSView);
    SAFE_RELEASE(oldRTView);
    SAFE_RELEASE(oldDSView);
  }
}

void CWinRenderer::RenderHQ()
{
  m_scalerShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_destWidth, m_destHeight
                       , m_sourceRect, g_graphicsContext.StereoCorrection(m_destRect)
                       , false);
}

void CWinRenderer::RenderHW(DWORD flags)
{
  DXVABuffer *image = reinterpret_cast<DXVABuffer*>(m_VideoBuffers[m_iYV12RenderBuffer]);
  if (!image->pic)
    return;
  
  int past = 0;
  int future = 0;
  DXVABuffer **buffers = reinterpret_cast<DXVABuffer**>(m_VideoBuffers);

  ID3D11View* views[8];
  memset(views, 0, 8 * sizeof(ID3D11View*));
  views[2] = image->pic->view;

  // set future frames
  while (future < 2)
  {
    bool found = false;
    for (int i = 0; i < m_NumYV12Buffers; i++)
    {
      if (buffers[i] && buffers[i]->pic && buffers[i]->frameIdx == image->frameIdx + (future*2 + 2))
      {
        views[1 - future++] = buffers[i]->pic->view;
        found = true;
        break;
      }
    }
    if (!found)
      break;
  }

  // set past frames
  while (past < 4)
  {
    bool found = false;
    for (int i = 0; i < m_NumYV12Buffers; i++)
    {
      if (buffers[i] && buffers[i]->pic && buffers[i]->frameIdx == image->frameIdx - (past*2 + 2))
      {
        views[3 + past++] = buffers[i]->pic->view;
        found = true;
        break;
      }
    }
    if (!found)
      break;
  }

  CRect destRect;
  switch (m_renderOrientation)
  {
  case 90:
    destRect = CRect(m_rotatedDestCoords[3], m_rotatedDestCoords[1]);
    break;
  case 180:
    destRect = m_destRect;
    break;
  case 270:
    destRect = CRect(m_rotatedDestCoords[1], m_rotatedDestCoords[3]);
    break;
  default:
    destRect = m_bUseHQScaler ? m_sourceRect : g_graphicsContext.StereoCorrection(m_destRect);
    break;
  }

  CRect src = m_sourceRect, dst = destRect;
  CRect target = CRect(0.0f, 0.0f,
                       static_cast<float>(m_IntermediateTarget.GetWidth()), 
                       static_cast<float>(m_IntermediateTarget.GetHeight()));
  if (m_capture)
  {
    target.x2 = m_capture->GetWidth();
    target.y2 = m_capture->GetHeight();
  }
  CWIN32Util::CropSource(src, dst, target, m_renderOrientation);

  ID3D11RenderTargetView* pView = nullptr;
  ID3D11Resource* pResource = m_IntermediateTarget.Get();
  if (m_capture)
  {
    g_Windowing.Get3D11Context()->OMGetRenderTargets(1, &pView, nullptr);
    if (pView)
      pView->GetResource(&pResource);
  }

  m_processor->Render(src, dst, pResource, views, flags, image->frameIdx, m_renderOrientation);

  if (m_capture)
  {
    SAFE_RELEASE(pResource);
    SAFE_RELEASE(pView);
  }

  if (!m_bUseHQScaler && !m_capture)
  {
    CRect oldViewPort;
    bool stereoHack = g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_SPLIT_HORIZONTAL
                   || g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_SPLIT_VERTICAL;

    if (stereoHack)
    {
      CRect bbSize = g_Windowing.GetBackBufferRect();

      g_Windowing.GetViewPort(oldViewPort);
      g_Windowing.SetViewPort(bbSize);
      g_Windowing.SetCameraPosition(CPoint(bbSize.Width() / 2.f, bbSize.Height() / 2.f), bbSize.Width(), bbSize.Height(), 0.f);
    }

    // render frame
    CRect tu = { dst.x1 / m_destWidth, dst.y1 / m_destHeight, dst.x2 / m_destWidth, dst.y2 / m_destHeight };
    CD3DTexture::DrawQuad(dst, 0xFFFFFF, &m_IntermediateTarget, &tu, SHADER_METHOD_RENDER_TEXTURE_NOBLEND);

    if (stereoHack)
      g_Windowing.SetViewPort(oldViewPort);
  }
}

bool CWinRenderer::RenderCapture(CRenderCapture* capture)
{
  if (!m_bConfigured || m_NumYV12Buffers == 0)
    return false;

  bool succeeded = false;

  ID3D11DeviceContext* pContext = g_Windowing.Get3D11Context();

  CRect saveSize = m_destRect;
  saveRotatedCoords();//backup current m_rotatedDestCoords

  m_destRect.SetRect(0, 0, (float)capture->GetWidth(), (float)capture->GetHeight());
  syncDestRectToRotatedPoints();//syncs the changed destRect to m_rotatedDestCoords

  ID3D11DepthStencilView* oldDepthView;
  ID3D11RenderTargetView* oldSurface;
  pContext->OMGetRenderTargets(1, &oldSurface, &oldDepthView);

  capture->BeginRender();
  if (capture->GetState() != CAPTURESTATE_FAILED)
  {
    m_capture = capture;
    Render(0);
    m_capture = nullptr;
    capture->EndRender();
    succeeded = true;
  }

  pContext->OMSetRenderTargets(1, &oldSurface, oldDepthView);
  oldSurface->Release();
  SAFE_RELEASE(oldDepthView); // it can be nullptr

  m_destRect = saveSize;
  restoreRotatedCoords();//restores the previous state of the rotated dest coords

  return succeeded;
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CWinRenderer::DeleteYV12Texture(int index)
{
  CSingleLock lock(g_graphicsContext);
  SAFE_DELETE(m_VideoBuffers[index]);
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
    if (!buf->Create(m_format, m_sourceWidth, m_sourceHeight, m_renderMethod == RENDER_PS))
    {
      CLog::Log(LOGERROR, __FUNCTION__" - Unable to create YV12 video texture %i", index);
      delete buf;
      return false;
    }
    m_VideoBuffers[index] = buf;
  }

  m_VideoBuffers[index]->StartDecode();
  m_VideoBuffers[index]->Clear();

  CLog::Log(LOGDEBUG, "created video buffer %i", index);
  return true;
}

bool CWinRenderer::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if (m_renderMethod == RENDER_DXVA)
    return false; // only auto. DXVA processor selects deinterlacing method automatically

  if(m_format != RENDER_FMT_DXVA
  && (   method == VS_INTERLACEMETHOD_DEINTERLACE
      || method == VS_INTERLACEMETHOD_DEINTERLACE_HALF))
    return true;

  return false;
}

bool CWinRenderer::Supports(ERENDERFEATURE feature)
{
  if(feature == RENDERFEATURE_BRIGHTNESS)
    return true;

  if(feature == RENDERFEATURE_CONTRAST)
    return true;

  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_NONLINSTRETCH   ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_ROTATION        ||
      feature == RENDERFEATURE_POSTPROCESS)
    return true;


  return false;
}

bool CWinRenderer::Supports(ESCALINGMETHOD method)
{
  if (m_renderMethod == RENDER_PS || m_renderMethod == RENDER_DXVA)
  {
    if (m_renderMethod == RENDER_DXVA)
    {
      if (method == VS_SCALINGMETHOD_DXVA_HARDWARE ||
          method == VS_SCALINGMETHOD_AUTO)
        return true;
      else if (!g_advancedSettings.m_DXVAAllowHqScaling || m_renderOrientation)
        return false;
    }

    if (  method == VS_SCALINGMETHOD_AUTO
      || (method == VS_SCALINGMETHOD_LINEAR && m_renderMethod == RENDER_PS)) 
        return true;

    if (g_Windowing.GetFeatureLevel() >= D3D_FEATURE_LEVEL_9_3 && !m_renderOrientation)
    {
      if(method == VS_SCALINGMETHOD_CUBIC
      || method == VS_SCALINGMETHOD_LANCZOS2
      || method == VS_SCALINGMETHOD_SPLINE36_FAST
      || method == VS_SCALINGMETHOD_LANCZOS3_FAST
      || method == VS_SCALINGMETHOD_SPLINE36
      || method == VS_SCALINGMETHOD_LANCZOS3)
      {
        // if scaling is below level, avoid hq scaling
        float scaleX = fabs(((float)m_sourceWidth - m_destRect.Width())/m_sourceWidth)*100;
        float scaleY = fabs(((float)m_sourceHeight - m_destRect.Height())/m_sourceHeight)*100;
        int minScale = CSettings::GetInstance().GetInt(CSettings::SETTING_VIDEOPLAYER_HQSCALERS);
        if (scaleX < minScale && scaleY < minScale)
          return false;
        return true;
      }
    }
  }
  else if(m_renderMethod == RENDER_SW)
  {
    if(method == VS_SCALINGMETHOD_AUTO
    || method == VS_SCALINGMETHOD_NEAREST
    || method == VS_SCALINGMETHOD_LINEAR)
      return true;
  }
  return false;
}

EINTERLACEMETHOD CWinRenderer::AutoInterlaceMethod()
{
  if (m_renderMethod == RENDER_DXVA)
    return VS_INTERLACEMETHOD_RENDER_BOB;

  return VS_INTERLACEMETHOD_DEINTERLACE_HALF;
}

CRenderInfo CWinRenderer::GetRenderInfo()
{
  CRenderInfo info;
  info.formats = m_formats;
  info.max_buffer_size = NUM_BUFFERS;
  if (m_renderMethod == RENDER_DXVA && m_processor)
    info.optimal_buffer_size = m_processor->Size();
  else
    info.optimal_buffer_size = 4;
  return info;
}

void CWinRenderer::ReleaseBuffer(int idx)
{
  if (m_renderMethod == RENDER_DXVA && m_VideoBuffers[idx])
    SAFE_RELEASE(reinterpret_cast<DXVABuffer*>(m_VideoBuffers[idx])->pic);
}

bool CWinRenderer::NeedBufferForRef(int idx)
{
  // check if processor wants to keep past frames
  if (m_renderMethod == RENDER_DXVA && m_processor)
  {
    DXVABuffer** buffers = reinterpret_cast<DXVABuffer**>(m_VideoBuffers);

    int numPast = m_processor->PastRefs();
    if (buffers[idx] && buffers[idx]->pic)
    {
      if (buffers[idx]->frameIdx + numPast*2 >= buffers[m_iYV12RenderBuffer]->frameIdx)
        return true;
    }
  }
  return false;
}

//============================================

YUVBuffer::~YUVBuffer()
{
  Release();
}

bool YUVBuffer::Create(ERenderFormat format, unsigned int width, unsigned int height, bool dynamic)
{
  m_format = format;
  m_width = width;
  m_height = height;
  m_mapType = dynamic ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
  // Create the buffers with D3D11_USAGE_DYNAMIC which can be used as shader resource for PS rendering
  // or D3D11_USAGE_STAGING which can be read and written for SW rendering
  D3D11_USAGE usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_STAGING;

  switch(m_format)
  {
  case RENDER_FMT_YUV420P10:
  case RENDER_FMT_YUV420P16:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width,      m_height,      1, usage, DXGI_FORMAT_R16_UNORM)
        || !planes[PLANE_U].texture.Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R16_UNORM)
        || !planes[PLANE_V].texture.Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R16_UNORM))
        return false;
      m_activeplanes = 3;
      break;
    }
  case RENDER_FMT_YUV420P:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width,      m_height,      1, usage, DXGI_FORMAT_R8_UNORM)
        || !planes[PLANE_U].texture.Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R8_UNORM)
        || !planes[PLANE_V].texture.Create(m_width >> 1, m_height >> 1, 1, usage, DXGI_FORMAT_R8_UNORM))
        return false;
      m_activeplanes = 3;
      break;
    }
  case RENDER_FMT_DXVA:
  case RENDER_FMT_NV12:
    {
      DXGI_FORMAT uvFormat = DXGI_FORMAT_R8G8_UNORM;
      // FL 9.x doesn't support DXGI_FORMAT_R8G8_UNORM, so we have to use SNORM and correct values in shader
      if (!g_Windowing.IsFormatSupport(uvFormat, D3D11_FORMAT_SUPPORT_TEXTURE2D))
        uvFormat = DXGI_FORMAT_R8G8_SNORM;
      if ( !planes[PLANE_Y].texture.Create( m_width,      m_height,      1, usage, DXGI_FORMAT_R8_UNORM)
        || !planes[PLANE_UV].texture.Create(m_width >> 1, m_height >> 1, 1, usage, uvFormat))
        return false;
      m_activeplanes = 2;
      break;
    }
  case RENDER_FMT_YUYV422:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width >> 1, m_height, 1, usage, DXGI_FORMAT_B8G8R8A8_UNORM))
        return false;
      m_activeplanes = 1;
      break;
    }
  case RENDER_FMT_UYVY422:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width >> 1, m_height, 1, usage, DXGI_FORMAT_B8G8R8A8_UNORM))
        return false;
      m_activeplanes = 1;
      break;
    }
  default:
    m_activeplanes = 0;
    return false;
  }

  return true;
}

void YUVBuffer::Release()
{
  SAFE_RELEASE(m_staging);
  for(unsigned i = 0; i < m_activeplanes; i++)
  {
    // unlock before release
    if (m_locked && planes[i].texture.Get() && planes[i].rect.pData)
      planes[i].texture.UnlockRect(0);

    planes[i].texture.Release();
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void YUVBuffer::StartRender()
{
  if (!m_locked)
    return;

  if (m_bPending)
  {
    PerformCopy();
    m_bPending = false;
  }

  m_locked = false;

  for (unsigned i = 0; i < m_activeplanes; i++)
  {
    if (planes[i].texture.Get() && planes[i].rect.pData)
      if (!planes[i].texture.UnlockRect(0))
        CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock texture %d", i);
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void YUVBuffer::StartDecode()
{
  if (m_locked)
    return;

  m_locked = true;
  m_bPending = false;

  for(unsigned i = 0; i < m_activeplanes; i++)
  {
    if(planes[i].texture.Get()
    && planes[i].texture.LockRect(0, &planes[i].rect, m_mapType) == false)
    {
      memset(&planes[i].rect, 0, sizeof(planes[i].rect));
      CLog::Log(LOGERROR, __FUNCTION__" - failed to lock texture %d into memory", i);
      m_locked = false;
    }
  }
}

void YUVBuffer::Clear()
{
  // Set Y to 0 and U,V to 128 (RGB 0,0,0) to avoid visual artifacts at the start of playback

  switch(m_format)
  {
  case RENDER_FMT_YUV420P16:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pData, 0,     planes[PLANE_Y].rect.RowPitch *  m_height    / 2);
      wmemset((wchar_t*)planes[PLANE_U].rect.pData, 32768, planes[PLANE_U].rect.RowPitch * (m_height / 2) / 2);
      wmemset((wchar_t*)planes[PLANE_V].rect.pData, 32768, planes[PLANE_V].rect.RowPitch * (m_height / 2) / 2);
      break;
    }
  case RENDER_FMT_YUV420P10:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pData, 0, planes[PLANE_Y].rect.RowPitch *  m_height / 2);
      wmemset((wchar_t*)planes[PLANE_U].rect.pData, 512, planes[PLANE_U].rect.RowPitch * (m_height / 2) / 2);
      wmemset((wchar_t*)planes[PLANE_V].rect.pData, 512, planes[PLANE_V].rect.RowPitch * (m_height / 2) / 2);
      break;
    }
  case RENDER_FMT_YUV420P:
    {
      memset(planes[PLANE_Y].rect.pData, 0, planes[PLANE_Y].rect.RowPitch *  m_height);
      memset(planes[PLANE_U].rect.pData, 128, planes[PLANE_U].rect.RowPitch * (m_height / 2));
      memset(planes[PLANE_V].rect.pData, 128, planes[PLANE_V].rect.RowPitch * (m_height / 2));
      break;
    }
  case RENDER_FMT_DXVA:
  case RENDER_FMT_NV12:
    {
      memset(planes[PLANE_Y].rect.pData,    0, planes[PLANE_Y].rect.RowPitch *  m_height);
      memset(planes[PLANE_UV].rect.pData, 128, planes[PLANE_U].rect.RowPitch * (m_height / 2));
      break;
    }
  // YUY2, UYVY: wmemset to set a 16bit pattern, byte-swapped because x86 is LE
  case RENDER_FMT_YUYV422:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pData, 0x8000, planes[PLANE_Y].rect.RowPitch / 2 * m_height);
      break;
    }
  case RENDER_FMT_UYVY422:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pData, 0x0080, planes[PLANE_Y].rect.RowPitch / 2 * m_height);
      break;
    }

  }
}

bool YUVBuffer::IsReadyToRender()
{
  return !m_locked;
}

bool YUVBuffer::CopyFromPicture(DVDVideoPicture &picture)
{
  if (picture.format == RENDER_FMT_DXVA)
  {
    return CopyFromDXVA(reinterpret_cast<ID3D11VideoDecoderOutputView*>(picture.dxva->view));
  }
  return false;
}

bool YUVBuffer::CopyFromDXVA(ID3D11VideoDecoderOutputView* pView)
{
  if (!pView)
    return false;

  HRESULT hr = S_OK;
  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vpivd;
  pView->GetDesc(&vpivd);
  ID3D11Resource* resource = nullptr;
  pView->GetResource(&resource);

  if (!m_staging)
  {
    // create staging texture
    ID3D11Texture2D* surface = nullptr;
    hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&surface));
    if (SUCCEEDED(hr))
    {
      D3D11_TEXTURE2D_DESC tDesc;
      surface->GetDesc(&tDesc);
      SAFE_RELEASE(surface);

      CD3D11_TEXTURE2D_DESC sDesc(tDesc);
      sDesc.ArraySize = 1;
      sDesc.Usage = D3D11_USAGE_STAGING;
      sDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      sDesc.BindFlags = 0;

      hr = g_Windowing.Get3D11Device()->CreateTexture2D(&sDesc, nullptr, &m_staging);
      if (SUCCEEDED(hr))
        m_sDesc = sDesc;
    }
  }

  if (m_staging)
  {
    ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();
    // copy content from decoder texture to temporary texture.
    pContext->CopySubresourceRegion(m_staging,
                                    D3D11CalcSubresource(0, 0, 1),
                                    0, 0, 0,
                                    resource,
                                    D3D11CalcSubresource(0, vpivd.Texture2D.ArraySlice, 1),
                                    nullptr);
    m_bPending = true;
  }
  SAFE_RELEASE(resource);

  return SUCCEEDED(hr);
}

void YUVBuffer::PerformCopy()
{
  if (!m_locked)
    return;

  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();
  D3D11_MAPPED_SUBRESOURCE rectangle;
  if (SUCCEEDED(pContext->Map(m_staging, 0, D3D11_MAP_READ, 0, &rectangle)))
  {
    void* (*copy_func)(void* d, const void* s, size_t size) =
        ((g_cpuInfo.GetCPUFeatures() & CPU_FEATURE_SSE4) != 0) ? gpu_memcpy : memcpy;

    uint8_t* s_y = static_cast<uint8_t*>(rectangle.pData);
    uint8_t *s_uv = static_cast<uint8_t*>(rectangle.pData) + m_sDesc.Height * rectangle.RowPitch;
    uint8_t* d_y = static_cast<uint8_t*>(planes[PLANE_Y].rect.pData);
    uint8_t *d_uv = static_cast<uint8_t*>(planes[PLANE_UV].rect.pData);

    if ( planes[PLANE_Y ].rect.RowPitch == rectangle.RowPitch
      && planes[PLANE_UV].rect.RowPitch == rectangle.RowPitch)
    {
      copy_func(d_y, s_y, rectangle.RowPitch * m_height);
      copy_func(d_uv, s_uv, rectangle.RowPitch * m_height >> 1);
    }
    else
    {
      for (unsigned y = 0; y < m_sDesc.Height >> 1; ++y)
      {
        // Copy Y
        copy_func(d_y, s_y, planes[PLANE_Y].rect.RowPitch);
        s_y += rectangle.RowPitch;
        d_y += planes[PLANE_Y].rect.RowPitch;
        // Copy Y
        copy_func(d_y, s_y, planes[PLANE_Y].rect.RowPitch);
        s_y += rectangle.RowPitch;
        d_y += planes[PLANE_Y].rect.RowPitch;
        // Copy UV
        copy_func(d_uv, s_uv, planes[PLANE_UV].rect.RowPitch);
        s_uv += rectangle.RowPitch;
        d_uv += planes[PLANE_UV].rect.RowPitch;
      }
    }
    pContext->Unmap(m_staging, 0);
  }
}

#endif
