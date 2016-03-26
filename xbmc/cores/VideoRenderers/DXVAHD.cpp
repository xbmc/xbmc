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

// setting that here because otherwise SampleFormat is defined to AVSampleFormat
// which we don't use here
#define FF_API_OLD_SAMPLE_FMT 0
#define DEFAULT_STREAM_INDEX (0)

#include <dxva2api.h>
#include <windows.h>
#include "DXVAHD.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "RenderFlags.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "utils/Log.h"
#include "utils/win32/memcpy_sse2.h"
#include "win32/WIN32Util.h"
#include "windowing/WindowingFactory.h"

using namespace DXVA;

#define LOGIFERROR(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, "%s - failed executing "#a" at line %d with error %x", __FUNCTION__, __LINE__, res); \
  } \
} while(0);

CProcessorHD::CProcessorHD()
{
  m_pVideoDevice = nullptr;
  m_pVideoContext = nullptr;
  m_pEnumerator = nullptr;
  m_pVideoProcessor = nullptr;
  g_Windowing.Register(this);

  m_context = nullptr;
  m_width = 0;
  m_height = 0;
}

CProcessorHD::~CProcessorHD()
{
  g_Windowing.Unregister(this);
  UnInit();
}

void CProcessorHD::UnInit()
{
  CSingleLock lock(m_section);
  Close();
  SAFE_RELEASE(m_pVideoDevice);
  SAFE_RELEASE(m_pVideoContext);
}

void CProcessorHD::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_pEnumerator);
  SAFE_RELEASE(m_pVideoProcessor);
  SAFE_RELEASE(m_context);
}

bool CProcessorHD::UpdateSize(const DXVA2_VideoDesc& dsc)
{
  return true;
}

bool CProcessorHD::PreInit()
{
  SAFE_RELEASE(m_pVideoDevice);
  SAFE_RELEASE(m_pVideoContext);

  if ( FAILED(g_Windowing.Get3D11Device()->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&m_pVideoDevice)))
    || FAILED(g_Windowing.GetImmediateContext()->QueryInterface(__uuidof(ID3D11VideoContext), reinterpret_cast<void**>(&m_pVideoContext))))
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - failed to get video devices.");
    return false;
  }

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc1;
  ZeroMemory(&desc1, sizeof(D3D11_VIDEO_PROCESSOR_CONTENT_DESC));
  desc1.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  desc1.InputWidth = 640;
  desc1.InputHeight = 480;
  desc1.OutputWidth = 640;
  desc1.OutputHeight = 480;
  desc1.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  // try to create video enum
  if (FAILED(m_pVideoDevice->CreateVideoProcessorEnumerator(&desc1, &m_pEnumerator)))
  {
    CLog::Log(LOGWARNING, "%s - failed to create Video Enumerator.", __FUNCTION__);
    return false;
  }

  memset(&m_texDesc, 0, sizeof(D3D11_TEXTURE2D_DESC));
  return true;
}

void CProcessorHD::ApplySupportedFormats(std::vector<ERenderFormat> *formats)
{
  // do not check for NV12 it supported by default
  UINT flags;
  if (SUCCEEDED(m_pEnumerator->CheckVideoProcessorFormat(DXGI_FORMAT_P010, &flags))
    && (flags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT))
  {
    // TODO: temporary disabled
    //formats->push_back(RENDER_FMT_YUV420P10);
  }
  if (SUCCEEDED(m_pEnumerator->CheckVideoProcessorFormat(DXGI_FORMAT_P016, &flags))
    && (flags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT))
  {
    // TODO: temporary disabled
    //formats->push_back(RENDER_FMT_YUV420P16);
  }
}

bool CProcessorHD::InitProcessor()
{
  SAFE_RELEASE(m_pEnumerator);

  CLog::Log(LOGDEBUG, "%s - Initing Video Enumerator with params: %dx%d.", __FUNCTION__, m_width, m_height);

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc;
  ZeroMemory(&contentDesc, sizeof(contentDesc));
  contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  contentDesc.InputWidth = m_width;
  contentDesc.InputHeight = m_height;
  contentDesc.OutputWidth = m_width;
  contentDesc.OutputHeight = m_height;
  contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  if (FAILED(m_pVideoDevice->CreateVideoProcessorEnumerator(&contentDesc, &m_pEnumerator)))
  {
    CLog::Log(LOGWARNING, "%s - failed to reinit Video Enumerator with new params.", __FUNCTION__);
    return false;
  }

  if (FAILED(m_pEnumerator->GetVideoProcessorCaps(&m_vcaps)))
  {
    CLog::Log(LOGWARNING, "%s - failed to get processor caps.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s - Video processor has %d rate conversion.", __FUNCTION__, m_vcaps.RateConversionCapsCount);
  CLog::Log(LOGDEBUG, "%s - Video processor has %#x feature caps.", __FUNCTION__, m_vcaps.FeatureCaps);
  CLog::Log(LOGDEBUG, "%s - Video processor has %#x device caps.", __FUNCTION__, m_vcaps.DeviceCaps);
  CLog::Log(LOGDEBUG, "%s - Video processor has %#x input format caps.", __FUNCTION__, m_vcaps.InputFormatCaps);
  CLog::Log(LOGDEBUG, "%s - Video processor has %d max input streams.", __FUNCTION__, m_vcaps.MaxInputStreams);
  CLog::Log(LOGDEBUG, "%s - Video processor has %d max stream states.", __FUNCTION__, m_vcaps.MaxStreamStates);

  if (0 != (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_STEREO))
    m_bStereoEnabled = true;

  if (0 != (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LEGACY))
    CLog::Log(LOGWARNING, "%s - The video driver does not support full video processing capabilities.", __FUNCTION__);

  m_max_back_refs = 0;
  m_max_fwd_refs = 0;
  m_procIndex = 0;

  unsigned maxProcCaps = 0;
  // try to find best processor
  for (unsigned int i = 0; i < m_vcaps.RateConversionCapsCount; i++)
  {
    D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS convCaps;
    LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(i, &convCaps))

    // check only deintelace caps
    if ((convCaps.ProcessorCaps & 15) > maxProcCaps)
    {
      m_procIndex = i;
      maxProcCaps = convCaps.ProcessorCaps & 15;
    }
  }

  CLog::Log(LOGDEBUG, "%s - Selected video processor index: %d.", __FUNCTION__, m_procIndex);

  LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(m_procIndex, &m_rateCaps))
  m_max_fwd_refs = m_rateCaps.FutureFrames;
  m_max_back_refs = m_rateCaps.PastFrames;

  CLog::Log(LOGNOTICE, "%s - Supported deinterlace methods: Blend:%s, Bob:%s, Adaptive:%s, MoComp:%s.", __FUNCTION__,
    (m_rateCaps.ProcessorCaps & 0x1) != 0 ? "yes" : "no", // BLEND
    (m_rateCaps.ProcessorCaps & 0x2) != 0 ? "yes" : "no", // BOB
    (m_rateCaps.ProcessorCaps & 0x4) != 0 ? "yes" : "no", // ADAPTIVE
    (m_rateCaps.ProcessorCaps & 0x8) != 0 ? "yes" : "no"  // MOTION_COMPENSATION
    );

  CLog::Log(LOGDEBUG, "%s - Selected video processor allows %d future frames and %d past frames.", __FUNCTION__, m_rateCaps.FutureFrames, m_rateCaps.PastFrames);

  m_size = m_max_back_refs + 1 + m_max_fwd_refs + 2;  // refs + 1 display + 2 safety frames

  // Get the image filtering capabilities.
  for (long i = 0; i < NUM_FILTERS; i++)
  {
    if (m_vcaps.FilterCaps & (1 << i))
    {
      ZeroMemory(&m_Filters[i].Range, sizeof(D3D11_VIDEO_PROCESSOR_FILTER_RANGE));
      if (FAILED(m_pEnumerator->GetVideoProcessorFilterRange(PROCAMP_FILTERS[i], &m_Filters[i].Range)))
      {
        m_Filters[i].bSupported = false;
        continue;
      }
      m_Filters[i].bSupported = true;
      CLog::Log(LOGDEBUG, "%s - Filter %d has following params - max: %d, min: %d, default: %d", __FUNCTION__,
        PROCAMP_FILTERS[i], m_Filters[i].Range.Maximum, m_Filters[i].Range.Minimum, m_Filters[i].Range.Default);
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - Filter %d not supported by processor.", __FUNCTION__, PROCAMP_FILTERS[i]);

      m_Filters[i].bSupported = false;
    }
  }

  return true;
}

bool CProcessorHD::Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format)
{
  Close();

  CSingleLock lock(m_section);

  m_width = width;
  m_height = height;
  m_flags = flags;
  m_renderFormat = format;

  if (!InitProcessor())
    return false;

  if (g_advancedSettings.m_DXVANoDeintProcForProgressive)
  {
    CLog::Log(LOGNOTICE, "%s - Auto deinterlacing mode workaround activated. Deinterlacing processor will be used only for interlaced frames.", __FUNCTION__);
  }

  UINT uiFlags;
  // check default output format DXGI_FORMAT_B8G8R8A8_UNORM (as render target)
  if ( S_OK != m_pEnumerator->CheckVideoProcessorFormat(DXGI_FORMAT_B8G8R8A8_UNORM, &uiFlags)
    || 0 == (uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT))
  {
    CLog::Log(LOGERROR, "%s - Unsupported output format.", __FUNCTION__);
    return false;
  }

  if (format == RENDER_FMT_DXVA)
  {
    m_textureFormat = (DXGI_FORMAT)extended_format;

    // this was checked by decoder, but check again.
    if ( S_OK != m_pEnumerator->CheckVideoProcessorFormat(m_textureFormat, &uiFlags)
      || 0 == (uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT))
    {
      CLog::Log(LOGERROR, "%s - Unsupported input format.", __FUNCTION__);
      return false;
    }
  }
  else
  {
    // Only NV12 software colorspace conversion is implemented for now
    m_textureFormat = DXGI_FORMAT_NV12; // default

    if (format == RENDER_FMT_YUV420P)
      m_textureFormat = DXGI_FORMAT_NV12;
    if (format == RENDER_FMT_YUV420P10)
      m_textureFormat = DXGI_FORMAT_P010;
    if (format == RENDER_FMT_YUV420P16)
      m_textureFormat = DXGI_FORMAT_P016;

    if ( S_OK != m_pEnumerator->CheckVideoProcessorFormat(m_textureFormat, &uiFlags)
      || 0 == (uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT))
    {
      CLog::Log(LOGERROR, "%s - Unsupported input format.", __FUNCTION__);
      return false;
    }

    if (!CreateSurfaces())
      return false;
  }

  CLog::Log(LOGDEBUG, "%s - Creating processor with input format: (%d).", __FUNCTION__, m_textureFormat);

  if (!OpenProcessor())
  {
    return false;
  }

  return true;
}

bool CProcessorHD::ReInit()
{
  return PreInit() && Open(m_width, m_height, m_flags, m_renderFormat, m_textureFormat);
}

bool CProcessorHD::OpenProcessor()
{
  // restore the device if it was lost
  if (!m_pEnumerator && !ReInit())
    return false;

  SAFE_RELEASE(m_pVideoProcessor);

  CLog::Log(LOGDEBUG, "%s - Creating video processor.", __FUNCTION__);

  // create processor
  // There is a MSFT bug when creating processor it might throw first-chance exception
  HRESULT hr = m_pVideoDevice->CreateVideoProcessor(m_pEnumerator, m_procIndex, &m_pVideoProcessor);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s - Failed creating video processor with error %x.", __FUNCTION__, hr);
    return false;
  }

  D3D11_VIDEO_PROCESSOR_COLOR_SPACE cs
  {
    0,                                          // 0 - Playback, 1 - Processing
    0,                                          // 0 - Full (0-255), 1 - Limited (16-235)
    m_flags & CONF_FLAGS_YUVCOEF_BT709 ? 1 : 0, // 0 - BT.601, 1 - BT.709
    m_flags & CONF_FLAGS_YUV_FULLRANGE ? 1 : 0, // 0 - Conventional YCbCr, 1 - xvYCC
    0,                                          // 2 - Full luminance range [0-255], 1 - Studio luminance range [16-235], 0 - driver defaults
  };
  if (m_vcaps.DeviceCaps & D3D11_VIDEO_PROCESSOR_DEVICE_CAPS_NOMINAL_RANGE)
    cs.Nominal_Range = m_flags & CONF_FLAGS_YUV_FULLRANGE ? 2 : 1;
  m_pVideoContext->VideoProcessorSetStreamColorSpace(m_pVideoProcessor, DEFAULT_STREAM_INDEX, &cs);

  // Output background color (black)
  D3D11_VIDEO_COLOR color = {};
  color.YCbCr = { 0.0625f, 0.5f, 0.5f, 1.0f }; // black color
  m_pVideoContext->VideoProcessorSetOutputBackgroundColor(m_pVideoProcessor, TRUE, &color);

  // the following code is unneeded, keep it for reference only
  /*if (0 != (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ALPHA_FILL))
  {
    CLog::Log(LOGDEBUG, "%s - Processor supports alfa fill feature.", __FUNCTION__);
    //m_pVideoContext->VideoProcessorSetStreamAlpha(m_pVideoProcessor, DEFAULT_STREAM_INDEX, true, 1.0f);
    //m_pVideoContext->VideoProcessorSetOutputAlphaFillMode(m_pVideoProcessor, D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE_BACKGROUND, DEFAULT_STREAM_INDEX);
  }
  else
    CLog::Log(LOGDEBUG, "%s - Processor doesn't support alfa fill feature.", __FUNCTION__);

  // Output rate (repeat frames)
  if (0 != (m_rateCaps.ProcessorCaps & D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_FRAME_RATE_CONVERSION))
  {
    CLog::Log(LOGDEBUG, "%s - Processor supports frame rate conversion feature.", __FUNCTION__);
    //m_pVideoContext->VideoProcessorSetStreamOutputRate(m_pVideoProcessor, DEFAULT_STREAM_INDEX, D3D11_VIDEO_PROCESSOR_OUTPUT_RATE_NORMAL, TRUE, NULL);
  }
  else
    CLog::Log(LOGDEBUG, "%s - Processor doesn't support frame rate conversion feature.", __FUNCTION__);*/

  return true;
}

bool CProcessorHD::CreateSurfaces()
{
  HRESULT hr;
  size_t idx;
  ID3D11Device* pD3DDevice = g_Windowing.Get3D11Device();

  // we cannot use texture array (like in decoder) for USAGE_DYNAMIC, so create separete textures
  CD3D11_TEXTURE2D_DESC texDesc(m_textureFormat, FFALIGN(m_width, 16), FFALIGN(m_height, 16), 1, 1, D3D11_BIND_DECODER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC pivd = { 0, D3D11_VPIV_DIMENSION_TEXTURE2D };
  pivd.Texture2D.ArraySlice = 0;
  pivd.Texture2D.MipSlice = 0;

  ID3D11VideoProcessorInputView* views[32] = { 0 };
  CLog::Log(LOGDEBUG, "%s - Creating %d processor surfaces with format %d.", __FUNCTION__, m_size, m_textureFormat);

  for (idx = 0; idx < m_size; idx++)
  {
    ID3D11Texture2D* pTexture = nullptr;
    hr = pD3DDevice->CreateTexture2D(&texDesc, NULL, &pTexture);
    if (FAILED(hr))
      break;

    hr = m_pVideoDevice->CreateVideoProcessorInputView(pTexture, m_pEnumerator, &pivd, &views[idx]);
    SAFE_RELEASE(pTexture);
    if (FAILED(hr))
      break;
  }

  if (idx != m_size)
  {
    // something goes wrong
    CLog::Log(LOGERROR, "%s - Failed to create processor surfaces.", __FUNCTION__);
    for (unsigned idx = 0; idx < m_size; idx++)
    {
      SAFE_RELEASE(views[idx]);
    }
    return false;
  }

  m_context = new CSurfaceContext();
  for (unsigned int i = 0; i < m_size; i++)
  {
    m_context->AddSurface(views[i]);
  }

  m_texDesc = texDesc;
  return true;
}

CRenderPicture *CProcessorHD::Convert(DVDVideoPicture* picture)
{
  // RENDER_FMT_YUV420P -> DXGI_FORMAT_NV12
  // RENDER_FMT_YUV420P10 -> DXGI_FORMAT_P010
  // RENDER_FMT_YUV420P16 -> DXGI_FORMAT_P016
  if ( picture->format != RENDER_FMT_YUV420P
    && picture->format != RENDER_FMT_YUV420P10
    && picture->format != RENDER_FMT_YUV420P16)
  {
    CLog::Log(LOGERROR, "%s - colorspace not supported by processor, skipping frame.", __FUNCTION__);
    return nullptr;
  }

  ID3D11View* pView = m_context->GetFree(nullptr);
  if (!pView)
  {
    CLog::Log(LOGERROR, "%s - no free video surface", __FUNCTION__);
    return nullptr;
  }

  ID3D11VideoProcessorInputView* view = reinterpret_cast<ID3D11VideoProcessorInputView*>(pView);

  ID3D11Resource* pResource = nullptr;
  view->GetResource(&pResource);

  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC vpivd;
  view->GetDesc(&vpivd);
  UINT subresource = D3D11CalcSubresource(0, vpivd.Texture2D.ArraySlice, 1);

  D3D11_MAPPED_SUBRESOURCE rectangle;
  ID3D11DeviceContext* pContext = g_Windowing.GetImmediateContext();
  if (FAILED(pContext->Map(pResource, subresource, D3D11_MAP_WRITE_DISCARD, 0, &rectangle)))
  {
    CLog::Log(LOGERROR, "%s - could not lock rect", __FUNCTION__);
    m_context->ClearReference(view);
    return nullptr;
  }

  if (picture->format == RENDER_FMT_YUV420P)
  {
    uint8_t*  pData = static_cast<uint8_t*>(rectangle.pData);
    uint8_t*  dst[] = { pData, pData + m_texDesc.Height * rectangle.RowPitch };
    int dstStride[] = { rectangle.RowPitch, rectangle.RowPitch };
    convert_yuv420_nv12(picture->data, picture->iLineSize, picture->iHeight, picture->iWidth, dst, dstStride);
  }
  else
  {
    // TODO: Optimize this later using sse2/sse4
    uint16_t * d_y = static_cast<uint16_t*>(rectangle.pData);
    uint16_t * d_uv = d_y + m_texDesc.Height * rectangle.RowPitch;
    // Convert to NV12 - Luma
    for (size_t line = 0; line < picture->iHeight; ++line)
    {
      uint16_t * y = (uint16_t*)(picture->data[0] + picture->iLineSize[0] * line);
      uint16_t * d = d_y + rectangle.RowPitch * line;
      memcpy(d, y, picture->iLineSize[0]);
    }
    // Convert to NV12 - Chroma
    size_t chromaWidth = (picture->iWidth + 1) >> 1;
    size_t chromaHeight = picture->iHeight >> 1;
    for (size_t line = 0; line < chromaHeight; ++line)
    {
      uint16_t * u = (uint16_t*)picture->data[1] + line * picture->iLineSize[1];
      uint16_t * v = (uint16_t*)picture->data[2] + line * picture->iLineSize[2];
      uint16_t * d = d_uv + line * rectangle.RowPitch;
      for (size_t x = 0; x < chromaWidth; x++)
      {
        *d++ = *u++; 
        *d++ = *v++;
      }
    }
  }
  pContext->Unmap(pResource, subresource);
  SAFE_RELEASE(pResource);

  m_context->ClearReference(view);
  m_context->MarkRender(view);
  CRenderPicture *pic = new CRenderPicture(m_context);
  pic->view           = view;
  return pic;
}

bool CProcessorHD::ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def)
{
  if (filter >= NUM_FILTERS)
    return false;

  // Unsupported filter. Ignore.
  if (!m_Filters[filter].bSupported)
    return false;

  D3D11_VIDEO_PROCESSOR_FILTER_RANGE range = m_Filters[filter].Range;
  int val;

  if(value > def)
    val = range.Default + (range.Maximum - range.Default) * (value - def) / (max - def);
  else if(value < def)
    val = range.Default + (range.Minimum - range.Default) * (value - def) / (min - def);
  else
    val = range.Default;

  m_pVideoContext->VideoProcessorSetStreamFilter(m_pVideoProcessor, DEFAULT_STREAM_INDEX, filter, val != range.Default, val);
  return true;
}

ID3D11VideoProcessorInputView* CProcessorHD::GetInputView(ID3D11View* view) 
{
  ID3D11VideoProcessorInputView* inputView = nullptr;
  if (m_context) // we have own context so the view will be processor input view
  {
    inputView = reinterpret_cast<ID3D11VideoProcessorInputView*>(view);
    inputView->AddRef(); // it will be released in Render method

    return inputView;
  }

  // the view came from decoder
  ID3D11VideoDecoderOutputView* decoderView = reinterpret_cast<ID3D11VideoDecoderOutputView*>(view);
  if (!decoderView) 
  {
    CLog::Log(LOGERROR, __FUNCTION__" - cannot get view.");
    return nullptr;
  }

  ID3D11Resource* resource = nullptr;
  D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vdovd;
  decoderView->GetDesc(&vdovd);
  decoderView->GetResource(&resource);

  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC vpivd = { 0 };
  vpivd.FourCC = 0; // if zero, the driver uses the DXGI format; must be 0 on level 9.x
  vpivd.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
  vpivd.Texture2D.ArraySlice = vdovd.Texture2D.ArraySlice;
  vpivd.Texture2D.MipSlice = 0;

  if (FAILED(m_pVideoDevice->CreateVideoProcessorInputView(resource, m_pEnumerator, &vpivd, &inputView)))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - cannot create processor view.");
  }
  resource->Release();

  return inputView;
}

bool CProcessorHD::Render(CRect src, CRect dst, ID3D11Resource* target, ID3D11View** views, DWORD flags, UINT frameIdx, UINT rotation)
{
  HRESULT hr;
  CSingleLock lock(m_section);

  // restore processor if it was lost
  if (!m_pVideoProcessor && !OpenProcessor())
    return false;
  
  if (!views[2])
    return false;

  EDEINTERLACEMODE deinterlace_mode = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_DeinterlaceMode;
  if (g_advancedSettings.m_DXVANoDeintProcForProgressive)
    deinterlace_mode = (flags & RENDER_FLAG_FIELD0 || flags & RENDER_FLAG_FIELD1) ? VS_DEINTERLACEMODE_FORCE : VS_DEINTERLACEMODE_OFF;
  EINTERLACEMETHOD interlace_method = g_renderManager.AutoInterlaceMethod(CMediaSettings::GetInstance().GetCurrentVideoSettings().m_InterlaceMethod);

  bool progressive = deinterlace_mode == VS_DEINTERLACEMODE_OFF;

  RECT sourceRECT = { src.x1, src.y1, src.x2, src.y2 };
  RECT dstRECT    = { dst.x1, dst.y1, dst.x2, dst.y2 };

  D3D11_VIDEO_FRAME_FORMAT dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;

  unsigned int providedPast = 0;
  for (int i = 3; i < 8; i++)
  {
    if (views[i])
      providedPast++;
  }
  unsigned int providedFuture = 0;
  for (int i = 1; i >= 0; i--)
  {
    if (views[i])
      providedFuture++;
  }
  int futureFrames = std::min(providedFuture, m_rateCaps.FutureFrames);
  int pastFrames = std::min(providedPast, m_rateCaps.PastFrames);

  D3D11_VIDEO_PROCESSOR_STREAM stream_data = { 0 };
  stream_data.Enable = TRUE;
  stream_data.PastFrames = pastFrames;
  stream_data.FutureFrames = futureFrames;
  stream_data.ppPastSurfaces = new ID3D11VideoProcessorInputView*[pastFrames];
  stream_data.ppFutureSurfaces = new ID3D11VideoProcessorInputView*[futureFrames];
  stream_data.pInputSurfaceRight = nullptr;
  stream_data.ppPastSurfacesRight = nullptr;
  stream_data.ppFutureSurfacesRight = nullptr;

  int start = 2 - futureFrames;
  int end = 2 + pastFrames;

  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC pivd;
  ZeroMemory(&pivd, sizeof(D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC));
  pivd.FourCC = 0;
  pivd.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
  pivd.Texture2D.ArraySlice = 0;
  pivd.Texture2D.MipSlice = 0;

  for (int i = start; i <= end; i++)
  {
    if (!views[i])
      continue;

    if (i > 2)
    {
      // frames order should be { ?, T-3, T-2, T-1 }
      stream_data.ppPastSurfaces[2 + pastFrames - i] = GetInputView(views[i]);
    }
    else if (i == 2)
    {
      stream_data.pInputSurface = GetInputView(views[2]);
    }
    else if (i < 2)
    {
      // frames order should be { T+1, T+2, T+3, .. }
      stream_data.ppFutureSurfaces[1 - i] = GetInputView(views[i]);
    }
  }

  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_TOP)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  else if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_BOT)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_BOT)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;
  if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_TOP)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;

  // Override the sample format when the processor doesn't need to deinterlace or when deinterlacing is forced and flags are missing.
  if (progressive)
  {
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
  }
  else if (deinterlace_mode == VS_DEINTERLACEMODE_FORCE 
    && dxvaFrameFormat == D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE)
  {
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  }

  bool frameProgressive = dxvaFrameFormat == D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;

  // Progressive or Interlaced video at normal rate.
  stream_data.InputFrameOrField = frameIdx;
  stream_data.OutputIndex = flags & RENDER_FLAG_FIELD1 && !frameProgressive ? 1 : 0;

  // input format
  m_pVideoContext->VideoProcessorSetStreamFrameFormat(m_pVideoProcessor, DEFAULT_STREAM_INDEX, dxvaFrameFormat);
  // Source rect
  m_pVideoContext->VideoProcessorSetStreamSourceRect(m_pVideoProcessor, DEFAULT_STREAM_INDEX, TRUE, &sourceRECT);
  // Stream dest rect
  m_pVideoContext->VideoProcessorSetStreamDestRect(m_pVideoProcessor, DEFAULT_STREAM_INDEX, TRUE, &dstRECT);
  // Output rect
  m_pVideoContext->VideoProcessorSetOutputTargetRect(m_pVideoProcessor, TRUE, &dstRECT);
  // Output color space
  // don't apply any color range conversion, this will be fixed at later stage.
  D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace = {};
  colorSpace.Usage         = 0;  // 0 - playback, 1 - video processing
  colorSpace.RGB_Range     = 0;  // 0 - 0-255, 1 - 16-235
  colorSpace.YCbCr_Matrix  = 1;  // 0 - BT.601, 1 = BT.709
  colorSpace.YCbCr_xvYCC   = 1;  // 0 - Conventional YCbCr, 1 - xvYCC
  colorSpace.Nominal_Range = 2;  // 2 - 0-255, 1 = 16-235, 0 - undefined

  m_pVideoContext->VideoProcessorSetOutputColorSpace(m_pVideoProcessor, &colorSpace);

  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS, 
              CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness, 0, 100, 50);
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST, 
              CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast, 0, 100, 50);
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_HUE, 50, 0, 100, 50);
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_SATURATION, 50, 0, 100, 50);
  // Rotation
  m_pVideoContext->VideoProcessorSetStreamRotation(m_pVideoProcessor, DEFAULT_STREAM_INDEX, (rotation != 0), (D3D11_VIDEO_PROCESSOR_ROTATION)(rotation / 90));

  //
  // Create Output View of Output Surfaces.
  //
  D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC OutputViewDesc;
  ZeroMemory(&OutputViewDesc, sizeof(OutputViewDesc));
  OutputViewDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
  OutputViewDesc.Texture2D.MipSlice = 0;
  OutputViewDesc.Texture2DArray.ArraySize = 0; // 2 for stereo
  OutputViewDesc.Texture2DArray.MipSlice = 0;
  OutputViewDesc.Texture2DArray.FirstArraySlice = 0;

  ID3D11VideoProcessorOutputView* pOutputView;
  hr = m_pVideoDevice->CreateVideoProcessorOutputView(target, m_pEnumerator, &OutputViewDesc, &pOutputView);
  if (S_OK != hr)
    CLog::Log(FAILED(hr) ? LOGERROR : LOGWARNING, __FUNCTION__" - Device returns result '%x' while creating processor output.", hr);

  if (SUCCEEDED(hr))
  {
    hr = m_pVideoContext->VideoProcessorBlt(m_pVideoProcessor, pOutputView, frameIdx, 1, &stream_data);
    if (S_OK != hr)
    {
      CLog::Log(FAILED(hr) ? LOGERROR : LOGWARNING, __FUNCTION__" - Device returns result '%x' while VideoProcessorBlt execution.", hr);
    }
  }

  SAFE_RELEASE(pOutputView);
  SAFE_RELEASE(stream_data.pInputSurface);

  for (unsigned i = 0; i < stream_data.PastFrames; ++i)
    SAFE_RELEASE(stream_data.ppPastSurfaces[i]);

  for (unsigned i = 0; i < stream_data.FutureFrames; ++i)
    SAFE_RELEASE(stream_data.ppFutureSurfaces[i]);

  delete[] stream_data.ppPastSurfaces;
  delete [] stream_data.ppFutureSurfaces;

  return !FAILED(hr);
}

#endif
