/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <ppltasks.h>

#include "WinRenderer.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/FFmpeg.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "platform/win32/WIN32Util.h"
#include "RenderFactory.h"
#include "RenderFlags.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "ServiceBroker.h"
#include "system.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "VideoShaders/WinVideoFilter.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"

typedef struct
{
  RenderMethod  method;
  const char   *name;
} RenderMethodDetail;

static RenderMethodDetail RenderMethodDetails[] =
{
    { RENDER_SW     , "Software" },
    { RENDER_PS     , "Pixel Shaders" },
    { RENDER_DXVA   , "DXVA" },
    { RENDER_INVALID, nullptr }
};

static RenderMethodDetail *FindRenderMethod(RenderMethod m)
{
  for (unsigned i = 0; RenderMethodDetails[i].method != RENDER_INVALID; i++) {
    if (RenderMethodDetails[i].method == m)
      return &RenderMethodDetails[i];
  }
  return nullptr;
}

CBaseRenderer* CWinRenderer::Create(CVideoBuffer *buffer)
{
  return new CWinRenderer();
}

bool CWinRenderer::Register()
{
  VIDEOPLAYER::CRendererFactory::RegisterRenderer("default", CWinRenderer::Create);
  return true;
}

CWinRenderer::CWinRenderer() : CBaseRenderer()
  , m_bConfigured(false)
  , m_bUseHQScaler(false)
  , m_bFilterInitialized(false)
  , m_cmsOn(false)
  , m_clutLoaded(true)
  , m_toneMapping(false)
  , m_destWidth(0)
  , m_destHeight(0)
  , m_frameIdx(0)
  , m_iYV12RenderBuffer(0)
  , m_NumYV12Buffers(0)
  , m_neededBuffers(0)
  , m_iRequestedMethod(RENDER_METHOD_AUTO)
  , m_dxva_format(DXGI_FORMAT_UNKNOWN)
  , m_renderMethod(RENDER_PS)
  , m_bufferFormat(BUFFER_FMT_NONE)
  , m_scalingMethod(VS_SCALINGMETHOD_LINEAR)
  , m_scalingMethodGui(static_cast<ESCALINGMETHOD>(-1))
  , m_processor(nullptr)
  , m_sw_scale_ctx(nullptr)
  , m_colorShader(nullptr)
  , m_scalerShader(nullptr)
  , m_capture(nullptr)
  , m_pCLUTView(nullptr)
{
  m_format = AV_PIX_FMT_NONE;
  for (unsigned i = 0; i < NUM_BUFFERS; i++)
    m_renderBuffers[i].Release();

  m_colorManager = std::make_unique<CColorManager>();
  m_outputShader.reset();
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_useDithering = settings->GetBool("videoscreen.dither");
  m_ditherDepth = settings->GetInt("videoscreen.ditherdepth");

  PreInit();
}

CWinRenderer::~CWinRenderer()
{
  CWinRenderer::UnInit();
}

void CWinRenderer::ManageTextures()
{
  if( m_NumYV12Buffers < m_neededBuffers )
  {
    for(int i = m_NumYV12Buffers; i<m_neededBuffers;i++)
      CreateRenderBuffer(i);

    m_NumYV12Buffers = m_neededBuffers;
  }
  else if( m_NumYV12Buffers > m_neededBuffers )
  {
    for (int i = m_NumYV12Buffers - 1; i >= m_neededBuffers; i--)
      DeleteRenderBuffer(i);

    m_NumYV12Buffers = m_neededBuffers;
    m_iYV12RenderBuffer = m_iYV12RenderBuffer % m_NumYV12Buffers;
  }
}

void CWinRenderer::SelectRenderMethod()
{
  EBufferFormat dxFormat = SelectBufferFormat(m_format, RENDER_DXVA);
  EBufferFormat psFormat = SelectBufferFormat(m_format, RENDER_PS);
  // modern drivers allow using HW pic in shaders
  bool allowChangeMethod = dxFormat == psFormat && m_iRequestedMethod != RENDER_SW;

  // old drivers + HW decoded picture -> we must force DXVA render method
  if (!allowChangeMethod && m_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    CLog::Log(LOGNOTICE, "%s: rendering method forced to DXVA processor.", __FUNCTION__);
    m_renderMethod = RENDER_DXVA;
    if (!m_processor || !m_processor->Open(m_sourceWidth, m_sourceHeight))
    {
      CLog::Log(LOGNOTICE, "%s: unable to open DXVA processor.", __FUNCTION__);
      if (m_processor)
        m_processor->Close();
      m_renderMethod = RENDER_INVALID;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s: requested render method: %d", __FUNCTION__, m_iRequestedMethod);
    switch (m_iRequestedMethod)
    {
    case RENDER_METHOD_DXVA:
    {
      // skip if format isn't DXVA compatible.
      if (dxFormat >= BUFFER_FMT_D3D11_BYPASS)
      {
        if (m_processor && m_processor->Open(m_sourceWidth, m_sourceHeight))
        {
          m_renderMethod = RENDER_DXVA;
          break;
        }
        allowChangeMethod = false;
        CLog::Log(LOGNOTICE, "%s: unable to open DXVA processor.", __FUNCTION__);
        if (m_processor)
          m_processor->Close();
      }
    }
    // fallback to auto
    case RENDER_METHOD_AUTO:
      if (allowChangeMethod)
      {
        // for modern drivers select method depends on input
        // for HW decoded or interlaced picture prefer DXVA method.
        if (m_format == AV_PIX_FMT_D3D11VA_VLD || m_iFlags & DVP_FLAG_INTERLACED)
        {
          if (m_processor && m_processor->Open(m_sourceWidth, m_sourceHeight))
          {
            m_renderMethod = RENDER_DXVA;
            break;
          }
          CLog::Log(LOGNOTICE, "%s: unable to open DXVA processor", __FUNCTION__);
          if (m_processor)
            m_processor->Close();
        }
      }
    // drop through to pixel shader
    case RENDER_METHOD_D3D_PS:
    {
      CTestShader shader;
      if (shader.Create())
      {
        m_renderMethod = RENDER_PS;
        break;
      }
      // this is something out of the ordinary
      CLog::Log(LOGNOTICE, "%s: unable to load test shader - D3D installation is most likely incomplete, falling back to SW mode.", __FUNCTION__);
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, "DirectX", g_localizeStrings.Get(2101));
    }
    // drop through to software
    case RENDER_METHOD_SOFTWARE:
    default:
      // so we'll do the color conversion in software.
      m_renderMethod = RENDER_SW;
      break;
    }
  }

  m_bufferFormat = SelectBufferFormat(m_format, m_renderMethod);
  m_frameIdx = 0;

  RenderMethodDetail *rmdet = FindRenderMethod(m_renderMethod);
  CLog::Log(LOGDEBUG, "%s: selected render method %d: %s", __FUNCTION__, m_renderMethod, rmdet != nullptr ? rmdet->name : "unknown");
  CLog::Log(LOGDEBUG, "%s: selected buffer format %d", __FUNCTION__, m_bufferFormat);
}

bool CWinRenderer::Configure(const VideoPicture &picture, float fps, unsigned int orientation)
{
  m_sourceWidth       = picture.iWidth;
  m_sourceHeight      = picture.iHeight;
  m_renderOrientation = orientation;
  // need to recreate textures
  m_NumYV12Buffers    = 0;
  m_iYV12RenderBuffer = 0;
  // reinitialize the filters/shaders
  m_bFilterInitialized = false;

  m_fps = fps;
  m_iFlags = GetFlagsChromaPosition(picture.chroma_position)
           | GetFlagsColorMatrix(picture.color_space, picture.iWidth, picture.iHeight)
           | GetFlagsColorPrimaries(picture.color_primaries)
           | GetFlagsStereoMode(picture.stereoMode);

  m_srcPrimaries = GetSrcPrimaries(static_cast<AVColorPrimaries>(picture.color_primaries), picture.iWidth, picture.iHeight);
  m_format = picture.videoBuffer->GetFormat();
  if (m_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    DXVA::CDXVAOutputBuffer *dxvaBuf = static_cast<DXVA::CDXVAOutputBuffer*>(picture.videoBuffer);
    m_dxva_format = dxvaBuf->format;
  }

  // calculate the input frame aspect ratio
  CalculateFrameAspectRatio(picture.iDisplayWidth, picture.iDisplayHeight);
  SetViewMode(m_videoSettings.m_ViewMode);
  ManageRenderArea();

  SelectRenderMethod();
  m_bConfigured = true;

  // load 3DLUT
  ColorManagmentUpdate();

  return true;
}

int CWinRenderer::NextBuffer() const
{
  if(m_NumYV12Buffers)
    return (m_iYV12RenderBuffer + 1) % m_NumYV12Buffers;
  return -1;
}

void CWinRenderer::AddVideoPicture(const VideoPicture &picture, int index)
{
  m_renderBuffers[index].AppendPicture(picture);
  m_renderBuffers[index].frameIdx = m_frameIdx;
  m_frameIdx += 2;
}

void CWinRenderer::Update()
{
  if (!m_bConfigured)
    return;
  ManageRenderArea();
  ManageTextures();
}

void CWinRenderer::RenderUpdate(int index, int index2, bool clear, unsigned int flags, unsigned int alpha)
{
  m_iYV12RenderBuffer = index;

  if (clear)
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear(DX::Windowing()->UseLimitedColor() ? 0x101010 : 0);

  if (!m_bConfigured)
    return;

  DX::Windowing()->SetAlphaBlendEnable(alpha < 255);
  ManageTextures();
  ManageRenderArea();
  Render(flags, DX::Windowing()->GetBackBuffer());
  DX::Windowing()->SetAlphaBlendEnable(true);
}

void CWinRenderer::PreInit()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  m_bConfigured = false;
  UnInit();

  m_iRequestedMethod = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD);

  m_processor = std::make_unique<DXVA::CProcessorHD>();
  if (!m_processor->PreInit())
  {
    CLog::LogF(LOGNOTICE, "could not init DXVA processor - skipping.");
    m_processor.reset();;
  }
}

void CWinRenderer::UnInit()
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  m_colorShader.reset();
  m_scalerShader.reset();

  m_bConfigured = false;
  m_bFilterInitialized = false;

  for(int i = 0; i < NUM_BUFFERS; i++)
    DeleteRenderBuffer(i);

  m_NumYV12Buffers = 0;

  if (m_sw_scale_ctx)
  {
    sws_freeContext(m_sw_scale_ctx);
    m_sw_scale_ctx = nullptr;
  }

  if (m_processor)
  {
    m_processor.reset();
  }
  m_pCLUTView = nullptr;
  m_outputShader.reset();
}

bool CWinRenderer::Flush(bool saveBuffers)
{
  if (!m_bConfigured)
    return false;

  for (int i = 0; i < NUM_BUFFERS; i++)
    DeleteRenderBuffer(i);

  m_iYV12RenderBuffer = 0;
  m_NumYV12Buffers = 0;
  m_bFilterInitialized = false;

  return false;
}

bool CWinRenderer::CreateIntermediateRenderTarget(unsigned int width, unsigned int height, bool dynamic)
{
  DXGI_FORMAT format = DX::Windowing()->GetBackBuffer()->GetFormat();

  // don't create new one if it exists with requested size and format
  if ( m_IntermediateTarget.Get() && m_IntermediateTarget.GetFormat() == format
    && m_IntermediateTarget.GetWidth() == width && m_IntermediateTarget.GetHeight() == height)
    return true;

  if (m_IntermediateTarget.Get())
    m_IntermediateTarget.Release();

  CLog::Log(LOGDEBUG, "%s: format %i.", __FUNCTION__, format);

  if (!m_IntermediateTarget.Create(width, height, 1, dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT, format))
  {
    CLog::Log(LOGERROR, "%s: intermediate render target creation failed.", __FUNCTION__);
    return false;
  }
  return true;
}

template<typename T>
bool Contains(std::vector<T> vector, T item)
{
  return find(vector.begin(), vector.end(), item) != vector.end();
}

EBufferFormat CWinRenderer::SelectBufferFormat(AVPixelFormat format, const RenderMethod method) const
{
  AVPixelFormat decoderFormat = format;
  if (format == AV_PIX_FMT_D3D11VA_VLD)
  {
    switch(m_dxva_format)
    {
    case DXGI_FORMAT_NV12:
      decoderFormat = AV_PIX_FMT_NV12;
      break;
    case DXGI_FORMAT_P010:
      decoderFormat = AV_PIX_FMT_P010;
      break;
    case DXGI_FORMAT_P016:
      decoderFormat = AV_PIX_FMT_P016;
      break;
    default:
      break;
    }
  }

  // check shared formats and processor formats
  if ( method != RENDER_SW && Contains(DX::Windowing()->m_sharedFormats, decoderFormat)
    || (method == RENDER_DXVA && Contains(DX::Windowing()->m_processorFormats, decoderFormat)) )
  {
    switch (format)
    {
    case AV_PIX_FMT_D3D11VA_VLD:
      return BUFFER_FMT_D3D11_BYPASS;
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_YUV420P:
      return BUFFER_FMT_D3D11_NV12;
    case AV_PIX_FMT_YUV420P10:
    case AV_PIX_FMT_P010:
      return BUFFER_FMT_D3D11_P010;
    case AV_PIX_FMT_YUV420P16:
    case AV_PIX_FMT_P016:
      return BUFFER_FMT_D3D11_P016;
    default:
      return BUFFER_FMT_NONE;
    }
  }

  // fit decoder format for shaders
  if (format == AV_PIX_FMT_D3D11VA_VLD)
  {
    switch (m_dxva_format)
    {
    case DXGI_FORMAT_P010:
      decoderFormat = AV_PIX_FMT_YUV420P10;
      break;
    case DXGI_FORMAT_P016:
      decoderFormat = AV_PIX_FMT_YUV420P16;
      break;
    default:
      break;
    }
  }

  // check common formats (SW rendering or win7)
  if ( method == RENDER_SW || (method == RENDER_PS && Contains(DX::Windowing()->m_shaderFormats, decoderFormat)))
  {
    switch (format)
    {
    case AV_PIX_FMT_D3D11VA_VLD:
    {
      switch(m_dxva_format)
      {
        // @todo implement support of these formats
        //case DXGI_FORMAT_P010:
        //  return BUFFER_FMT_YUV420P10;
        //case DXGI_FORMAT_P016:
        //  return BUFFER_FMT_YUV420P16;
        case DXGI_FORMAT_NV12:
        default:
          return BUFFER_FMT_NV12;
      }
    }
    case AV_PIX_FMT_NV12:
      return BUFFER_FMT_NV12;
    case AV_PIX_FMT_YUV420P:
      return BUFFER_FMT_YUV420P;
    case AV_PIX_FMT_YUV420P10:
      return BUFFER_FMT_YUV420P10;
    case AV_PIX_FMT_YUV420P16:
      return BUFFER_FMT_YUV420P16;
    // is they still used?
    case AV_PIX_FMT_YUYV422:
      return BUFFER_FMT_YUYV422;
    case AV_PIX_FMT_UYVY422:
      return BUFFER_FMT_UYVY422;
    default:
      return BUFFER_FMT_NONE;
    }
  }

  return BUFFER_FMT_NONE;
}

void CWinRenderer::SelectSWVideoFilter()
{
  CreateIntermediateRenderTarget(m_sourceWidth, m_sourceHeight, true);
}

void CWinRenderer::SelectPSVideoFilter()
{
  m_bUseHQScaler = false;

  switch (m_scalingMethod)
  {
  case VS_SCALINGMETHOD_CUBIC:
  case VS_SCALINGMETHOD_LANCZOS2:
  case VS_SCALINGMETHOD_SPLINE36_FAST:
  case VS_SCALINGMETHOD_LANCZOS3_FAST:
  case VS_SCALINGMETHOD_SPLINE36:
  case VS_SCALINGMETHOD_LANCZOS3:
    m_bUseHQScaler = true;
    break;

  default:
    break;
  }

  if (m_scalingMethod == VS_SCALINGMETHOD_AUTO)
  {
    const bool scaleSD = m_sourceHeight < 720 && m_sourceWidth < 1280;
    const bool scaleUp = static_cast<int>(m_sourceHeight) < CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()
                && static_cast<int>(m_sourceWidth) < CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
    const bool scaleFps = m_fps < CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoAutoScaleMaxFps + 0.01f;

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
    else
      m_scalingMethod = VS_SCALINGMETHOD_LINEAR;
  }
  if (m_renderOrientation)
    m_bUseHQScaler = false;
}

void CWinRenderer::UpdatePSVideoFilter()
{
  m_scalerShader.reset();

  if (m_bUseHQScaler)
  {
    // First try the more efficient two pass convolution scaler
    m_scalerShader = std::make_unique<CConvolutionShaderSeparable>();

    if (!m_scalerShader->Create(m_scalingMethod, m_outputShader.get()))
    {
      m_scalerShader.reset();
      CLog::Log(LOGNOTICE, "%s: two pass convolution shader init problem, falling back to one pass.", __FUNCTION__);
    }

    // Fallback on the one pass version
    if (!m_scalerShader)
    {
      m_scalerShader = std::make_unique<CConvolutionShader1Pass>();

      if (!m_scalerShader->Create(m_scalingMethod, m_outputShader.get()))
      {
        m_scalerShader.reset();
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(34400), g_localizeStrings.Get(34401));
        m_bUseHQScaler = false;
      }
    }
  }

  if (m_renderMethod != RENDER_DXVA || m_bUseHQScaler)
    CreateIntermediateRenderTarget(m_sourceWidth, m_sourceHeight, false);

  m_colorShader.reset();

  if (m_renderMethod == RENDER_DXVA)
  {
    // we'd use m_IntermediateTarget as rendering target for possible anaglyph stereo with dxva processor.
    if (!m_bUseHQScaler)
      CreateIntermediateRenderTarget(m_destWidth, m_destHeight, false);
    // When using DXVA, we are already setup at this point, color shader is not needed
    return;
  }

  m_colorShader = std::make_unique<CYUV2RGBShader>();
  if (!m_colorShader->Create(m_bufferFormat, AVCOL_PRI_BT709, m_srcPrimaries))
  {
    if (m_bUseHQScaler)
    {
      m_IntermediateTarget.Release();
      m_scalerShader.reset();
    }
    m_colorShader.reset();
    m_bUseHQScaler = false;

    // we're in big trouble - fallback to sw method
    m_renderMethod = RENDER_SW;
    EBufferFormat oldFormat = m_bufferFormat;
    m_bufferFormat = SelectBufferFormat(m_format, m_renderMethod);
    if (oldFormat != m_bufferFormat && m_NumYV12Buffers > 0)
    {
      m_NumYV12Buffers = 0;
      ManageTextures();
    }
    SelectSWVideoFilter();
  }
}

void CWinRenderer::UpdateVideoFilter()
{
  bool cmsChanged = m_cmsOn != m_colorManager->IsEnabled()
                 || m_cmsOn && !m_colorManager->CheckConfiguration(m_cmsToken, m_iFlags);
  cmsChanged &= m_clutLoaded;

  if (m_scalingMethodGui == m_videoSettings.m_ScalingMethod
   && m_bFilterInitialized && !cmsChanged)
    return;

  m_bFilterInitialized = true;
  m_scalingMethodGui = m_videoSettings.m_ScalingMethod;
  m_scalingMethod    = m_scalingMethodGui;

  if (!Supports(m_scalingMethod))
  {
    CLog::Log(LOGWARNING, "%s: chosen scaling method %d is not supported by renderer", __FUNCTION__, static_cast<int>(m_scalingMethod));
    m_scalingMethod = VS_SCALINGMETHOD_AUTO;
  }

  if (cmsChanged)
    ColorManagmentUpdate();

  if (cmsChanged || !m_outputShader)
  {
    m_outputShader = std::make_unique<COutputShader>();
    if (!m_outputShader->Create(m_cmsOn, m_useDithering, m_ditherDepth, m_toneMapping))
    {
      CLog::Log(LOGDEBUG, "%s: Unable to create output shader.", __FUNCTION__);
      m_outputShader.reset();
    }
    else if (m_pCLUTView && m_CLUTSize)
      m_outputShader->SetCLUT(m_CLUTSize, m_pCLUTView.Get());
  }

  RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();
  if (!res.bFullScreen)
    res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo(RES_DESKTOP);

  m_destWidth = res.iScreenWidth;
  m_destHeight = res.iScreenHeight;

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

void CWinRenderer::Render(DWORD flags, CD3DTexture* target)
{
  CRenderBuffer& buf = m_renderBuffers[m_iYV12RenderBuffer];
  if (!buf.loaded)
  {
    if (!buf.UploadBuffer())
      return;
  }

  AVColorPrimaries srcPrim = GetSrcPrimaries(buf.primaries, buf.GetWidth(), buf.GetHeight());
  if (srcPrim != m_srcPrimaries)
  {
    m_srcPrimaries = srcPrim;
    m_bFilterInitialized = false;
  }

  bool toneMap = false;
  if (m_videoSettings.m_ToneMapMethod != VS_TONEMAPMETHOD_OFF)
  {
    if (buf.hasLightMetadata || (buf.hasDisplayMetadata && buf.displayMetadata.has_luminance))
      toneMap = true;
  }
  if (toneMap != m_toneMapping)
  {
    m_outputShader.reset();
    m_bFilterInitialized = false;
  }
  m_toneMapping = toneMap;

  UpdateVideoFilter();

  switch (m_renderMethod)
  {
  case RENDER_DXVA:
    RenderHW(flags, target);
    break;
  case RENDER_PS:
    RenderPS(target);
    break;
  case RENDER_SW:
    RenderSW(target);
    break;
  default:
    return;
  }

  if (m_bUseHQScaler)
    RenderHQ(target);

  DX::Windowing()->ApplyStateBlock();
}

void CWinRenderer::RenderSW(CD3DTexture* target)
{
  // if creation failed
  if (!m_outputShader)
    return;

  // Don't know where this martian comes from but it can happen in the initial frames of a video
  if (m_destRect.x1 < 0 && m_destRect.x2 < 0
   || m_destRect.y1 < 0 && m_destRect.y2 < 0)
    return;

  // fit format in case of hw decoder
  AVPixelFormat decoderFormat = m_format;
  if (m_format == AV_PIX_FMT_D3D11VA_VLD)
  {
    switch (m_dxva_format)
    {
    case DXGI_FORMAT_NV12:
      decoderFormat = AV_PIX_FMT_NV12;
      break;
    case DXGI_FORMAT_P010:
      decoderFormat = AV_PIX_FMT_YUV420P10;
      break;
    case DXGI_FORMAT_P016:
      decoderFormat = AV_PIX_FMT_YUV420P16;
      break;
    default:
      break;
    }
  }

  // 1. convert yuv to rgb
  m_sw_scale_ctx = sws_getCachedContext(m_sw_scale_ctx,
                                        m_sourceWidth, m_sourceHeight, decoderFormat,
                                        m_sourceWidth, m_sourceHeight, AV_PIX_FMT_BGRA,
                                        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  CRenderBuffer& buf = m_renderBuffers[m_iYV12RenderBuffer];

  uint8_t* src[YuvImage::MAX_PLANES];
  int srcStride[YuvImage::MAX_PLANES];

  for (unsigned int idx = 0; idx < buf.GetActivePlanes(); idx++)
    buf.MapPlane(idx, reinterpret_cast<void**>(&src[idx]), &srcStride[idx]);

  D3D11_MAPPED_SUBRESOURCE destlr;
  if (!m_IntermediateTarget.LockRect(0, &destlr, D3D11_MAP_WRITE_DISCARD))
    CLog::Log(LOGERROR, "%s: failed to lock swtarget texture into memory.", __FUNCTION__);

  uint8_t *dst[] = { static_cast<uint8_t*>(destlr.pData), nullptr, nullptr };
  int dstStride[] = { static_cast<int>(destlr.RowPitch), 0, 0 };

  sws_scale(m_sw_scale_ctx, src, srcStride, 0, m_sourceHeight, dst, dstStride);

  for (unsigned int idx = 0; idx < buf.GetActivePlanes(); idx++)
    buf.UnmapPlane(idx);

  if (!m_IntermediateTarget.UnlockRect(0))
    CLog::Log(LOGERROR, "%s: failed to unlock swtarget texture.", __FUNCTION__);

  // 2. output to display

  m_outputShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata, buf.hasLightMetadata, buf.lightMetadata);
  m_outputShader->SetToneMapParam(m_videoSettings.m_ToneMapParam);
  m_outputShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_sourceRect, m_rotatedDestCoords, target,
                         DX::Windowing()->UseLimitedColor(), m_videoSettings.m_Contrast * 0.01f, m_videoSettings.m_Brightness * 0.01f);
}

void CWinRenderer::RenderPS(CD3DTexture* target)
{
  CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(m_sourceWidth), static_cast<float>(m_sourceHeight));

  // reset scissors
  DX::Windowing()->ResetScissors();
  // reset view port
  DX::DeviceResources::Get()->GetD3DContext()->RSSetViewports(1, &viewPort);

  CRenderBuffer& buf = m_renderBuffers[m_iYV12RenderBuffer];

  CPoint srcPoints[4];
  m_sourceRect.GetQuad(srcPoints);
  // set converter params
  m_colorShader->SetParams(m_videoSettings.m_Contrast, m_videoSettings.m_Brightness, DX::Windowing()->UseLimitedColor());
  m_colorShader->SetColParams(buf.color_space, buf.bits, !buf.full_range, buf.texBits);
  // convert YUV -> RGB
  m_colorShader->Render(m_sourceRect, srcPoints, &buf, &m_IntermediateTarget);

  if (!m_bUseHQScaler)
  {
    // second pass (bilinear scaling)
    // select destination rectangle
    CPoint destPoints[4];
    if (m_renderOrientation)
    {
      for (size_t i = 0; i < 4; i++)
        destPoints[i] = m_rotatedDestCoords[i];
    }
    else
    {
      CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(m_destRect).GetQuad(destPoints);
    }
    // set params
    m_outputShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata, buf.hasLightMetadata, buf.lightMetadata);
    m_outputShader->SetToneMapParam(m_videoSettings.m_ToneMapParam);

    viewPort.Width = static_cast<float>(target->GetWidth());
    viewPort.Height = static_cast<float>(target->GetHeight());
    // set viewport to the whole target
    DX::DeviceResources::Get()->GetD3DContext()->RSSetViewports(1, &viewPort);
    // restore scissors
    DX::Windowing()->SetScissors(CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors()));
    // render frame
    m_outputShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_sourceRect, destPoints, target);
  }
  // Restore our view port.
  DX::Windowing()->RestoreViewPort();
}

void CWinRenderer::RenderHQ(CD3DTexture* target)
{
  m_scalerShader->Render(m_IntermediateTarget, m_sourceWidth, m_sourceHeight, m_destWidth, m_destHeight
                       , m_sourceRect, CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(m_destRect)
                       , false, target);
}

void CWinRenderer::RenderHW(DWORD flags, CD3DTexture* target)
{
  CRenderBuffer& buf = m_renderBuffers[m_iYV12RenderBuffer];
  if ( buf.format != BUFFER_FMT_D3D11_BYPASS
    && buf.format != BUFFER_FMT_D3D11_NV12
    && buf.format != BUFFER_FMT_D3D11_P010
    && buf.format != BUFFER_FMT_D3D11_P016)
    return;

  if (!buf.loaded)
    return;

  int past = 0;
  int future = 0;

  CRenderBuffer* views[8];
  memset(views, 0, 8 * sizeof(CRenderBuffer*));
  views[2] = &buf;

  // set future frames
  while (future < 2)
  {
    bool found = false;
    for (int i = 0; i < m_NumYV12Buffers; i++)
    {
      if (m_renderBuffers[i].frameIdx == buf.frameIdx + (future*2 + 2))
      {
        // a future frame may not be loaded yet
        if (m_renderBuffers[i].loaded || m_renderBuffers[i].UploadBuffer())
        {
          views[1 - future++] = &m_renderBuffers[i];
          found = true;
          break;
        }
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
      if (m_renderBuffers[i].frameIdx == buf.frameIdx - (past*2 + 2))
      {
        if (m_renderBuffers[i].loaded)
        {
          views[3 + past++] = &m_renderBuffers[i];
          found = true;
          break;
        }
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
    destRect = m_bUseHQScaler ? m_sourceRect : CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(m_destRect);
    break;
  }

  CRect src = m_sourceRect, dst = destRect;
  CRect targetRect = CRect(0.0f, 0.0f,
                       static_cast<float>(m_IntermediateTarget.GetWidth()),
                       static_cast<float>(m_IntermediateTarget.GetHeight()));

  if (target != DX::Windowing()->GetBackBuffer())
  {
    // rendering capture
    targetRect.x2 = static_cast<float>(target->GetWidth());
    targetRect.y2 = static_cast<float>(target->GetHeight());
  }
  CWIN32Util::CropSource(src, dst, targetRect, m_renderOrientation);

  m_processor->Render(src, dst, m_IntermediateTarget.Get(), views, flags, buf.frameIdx, m_renderOrientation,
                      m_videoSettings.m_Contrast, m_videoSettings.m_Brightness);

  if (!m_bUseHQScaler)
  {
    if ( CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_SPLIT_HORIZONTAL
      || CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_SPLIT_VERTICAL)
    {
      CD3DTexture *backBuffer = DX::Windowing()->GetBackBuffer();
      CD3D11_VIEWPORT bbSize(0.f, 0.f, static_cast<float>(backBuffer->GetWidth()), static_cast<float>(backBuffer->GetHeight()));
      DX::DeviceResources::Get()->GetD3DContext()->RSSetViewports(1, &bbSize);
    }

    // render frame
    m_outputShader->SetDisplayMetadata(buf.hasDisplayMetadata, buf.displayMetadata, buf.hasLightMetadata, buf.lightMetadata);
    m_outputShader->SetToneMapParam(m_videoSettings.m_ToneMapParam);
    m_outputShader->Render(m_IntermediateTarget, m_destWidth, m_destHeight, dst, dst, target);
    DX::Windowing()->RestoreViewPort();
  }
}

bool CWinRenderer::RenderCapture(CRenderCapture* capture)
{
  if (!m_bConfigured || m_NumYV12Buffers == 0)
    return false;

  bool succeeded = false;

  CRect saveSize = m_destRect;
  saveRotatedCoords();//backup current m_rotatedDestCoords

  m_destRect.SetRect(0, 0, static_cast<float>(capture->GetWidth()), static_cast<float>(capture->GetHeight()));
  syncDestRectToRotatedPoints();//syncs the changed destRect to m_rotatedDestCoords

  capture->BeginRender();
  if (capture->GetState() != CAPTURESTATE_FAILED)
  {
    Render(0, capture->GetTarget());
    capture->EndRender();
    succeeded = true;
  }

  m_destRect = saveSize;
  restoreRotatedCoords();//restores the previous state of the rotated dest coords

  return succeeded;
}

//********************************************************************************************************
// YV12 Texture creation, deletion, copying + clearing
//********************************************************************************************************
void CWinRenderer::DeleteRenderBuffer(int index)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  ReleaseBuffer(index);
  m_renderBuffers[index].Release();
}

bool CWinRenderer::CreateRenderBuffer(int index)
{
  CSingleLock lock(CServiceBroker::GetWinSystem()->GetGfxContext());
  DeleteRenderBuffer(index);

  if (!m_renderBuffers[index].CreateBuffer(m_bufferFormat, m_sourceWidth, m_sourceHeight, m_renderMethod == RENDER_SW))
  {
    CLog::Log(LOGERROR, "%s: unable to create video buffer %i", __FUNCTION__, index);
    m_renderBuffers[index].Release();
    return false;
  }
  m_renderBuffers[index].Lock();
  m_renderBuffers[index].Clear();

  CLog::Log(LOGDEBUG, "%s: created video buffer %i", __FUNCTION__, index);
  return true;
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
      feature == RENDERFEATURE_POSTPROCESS     ||
      feature == RENDERFEATURE_TONEMAP)
    return true;

  return false;
}

bool CWinRenderer::Supports(ESCALINGMETHOD method)
{
  if (method == VS_SCALINGMETHOD_AUTO)
    return true;

  if (method == VS_SCALINGMETHOD_LINEAR && m_renderMethod != RENDER_DXVA)
    return true;

  if (method == VS_SCALINGMETHOD_DXVA_HARDWARE && m_renderMethod == RENDER_DXVA)
    return true;

  if (m_renderMethod == RENDER_PS || m_renderMethod == RENDER_DXVA)
  {
    if (DX::DeviceResources::Get()->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_9_3 && !m_renderOrientation)
    {
      if (method == VS_SCALINGMETHOD_CUBIC
       || method == VS_SCALINGMETHOD_LANCZOS2
       || method == VS_SCALINGMETHOD_SPLINE36_FAST
       || method == VS_SCALINGMETHOD_LANCZOS3_FAST
       || method == VS_SCALINGMETHOD_SPLINE36
       || method == VS_SCALINGMETHOD_LANCZOS3)
      {
        // if scaling is below level, avoid hq scaling
        float scaleX = fabs((static_cast<float>(m_sourceWidth) - m_destRect.Width())/m_sourceWidth)*100;
        float scaleY = fabs((static_cast<float>(m_sourceHeight) - m_destRect.Height())/m_sourceHeight)*100;
        int minScale = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_HQSCALERS);
        if (scaleX < minScale && scaleY < minScale)
          return false;
        return true;
      }
    }
  }
  
  return false;
}

bool CWinRenderer::WantsDoublePass()
{
  if (m_renderMethod == RENDER_DXVA)
    return true;

  return false;
}

bool CWinRenderer::ConfigChanged(const VideoPicture& picture)
{
  if (picture.videoBuffer->GetFormat() != m_format)
    return true;

  return false;
}

CRenderInfo CWinRenderer::GetRenderInfo()
{
  CRenderInfo info;
  info.formats =
  {
    AV_PIX_FMT_D3D11VA_VLD,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_P010,
    AV_PIX_FMT_P016,
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_YUV420P16
  };
  info.max_buffer_size = NUM_BUFFERS;
  if (m_renderMethod == RENDER_DXVA && m_processor)
  {
    int buffers = m_processor->Size() + m_processor->PastRefs(); // extra buffers for past refs
    info.optimal_buffer_size = std::min(NUM_BUFFERS, buffers);
    if (m_format != AV_PIX_FMT_D3D11VA_VLD)
      info.m_deintMethods.push_back(VS_INTERLACEMETHOD_DXVA_AUTO);
  }
  else
    info.optimal_buffer_size = 4;
  return info;
}

void CWinRenderer::ReleaseBuffer(int idx)
{
  m_renderBuffers[idx].ReleasePicture();
}

bool CWinRenderer::NeedBuffer(int idx)
{
  // check if processor wants to keep past frames
  if (m_renderMethod == RENDER_DXVA && m_processor)
  {
    if (m_renderBuffers[idx].loaded)
    {
      const int numPast = m_processor->PastRefs();
      if (m_renderBuffers[idx].pictureFlags & DVP_FLAG_INTERLACED &&
          m_renderBuffers[idx].frameIdx + numPast * 2 >=
              m_renderBuffers[m_iYV12RenderBuffer].frameIdx)
        return true;
    }
  }
  return false;
}

// Color management helpers

void CWinRenderer::ColorManagmentUpdate()
{
  if (m_colorManager->IsEnabled())
  {
    if (!m_colorManager->CheckConfiguration(m_cmsToken, m_iFlags))
    {
      CLog::Log(LOGDEBUG, "%s: CMS configuration changed, reload LUT", __FUNCTION__);
      LoadCLUT();
    }
    m_cmsOn = true;
  }
  else
  {
    m_cmsOn = false;
  }
}

bool CWinRenderer::LoadCLUT()
{
  m_CLUTSize = 0;
  m_clutLoaded = false;

  auto loadLutTask = Concurrency::create_task([this]{
    // load 3DLUT data
    int clutSize, dataSize;
    if (!CColorManager::Get3dLutSize(CMS_DATA_FMT_RGBA, &clutSize, &dataSize))
      return 0;

    uint16_t* clutData = static_cast<uint16_t*>(_aligned_malloc(dataSize, 16));
    bool success = m_colorManager->GetVideo3dLut(m_iFlags, &m_cmsToken, CMS_DATA_FMT_RGBA, clutSize, clutData);
    if (success)
    {
      success = COutputShader::CreateCLUTView(clutSize, clutData, false, m_pCLUTView.ReleaseAndGetAddressOf());
    }
    else
      CLog::Log(LOGERROR, "%s: unable to loading the 3dlut data.", __FUNCTION__);

    _aligned_free(clutData);
    if (!success)
      return 0;

    return clutSize;
  });

  loadLutTask.then([&](int clutSize){
    m_CLUTSize = clutSize;
    if (m_outputShader)
        m_outputShader->SetCLUT(m_CLUTSize, m_pCLUTView.Get());
    m_clutLoaded = true;
  });
  return true;
}

AVColorPrimaries CWinRenderer::GetSrcPrimaries(AVColorPrimaries srcPrimaries, unsigned int width, unsigned int height) const
{
  AVColorPrimaries ret = srcPrimaries;
  if (ret == AVCOL_PRI_UNSPECIFIED)
  {
    if (width > 1024 || height >= 600)
      ret = AVCOL_PRI_BT709;
    else
      ret = AVCOL_PRI_BT470BG;
  }
  return ret;
}
