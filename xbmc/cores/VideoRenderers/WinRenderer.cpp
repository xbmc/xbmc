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

#ifdef HAS_DX

#include "WinRenderer.h"
#include "Util.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "guilib/Texture.h"
#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "utils/MathUtils.h"
#include "VideoShaders/WinVideoFilter.h"
#include "DllSwScale.h"
#include "guilib/LocalizeStrings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "win32/WIN32Util.h"
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"

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
  m_extended_format = 0;

  m_iRequestedMethod = RENDER_METHOD_AUTO;

  m_renderMethod = RENDER_PS;
  m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  m_scalingMethodGui = (ESCALINGMETHOD)-1;
  m_TextureFilter = D3DTEXF_POINT;

  m_bUseHQScaler = false;
  m_bFilterInitialized = false;

  for (int i = 0; i<NUM_BUFFERS; i++)
    m_VideoBuffers[i] = NULL;

  m_sw_scale_ctx = NULL;
  m_dllSwScale = NULL;
}

CWinRenderer::~CWinRenderer()
{
  UnInit();
}

static enum PixelFormat PixelFormatFromFormat(ERenderFormat format)
{
  if      (format == RENDER_FMT_YUV420P)   return PIX_FMT_YUV420P;
  else if (format == RENDER_FMT_YUV420P10) return PIX_FMT_YUV420P10;
  else if (format == RENDER_FMT_YUV420P10) return PIX_FMT_YUV420P16;
  else if (format == RENDER_FMT_NV12)      return PIX_FMT_NV12;
  else if (format == RENDER_FMT_UYVY422)   return PIX_FMT_UYVY422;
  else if (format == RENDER_FMT_YUYV422)   return PIX_FMT_YUYV422;
  else return PIX_FMT_NONE;
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
  // Set rendering to dxva before trying it, in order to open the correct processor immediately, when deinterlacing method is auto.

  // Force dxva renderer after dxva decoding: PS and SW renderers have performance issues after dxva decode.
  if (g_advancedSettings.m_DXVAForceProcessorRenderer && m_format == RENDER_FMT_DXVA)
  {
    CLog::Log(LOGNOTICE, "D3D: rendering method forced to DXVA2 processor");
    m_renderMethod = RENDER_DXVA;
    if (!m_processor.Open(m_sourceWidth, m_sourceHeight, m_iFlags, m_format, m_extended_format))
    {
      CLog::Log(LOGNOTICE, "D3D: unable to open DXVA2 processor");
      m_processor.Close();
      m_renderMethod = RENDER_INVALID;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": Requested render method: %d", m_iRequestedMethod);

    switch(m_iRequestedMethod)
    {
      case RENDER_METHOD_DXVA:
        m_renderMethod = RENDER_DXVA;
        if (m_processor.Open(m_sourceWidth, m_sourceHeight, m_iFlags, m_format, m_extended_format))
            break;
        else
        {
          CLog::Log(LOGNOTICE, "D3D: unable to open DXVA2 processor");
          m_processor.Close();
        }
      // Drop through to pixel shader
      case RENDER_METHOD_AUTO:
      case RENDER_METHOD_D3D_PS:
        // Try the pixel shaders support
        if (m_deviceCaps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
        {
          CTestShader shader;
          if (shader.Create())
          {
            m_renderMethod = RENDER_PS;
            break;
          }
          else
          {
            CLog::Log(LOGNOTICE, "D3D: unable to load test shader - D3D installation is most likely incomplete");
            CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "DirectX", g_localizeStrings.Get(2101));
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
        // So we'll do the color conversion in software.
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
    m_dllSwScale = new DllSwScale();

    if (!m_dllSwScale->Load())
      CLog::Log(LOGERROR,"CDVDDemuxFFmpeg::Open - failed to load ffmpeg libraries");

    if(!m_SWTarget.Create(m_sourceWidth, m_sourceHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT))
    {
      CLog::Log(LOGNOTICE, __FUNCTION__": Failed to create sw render target.");
      return false;
    }
  }
  return true;
}

bool CWinRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  if(m_sourceWidth  != width
  || m_sourceHeight != height)
  {
    m_sourceWidth       = width;
    m_sourceHeight      = height;
    // need to recreate textures
    m_NumYV12Buffers    = 0;
    m_iYV12RenderBuffer = 0;
    // reinitialize the filters/shaders
    m_bFilterInitialized = false;
  }

  m_fps = fps;
  m_iFlags = flags;
  m_format = format;
  m_extended_format = extended_format;

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  m_destWidth = g_settings.m_ResInfo[m_resolution].iWidth;
  m_destHeight = g_settings.m_ResInfo[m_resolution].iHeight;
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

bool CWinRenderer::AddVideoPicture(DVDVideoPicture* picture)
{
  if (m_renderMethod == RENDER_DXVA)
  {
    int source = NextYV12Texture();
    if(source < 0)
      return false;

    DXVABuffer *buf = (DXVABuffer*)m_VideoBuffers[source];
    buf->id = m_processor.Add(picture);
    return true;
  }
  return false;
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
  if(m_format == RENDER_FMT_YUV420P10
  || m_format == RENDER_FMT_YUV420P16)
    image->bpp = 2;
  else
    image->bpp = 1;

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

unsigned int CWinRenderer::PreInit()
{
  CSingleLock lock(g_graphicsContext);
  m_bConfigured = false;
  UnInit();
  m_resolution = g_guiSettings.m_LookAndFeelResolution;
  if ( m_resolution == RES_WINDOW )
    m_resolution = RES_DESKTOP;

  // setup the background colour
  m_clearColour = (g_advancedSettings.m_videoBlackBarColour & 0xff) * 0x010101;

  g_Windowing.Get3DDevice()->GetDeviceCaps(&m_deviceCaps);

  m_iRequestedMethod = g_guiSettings.GetInt("videoplayer.rendermethod");

  if ((g_advancedSettings.m_DXVAForceProcessorRenderer || m_iRequestedMethod == RENDER_METHOD_DXVA) && !m_processor.PreInit())
    CLog::Log(LOGNOTICE, "CWinRenderer::Preinit - could not init DXVA2 processor - skipping");

  m_formats.push_back(RENDER_FMT_YUV420P);
  if(g_Windowing.IsTextureFormatOk(D3DFMT_L16, 0))
  {
    m_formats.push_back(RENDER_FMT_YUV420P10);
    m_formats.push_back(RENDER_FMT_YUV420P16);
  }
  m_formats.push_back(RENDER_FMT_NV12);
  m_formats.push_back(RENDER_FMT_YUYV422);
  m_formats.push_back(RENDER_FMT_UYVY422);


  return 0;
}

void CWinRenderer::UnInit()
{
  CSingleLock lock(g_graphicsContext);

  if (m_SWTarget.Get())
    m_SWTarget.Release();

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
    m_dllSwScale->sws_freeContext(m_sw_scale_ctx);
    m_sw_scale_ctx = NULL;
  }
  SAFE_DELETE(m_dllSwScale);

  m_processor.UnInit();
}

bool CWinRenderer::CreateIntermediateRenderTarget()
{
  // Initialize a render target for intermediate rendering - same size as the video source
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  D3DFORMAT format = D3DFMT_X8R8G8B8;
  DWORD usage = D3DUSAGE_RENDERTARGET;

  if      (g_Windowing.IsTextureFormatOk(D3DFMT_A2R10G10B10, usage)) format = D3DFMT_A2R10G10B10;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_A2B10G10R10, usage)) format = D3DFMT_A2B10G10R10;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_A8R8G8B8, usage))    format = D3DFMT_A8R8G8B8;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_A8B8G8R8, usage))    format = D3DFMT_A8B8G8R8;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_X8R8G8B8, usage))    format = D3DFMT_X8R8G8B8;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_X8B8G8R8, usage))    format = D3DFMT_X8B8G8R8;
  else if (g_Windowing.IsTextureFormatOk(D3DFMT_R8G8B8, usage))      format = D3DFMT_R8G8B8;

  CLog::Log(LOGDEBUG, __FUNCTION__": format %i", format);

  if(!m_IntermediateTarget.Create(m_sourceWidth, m_sourceHeight, 1, usage, format, D3DPOOL_DEFAULT))
  {
    CLog::Log(LOGERROR, __FUNCTION__": render target creation failed. Going back to bilinear scaling.", format);
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
      m_TextureFilter = D3DTEXF_LINEAR;
      break;
    }
    // fall through for fallback
  case VS_SCALINGMETHOD_NEAREST:
  default:
    m_TextureFilter = D3DTEXF_POINT;
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

    if (Supports(VS_SCALINGMETHOD_LANCZOS3_FAST) && scaleSD && scaleUp && scaleFps)
    {
      m_scalingMethod = VS_SCALINGMETHOD_LANCZOS3_FAST;
      m_bUseHQScaler = true;
    }
  }
}

void CWinRenderer::UpdatePSVideoFilter()
{
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
    if (m_scalerShader == NULL)
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

  if(m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  if (m_bUseHQScaler && !CreateIntermediateRenderTarget())
  {
    SAFE_DELETE(m_scalerShader);
    m_bUseHQScaler = false;
  }

  SAFE_DELETE(m_colorShader);

  if (m_bUseHQScaler)
  {
    m_colorShader = new CYUV2RGBShader();
    if (!m_colorShader->Create(m_sourceWidth, m_sourceHeight, m_format))
    {
      // Try again after disabling the HQ scaler and freeing its resources
      m_IntermediateTarget.Release();
      SAFE_DELETE(m_scalerShader);
      SAFE_DELETE(m_colorShader);
      m_bUseHQScaler = false;
    }
  }

  if (!m_bUseHQScaler) //fallback from HQ scalers and multipass creation above
  {
    m_colorShader = new CYUV2RGBShader();
    if (!m_colorShader->Create(m_sourceWidth, m_sourceHeight, m_format))
      SAFE_DELETE(m_colorShader);
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

void CWinRenderer::Render(DWORD flags)
{
  if (m_renderMethod == RENDER_DXVA)
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

  // Don't need a stencil/depth buffer and a buffer smaller than the render target causes D3D complaints and nVidia issues
  // Save & restore when we're done.
  LPDIRECT3DSURFACE9 pZBuffer;
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
  pD3DDevice->GetDepthStencilSurface(&pZBuffer);
  pD3DDevice->SetDepthStencilSurface(NULL);

  if (m_renderMethod == RENDER_SW)
    RenderSW();
  else if (m_renderMethod == RENDER_PS)
    RenderPS();

  pD3DDevice->SetDepthStencilSurface(pZBuffer);
  pZBuffer->Release();
}

void CWinRenderer::RenderSW()
{
  enum PixelFormat format = PixelFormatFromFormat(m_format);

  // 1. convert yuv to rgb
  m_sw_scale_ctx = m_dllSwScale->sws_getCachedContext(m_sw_scale_ctx,
                                                      m_sourceWidth, m_sourceHeight, format,
                                                      m_sourceWidth, m_sourceHeight, PIX_FMT_BGRA,
                                                      SWS_FAST_BILINEAR | SwScaleCPUFlags(), NULL, NULL, NULL);

  YUVBuffer* buf = (YUVBuffer*)m_VideoBuffers[m_iYV12RenderBuffer];

  D3DLOCKED_RECT   srclr[MAX_PLANES];
  uint8_t         *src[MAX_PLANES];
  int              srcStride[MAX_PLANES];

  for (unsigned int idx = 0; idx < buf->GetActivePlanes(); idx++)
  {
    if(!(buf->planes[idx].texture.LockRect(0, &srclr[idx], NULL, D3DLOCK_READONLY)))
      CLog::Log(LOGERROR, __FUNCTION__" - failed to lock yuv textures into memory");
    else
    {
      src[idx] = (uint8_t*)srclr[idx].pBits;
      srcStride[idx] = srclr[idx].Pitch;
    }
  }
  
  D3DLOCKED_RECT destlr = {0,0};
  if (!m_SWTarget.LockRect(0, &destlr, NULL, D3DLOCK_DISCARD))
    CLog::Log(LOGERROR, __FUNCTION__" - failed to lock swtarget texture into memory");

  uint8_t *dst[]  = { (uint8_t*) destlr.pBits, 0, 0, 0 };
  int dstStride[] = { destlr.Pitch, 0, 0, 0 };

  m_dllSwScale->sws_scale(m_sw_scale_ctx, src, srcStride, 0, m_sourceHeight, dst, dstStride);

  for (unsigned int idx = 0; idx < buf->GetActivePlanes(); idx++)
    if(!(buf->planes[idx].texture.UnlockRect(0)))
      CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock yuv textures");

  if (!m_SWTarget.UnlockRect(0))
    CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock swtarget texture");

  // 2. scale to display

  // Don't know where this martian comes from but it can happen in the initial frames of a video
  if ((m_destRect.x1 < 0 && m_destRect.x2 < 0) || (m_destRect.y1 < 0 && m_destRect.y2 < 0))
    return;

  ScaleFixedPipeline();
}

/*
Code kept for reference, as a basis to re-enable StretchRect and 
do the color conversion with it.
See IDirect3D9::CheckDeviceFormat() for support of non-standard fourcc textures
and IDirect3D9::CheckDeviceFormatConversion for color conversion support

void CWinRenderer::ScaleStretchRect()
{
  // Test HW scaler support. StretchRect is slightly faster than drawing a quad.
  // If linear filtering is not supported, drop back to quads, as most HW has linear filtering for quads.
  //if(m_deviceCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES
  //&& m_deviceCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MINFLINEAR
  //&& m_deviceCaps.StretchRectFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
  //{
  //  m_StretchRectSupported = true;
  //}

  CRect sourceRect = m_sourceRect;
  CRect destRect = m_destRect;

  D3DSURFACE_DESC desc;
  if (FAILED(target->GetDesc(&desc)))
    CLog::Log(LOGERROR, "CWinRenderer::Render - failed to get back buffer description");
  CRect tgtRect(0, 0, desc.Width, desc.Height);

  // Need to manipulate the coordinates since StretchRect doesn't accept off-screen coordinates.
  CWIN32Util::CropSource(sourceRect, destRect, tgtRect);

  RECT srcRect = { sourceRect.x1, sourceRect.y1, sourceRect.x2, sourceRect.y2 };
  IDirect3DSurface9* source;
  if(!m_SWTarget.GetSurfaceLevel(0, &source))
    CLog::Log(LOGERROR, "CWinRenderer::Render - failed to get source");

  RECT dstRect = { destRect.x1, destRect.y1, destRect.x2, destRect.y2 };
  IDirect3DSurface9* target;
  if(FAILED(g_Windowing.Get3DDevice()->GetRenderTarget(0, &target)))
    CLog::Log(LOGERROR, "CWinRenderer::Render - failed to get back buffer");

  HRESULT hr;
  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  if(FAILED(hr = pD3DDevice->StretchRect(source, &srcRect, target, &dstRect, m_TextureFilter)))
    CLog::Log(LOGERROR, __FUNCTION__" - StretchRect failed (0x%08X)", hr);

  target->Release();
  source->Release();
}
*/

void CWinRenderer::ScaleFixedPipeline()
{
  HRESULT hr;
  IDirect3DDevice9 *pD3DDev = g_Windowing.Get3DDevice();
  D3DSURFACE_DESC srcDesc;
  if (FAILED(hr = m_SWTarget.Get()->GetLevelDesc(0, &srcDesc)))
    CLog::Log(LOGERROR, __FUNCTION__": GetLevelDesc failed. %s", CRenderSystemDX::GetErrorDescription(hr).c_str());

  float srcWidth  = (float)srcDesc.Width;
  float srcHeight = (float)srcDesc.Height;

  bool cbcontrol          = (g_settings.m_currentVideoSettings.m_Contrast != 50.0f || g_settings.m_currentVideoSettings.m_Brightness != 50.0f);
  unsigned int contrast   = (unsigned int)(g_settings.m_currentVideoSettings.m_Contrast *.01f * 255.0f); // we have to divide by two here/multiply by two later
  unsigned int brightness = (unsigned int)(g_settings.m_currentVideoSettings.m_Brightness * .01f * 255.0f);

  D3DCOLOR diffuse  = D3DCOLOR_ARGB(255, contrast, contrast, contrast);
  D3DCOLOR specular = D3DCOLOR_ARGB(255, brightness, brightness, brightness);

  struct VERTEX
  {
    FLOAT x,y,z,rhw;
    D3DCOLOR diffuse;
    D3DCOLOR specular;
    FLOAT tu, tv;
  };

  VERTEX vertex[] =
  {
    {m_destRect.x1, m_destRect.y1, 0.0f, 1.0f, diffuse, specular, m_sourceRect.x1 / srcWidth, m_sourceRect.y1 / srcHeight},
    {m_destRect.x2, m_destRect.y1, 0.0f, 1.0f, diffuse, specular, m_sourceRect.x2 / srcWidth, m_sourceRect.y1 / srcHeight},
    {m_destRect.x2, m_destRect.y2, 0.0f, 1.0f, diffuse, specular, m_sourceRect.x2 / srcWidth, m_sourceRect.y2 / srcHeight},
    {m_destRect.x1, m_destRect.y2, 0.0f, 1.0f, diffuse, specular, m_sourceRect.x1 / srcWidth, m_sourceRect.y2 / srcHeight},
  };

  // Compensate for D3D coordinates system
  for(int i = 0; i < sizeof(vertex)/sizeof(vertex[0]); i++)
  {
    vertex[i].x -= 0.5f;
    vertex[i].y -= 0.5f;
  };

  pD3DDev->SetTexture(0, m_SWTarget.Get());

  if (!cbcontrol)
  {
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  }
  else
  {
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE2X );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    hr = pD3DDev->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_ADDSIGNED );
    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_SPECULAR );
    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    hr = pD3DDev->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_CURRENT );

    hr = pD3DDev->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
    hr = pD3DDev->SetTextureStageState( 2, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
  }

  hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  hr = pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED); 

  hr = pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, m_TextureFilter);
  hr = pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, m_TextureFilter);
  hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);

  if (FAILED(hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertex, sizeof(VERTEX))))
    CLog::Log(LOGERROR, __FUNCTION__": DrawPrimitiveUP failed. %s", CRenderSystemDX::GetErrorDescription(hr).c_str());

  pD3DDev->SetTexture(0, NULL);
}

void CWinRenderer::RenderPS()
{
  if (!m_bUseHQScaler)
  {
    Stage1();
  }
  else
  {
    Stage1();
    Stage2();
  }
}

void CWinRenderer::Stage1()
{
  if (!m_bUseHQScaler)
  {
      m_colorShader->Render(m_sourceRect, m_destRect,
                            g_settings.m_currentVideoSettings.m_Contrast,
                            g_settings.m_currentVideoSettings.m_Brightness,
                            m_iFlags,
                            (YUVBuffer*)m_VideoBuffers[m_iYV12RenderBuffer]);
  }
  else
  {
    // Switch the render target to the temporary destination
    LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();
    LPDIRECT3DSURFACE9 newRT, oldRT;
    m_IntermediateTarget.GetSurfaceLevel(0, &newRT);
    pD3DDevice->GetRenderTarget(0, &oldRT);
    pD3DDevice->SetRenderTarget(0, newRT);

    CRect srcRect(0.0f, 0.0f, m_sourceWidth, m_sourceHeight);
    CRect rtRect(0.0f, 0.0f, m_sourceWidth, m_sourceHeight);

    m_colorShader->Render(srcRect, rtRect,
                          g_settings.m_currentVideoSettings.m_Contrast,
                          g_settings.m_currentVideoSettings.m_Brightness,
                          m_iFlags,
                          (YUVBuffer*)m_VideoBuffers[m_iYV12RenderBuffer]);

    // Restore the render target
    pD3DDevice->SetRenderTarget(0, oldRT);

    oldRT->Release();
    newRT->Release();
  }
}

void CWinRenderer::Stage2()
{
  m_scalerShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_destWidth, m_destHeight, m_sourceRect, m_destRect);
}

void CWinRenderer::RenderProcessor(DWORD flags)
{
  CSingleLock lock(g_graphicsContext);
  HRESULT hr;

  DXVABuffer *image = (DXVABuffer*)m_VideoBuffers[m_iYV12RenderBuffer];

  IDirect3DSurface9* target;
  if(FAILED(hr = g_Windowing.Get3DDevice()->GetRenderTarget(0, &target)))
  {
    CLog::Log(LOGERROR, "CWinRenderer::RenderSurface - failed to get render target. %s", CRenderSystemDX::GetErrorDescription(hr).c_str());
    return;
  }

  m_processor.Render(m_sourceRect, m_destRect, target, image->id, flags);

  target->Release();
}

bool CWinRenderer::RenderCapture(CRenderCapture* capture)
{
  if (!m_bConfigured || m_NumYV12Buffers == 0)
    return false;

  bool succeeded = false;

  LPDIRECT3DDEVICE9 pD3DDevice = g_Windowing.Get3DDevice();

  CRect saveSize = m_destRect;
  saveRotatedCoords();//backup current m_rotatedDestCoords

  m_destRect.SetRect(0, 0, (float)capture->GetWidth(), (float)capture->GetHeight());
  syncDestRectToRotatedPoints();//syncs the changed destRect to m_rotatedDestCoords

  LPDIRECT3DSURFACE9 oldSurface;
  pD3DDevice->GetRenderTarget(0, &oldSurface);

  capture->BeginRender();
  if (capture->GetState() != CAPTURESTATE_FAILED)
  {
    pD3DDevice->BeginScene();
    Render(0);
    pD3DDevice->EndScene();
    capture->EndRender();
    succeeded = true;
  }

  pD3DDevice->SetRenderTarget(0, oldSurface);
  oldSurface->Release();

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

    if (!buf->Create(m_format, m_sourceWidth, m_sourceHeight))
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

bool CWinRenderer::Supports(EDEINTERLACEMODE mode)
{
  if(mode == VS_DEINTERLACEMODE_OFF
  || mode == VS_DEINTERLACEMODE_AUTO
  || mode == VS_DEINTERLACEMODE_FORCE)
    return true;

  return false;
}

bool CWinRenderer::Supports(EINTERLACEMETHOD method)
{
  if(method == VS_INTERLACEMETHOD_AUTO)
    return true;

  if (m_renderMethod == RENDER_DXVA)
  {
    if(method == VS_INTERLACEMETHOD_DXVA_BOB
    || method == VS_INTERLACEMETHOD_DXVA_BEST)
      return true;
  }

  if(m_format != RENDER_FMT_DXVA
  && (   method == VS_INTERLACEMETHOD_DEINTERLACE
      || method == VS_INTERLACEMETHOD_DEINTERLACE_HALF
      || method == VS_INTERLACEMETHOD_SW_BLEND))
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
      feature == RENDERFEATURE_CROP            ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_VERTICAL_SHIFT  ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_POSTPROCESS)
    return true;


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
      || method == VS_SCALINGMETHOD_SPLINE36_FAST
      || method == VS_SCALINGMETHOD_LANCZOS3_FAST
      || method == VS_SCALINGMETHOD_SPLINE36
      || method == VS_SCALINGMETHOD_LANCZOS3)
        return true;
    }
  }
  else if(m_renderMethod == RENDER_SW)
  {
    if(method == VS_SCALINGMETHOD_AUTO
    || method == VS_SCALINGMETHOD_NEAREST)
      return true;
    if(method == VS_SCALINGMETHOD_LINEAR
    && m_deviceCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR
    && m_deviceCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
      return true;
  }
  return false;
}

EINTERLACEMETHOD CWinRenderer::AutoInterlaceMethod()
{
  if (m_renderMethod == RENDER_DXVA)
    return VS_INTERLACEMETHOD_DXVA_BOB;
  else
    return VS_INTERLACEMETHOD_DEINTERLACE_HALF;
}

//============================================

YUVBuffer::~YUVBuffer()
{
  Release();
}

bool YUVBuffer::Create(ERenderFormat format, unsigned int width, unsigned int height)
{
  m_format = format;
  m_width = width;
  m_height = height;

  // Create the buffers in system memory and copy to D3DPOOL_DEFAULT:
  // - helps with lost devices. A buffer can be locked for dvdplayer without interfering.
  // - Dynamic + D3DPOOL_DEFAULT caused trouble for Intel i3 and some IGP. Bad sync/locking in the driver  I suppose
  // and Present failed every second time for the second video played.
  // - this is what D3D9 does behind the scenes anyway
  switch(m_format)
  {
  case RENDER_FMT_YUV420P10:
  case RENDER_FMT_YUV420P16:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width    , m_height    , 1, 0, D3DFMT_L16, D3DPOOL_SYSTEMMEM)
        || !planes[PLANE_U].texture.Create(m_width / 2, m_height / 2, 1, 0, D3DFMT_L16, D3DPOOL_SYSTEMMEM)
        || !planes[PLANE_V].texture.Create(m_width / 2, m_height / 2, 1, 0, D3DFMT_L16, D3DPOOL_SYSTEMMEM))
        return false;
      m_activeplanes = 3;
      break;
    }
  case RENDER_FMT_YUV420P:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width    , m_height    , 1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM)
        || !planes[PLANE_U].texture.Create(m_width / 2, m_height / 2, 1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM)
        || !planes[PLANE_V].texture.Create(m_width / 2, m_height / 2, 1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM))
        return false;
      m_activeplanes = 3;
      break;
    }
  case RENDER_FMT_NV12:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width    , m_height    , 1, 0, D3DFMT_L8, D3DPOOL_SYSTEMMEM)
        || !planes[PLANE_UV].texture.Create(m_width / 2, m_height / 2, 1, 0, D3DFMT_A8L8, D3DPOOL_SYSTEMMEM))
        return false;
      m_activeplanes = 2;
      break;
    }
  case RENDER_FMT_YUYV422:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width >> 1    , m_height    , 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM))
        return false;
      m_activeplanes = 1;
      break;
    }
  case RENDER_FMT_UYVY422:
    {
      if ( !planes[PLANE_Y].texture.Create(m_width >> 1    , m_height    , 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM))
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
  for(unsigned i = 0; i < m_activeplanes; i++)
  {
    planes[i].texture.Release();
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void YUVBuffer::StartRender()
{
  for(unsigned i = 0; i < m_activeplanes; i++)
  {
    if(planes[i].texture.Get() && planes[i].rect.pBits)
      if (!planes[i].texture.UnlockRect(0))
        CLog::Log(LOGERROR, __FUNCTION__" - failed to unlock texture %d", i);
    memset(&planes[i].rect, 0, sizeof(planes[i].rect));
  }
}

void YUVBuffer::StartDecode()
{
  for(unsigned i = 0; i < m_activeplanes; i++)
  {
    if(planes[i].texture.Get()
    && planes[i].texture.LockRect(0, &planes[i].rect, NULL, D3DLOCK_DISCARD) == false)
    {
      memset(&planes[i].rect, 0, sizeof(planes[i].rect));
      CLog::Log(LOGERROR, __FUNCTION__" - failed to lock texture %d into memory", i);
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
      wmemset((wchar_t*)planes[PLANE_Y].rect.pBits, 0,     planes[PLANE_Y].rect.Pitch *  m_height    / 2);
      wmemset((wchar_t*)planes[PLANE_U].rect.pBits, 32768, planes[PLANE_U].rect.Pitch * (m_height/2) / 2);
      wmemset((wchar_t*)planes[PLANE_V].rect.pBits, 32768, planes[PLANE_V].rect.Pitch * (m_height/2) / 2);
      break;
    }
  case RENDER_FMT_YUV420P10:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pBits, 0,   planes[PLANE_Y].rect.Pitch *  m_height    / 2);
      wmemset((wchar_t*)planes[PLANE_U].rect.pBits, 512, planes[PLANE_U].rect.Pitch * (m_height/2) / 2);
      wmemset((wchar_t*)planes[PLANE_V].rect.pBits, 512, planes[PLANE_V].rect.Pitch * (m_height/2) / 2);
      break;
    }
  case RENDER_FMT_YUV420P:
    {
      memset(planes[PLANE_Y].rect.pBits, 0,   planes[PLANE_Y].rect.Pitch *  m_height);
      memset(planes[PLANE_U].rect.pBits, 128, planes[PLANE_U].rect.Pitch * (m_height/2));
      memset(planes[PLANE_V].rect.pBits, 128, planes[PLANE_V].rect.Pitch * (m_height/2));
      break;
    }
  case RENDER_FMT_NV12:
    {
      memset(planes[PLANE_Y].rect.pBits, 0,   planes[PLANE_Y].rect.Pitch *  m_height);
      memset(planes[PLANE_UV].rect.pBits, 128, planes[PLANE_U].rect.Pitch * (m_height/2));
      break;
    }
  // YUY2, UYVY: wmemset to set a 16bit pattern, byte-swapped because x86 is LE
  case RENDER_FMT_YUYV422:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pBits, 0x8000, planes[PLANE_Y].rect.Pitch / 2 * m_height);
      break;
    }
  case RENDER_FMT_UYVY422:
    {
      wmemset((wchar_t*)planes[PLANE_Y].rect.pBits, 0x0080, planes[PLANE_Y].rect.Pitch / 2 * m_height);
      break;
    }

  }
}

//==================================

DXVABuffer::~DXVABuffer()
{
  Release();
}

void DXVABuffer::Release()
{
  id = 0;
}

void DXVABuffer::StartDecode()
{
  Release();
}
#endif
