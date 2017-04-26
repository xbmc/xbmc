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
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFlags.h"
#include "cores/VideoPlayer/VideoRenderers/WinVideoBuffer.h"
#include "settings/MediaSettings.h"
#include "utils/Log.h"
#include "utils/win32/memcpy_sse2.h"
#include "platform/win32/WIN32Util.h"
#include "windowing/WindowingFactory.h"

using namespace DXVA;

#define LOGIFERROR(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, "%s: failed executing "#a" at line %d with error %x", __FUNCTION__, __LINE__, res); \
  } \
} while(0);

CProcessorHD::CProcessorHD()
  : m_width(0)
  , m_height(0)
  , m_pVideoDevice(nullptr)
  , m_pVideoContext(nullptr)
  , m_pEnumerator(nullptr)
  , m_pVideoProcessor(nullptr)
{
  g_Windowing.Register(this);
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
}

void CProcessorHD::Close()
{
  CSingleLock lock(m_section);
  SAFE_RELEASE(m_pEnumerator);
  SAFE_RELEASE(m_pVideoProcessor);
  SAFE_RELEASE(m_pVideoContext);
}

bool CProcessorHD::PreInit()
{
  SAFE_RELEASE(m_pVideoDevice);

  if (FAILED(g_Windowing.Get3D11Device()->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&m_pVideoDevice))))
  {
    CLog::Log(LOGWARNING, "%s: failed to get video device.", __FUNCTION__);
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
    CLog::Log(LOGWARNING, "%s: failed to create Video Enumerator.", __FUNCTION__);
    return false;
  }

  SAFE_RELEASE(m_pEnumerator);
  return true;
}

bool CProcessorHD::InitProcessor()
{
  SAFE_RELEASE(m_pEnumerator);
  SAFE_RELEASE(m_pVideoContext);

  if (FAILED(g_Windowing.GetImmediateContext()->QueryInterface(__uuidof(ID3D11VideoContext), reinterpret_cast<void**>(&m_pVideoContext))))
  {
    CLog::Log(LOGWARNING, "%s: Context initialization is failed.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s: Initing Video Enumerator with params: %dx%d.", __FUNCTION__, m_width, m_height);

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
    CLog::Log(LOGWARNING, "%s: failed to init video enumerator with params: %dx%d.", __FUNCTION__, m_width, m_height);
    return false;
  }

  if (FAILED(m_pEnumerator->GetVideoProcessorCaps(&m_vcaps)))
  {
    CLog::Log(LOGWARNING, "%s - failed to get processor caps.", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "%s: Video processor has %d rate conversion.", __FUNCTION__, m_vcaps.RateConversionCapsCount);
  CLog::Log(LOGDEBUG, "%s: Video processor has %#x feature caps.", __FUNCTION__, m_vcaps.FeatureCaps);
  CLog::Log(LOGDEBUG, "%s: Video processor has %#x device caps.", __FUNCTION__, m_vcaps.DeviceCaps);
  CLog::Log(LOGDEBUG, "%s: Video processor has %#x input format caps.", __FUNCTION__, m_vcaps.InputFormatCaps);
  CLog::Log(LOGDEBUG, "%s: Video processor has %d max input streams.", __FUNCTION__, m_vcaps.MaxInputStreams);
  CLog::Log(LOGDEBUG, "%s: Video processor has %d max stream states.", __FUNCTION__, m_vcaps.MaxStreamStates);

  if (0 != (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LEGACY))
    CLog::Log(LOGWARNING, "%s: The video driver does not support full video processing capabilities.", __FUNCTION__);

  m_max_back_refs = 0;
  m_max_fwd_refs = 0;
  m_procIndex = 0;

  unsigned maxProcCaps = 0;
  // try to find best processor
  for (unsigned int i = 0; i < m_vcaps.RateConversionCapsCount; i++)
  {
    D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS convCaps;
    LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(i, &convCaps))

    // check only deinterlace caps
    if ((convCaps.ProcessorCaps & 15) > maxProcCaps)
    {
      m_procIndex = i;
      maxProcCaps = convCaps.ProcessorCaps & 15;
    }
  }

  CLog::Log(LOGDEBUG, "%s: Selected video processor index: %d.", __FUNCTION__, m_procIndex);

  LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(m_procIndex, &m_rateCaps))
  m_max_fwd_refs  = std::min(m_rateCaps.FutureFrames, 2u);
  m_max_back_refs = std::min(m_rateCaps.PastFrames,  4u);

  CLog::Log(LOGNOTICE, "%s: Supported deinterlace methods: Blend:%s, Bob:%s, Adaptive:%s, MoComp:%s.", __FUNCTION__,
    (m_rateCaps.ProcessorCaps & 0x1) != 0 ? "yes" : "no", // BLEND
    (m_rateCaps.ProcessorCaps & 0x2) != 0 ? "yes" : "no", // BOB
    (m_rateCaps.ProcessorCaps & 0x4) != 0 ? "yes" : "no", // ADAPTIVE
    (m_rateCaps.ProcessorCaps & 0x8) != 0 ? "yes" : "no"  // MOTION_COMPENSATION
    );

  CLog::Log(LOGDEBUG, "%s: Selected video processor allows %d future frames and %d past frames.", __FUNCTION__, m_rateCaps.FutureFrames, m_rateCaps.PastFrames);

  m_size = m_max_back_refs + 1 + m_max_fwd_refs;  // refs + 1 display

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
      CLog::Log(LOGDEBUG, "%s: Filter %d has following params - max: %d, min: %d, default: %d", __FUNCTION__,
        PROCAMP_FILTERS[i], m_Filters[i].Range.Maximum, m_Filters[i].Range.Minimum, m_Filters[i].Range.Default);
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s: Filter %d not supported by processor.", __FUNCTION__, PROCAMP_FILTERS[i]);

      m_Filters[i].bSupported = false;
    }
  }

  return true;
}

bool CProcessorHD::IsFormatSupported(DXGI_FORMAT format, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT support) const
{
  UINT uiFlags;
  if (S_OK == m_pEnumerator->CheckVideoProcessorFormat(format, &uiFlags))
  {
    if (uiFlags & support)
      return true;
  }

  CLog::Log(LOGERROR, "%s: Unsupported format %d for %d.", __FUNCTION__, format, support);
  return false;
}

bool CProcessorHD::CheckFormats() const
{
  // check default output format DXGI_FORMAT_B8G8R8A8_UNORM (as render target)
  if (!IsFormatSupported(DXGI_FORMAT_B8G8R8A8_UNORM, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT))
    return false;
  return true;
}

bool CProcessorHD::Open(UINT width, UINT height)
{
  Close();

  CSingleLock lock(m_section);

  m_width = width;
  m_height = height;

  if (!InitProcessor())
    return false;

  if (!CheckFormats())
    return false;

  return OpenProcessor();
}

bool CProcessorHD::ReInit()
{
  CSingleLock lock(m_section);
  UnInit();

  if (!PreInit())
    return false;

  if (!InitProcessor())
    return false;

  if (!CheckFormats())
    return false;

  return true;
}

bool CProcessorHD::OpenProcessor()
{
  CSingleLock lock(m_section);

  // restore the device if it was lost
  if (!m_pEnumerator && !ReInit())
    return false;

  SAFE_RELEASE(m_pVideoProcessor);
  CLog::Log(LOGDEBUG, "%s: Creating processor.", __FUNCTION__);

  // create processor
  HRESULT hr = m_pVideoDevice->CreateVideoProcessor(m_pEnumerator, m_procIndex, &m_pVideoProcessor);
  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s: Failed creating video processor with error %x.", __FUNCTION__, hr);
    return false;
  }

  // Output background color (black)
  D3D11_VIDEO_COLOR color;
  color.YCbCr = { 0.0625f, 0.5f, 0.5f, 1.0f }; // black color
  m_pVideoContext->VideoProcessorSetOutputBackgroundColor(m_pVideoProcessor, TRUE, &color);

  return true;
}

bool CProcessorHD::ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const
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

ID3D11VideoProcessorInputView* CProcessorHD::GetInputView(SWinVideoBuffer* view) const
{
  ID3D11VideoProcessorInputView* inputView = nullptr;
  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC vpivd = { 0, D3D11_VPIV_DIMENSION_TEXTURE2D,{ 0, 0 } };

  if (view->format == BUFFER_FMT_DXVA_BYPASS)
  {
    // the view cames from decoder
    ID3D11VideoDecoderOutputView* decoderView = reinterpret_cast<ID3D11VideoDecoderOutputView*>(view->GetHWView());
    if (!decoderView)
    {
      CLog::Log(LOGERROR, "%s: cannot get decoder view.", __FUNCTION__);
      return nullptr;
    }

    ID3D11Resource* resource = nullptr;
    D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC vdovd;
    decoderView->GetDesc(&vdovd);
    decoderView->GetResource(&resource);
    vpivd.Texture2D.ArraySlice = vdovd.Texture2D.ArraySlice;

    if (FAILED(m_pVideoDevice->CreateVideoProcessorInputView(resource, m_pEnumerator, &vpivd, &inputView)))
      CLog::Log(LOGERROR, "%s: cannot create processor input view.", __FUNCTION__);

    resource->Release();
  }
  else if (view->format == BUFFER_FMT_DXVA_NV12
        || view->format == BUFFER_FMT_DXVA_P010
        || view->format == BUFFER_FMT_DXVA_P016)
  {
    if (FAILED(m_pVideoDevice->CreateVideoProcessorInputView(view->GetResource(), m_pEnumerator, &vpivd, &inputView)))
      CLog::Log(LOGERROR, "%s: cannot create processor input view.", __FUNCTION__);
  }

  return inputView;
}

static void ReleaseStream(D3D11_VIDEO_PROCESSOR_STREAM &stream_data)
{
  SAFE_RELEASE(stream_data.pInputSurface);

  for (size_t i = 0; i < stream_data.PastFrames; ++i)
    SAFE_RELEASE(stream_data.ppPastSurfaces[i]);

  for (size_t i = 0; i < stream_data.FutureFrames; ++i)
    SAFE_RELEASE(stream_data.ppFutureSurfaces[i]);

  delete[] stream_data.ppPastSurfaces;
  delete[] stream_data.ppFutureSurfaces;
}

bool CProcessorHD::Render(CRect src, CRect dst, ID3D11Resource* target, SWinVideoBuffer** views, DWORD flags, UINT frameIdx, UINT rotation)
{
  HRESULT hr;
  CSingleLock lock(m_section);

  // restore processor if it was lost
  if (!m_pVideoProcessor && !OpenProcessor())
    return false;
  
  if (!views[2])
    return false;

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
  if (pastFrames)
    stream_data.ppPastSurfaces = new ID3D11VideoProcessorInputView*[pastFrames];
  if (futureFrames)
    stream_data.ppFutureSurfaces = new ID3D11VideoProcessorInputView*[futureFrames];

  ID3D11VideoProcessorInputView* view = nullptr;
  int start = 2 - futureFrames;
  int end = 2 + pastFrames;
  int count = 0;

  for (int i = start; i <= end; i++)
  {
    if (!views[i])
      continue;

    view = GetInputView(views[i]);
    if (i > 2)
    {
      // frames order should be { ?, T-3, T-2, T-1 }
      stream_data.ppPastSurfaces[2 + pastFrames - i] = view;
    }
    else if (i == 2)
    {
      stream_data.pInputSurface = view;
    }
    else if (i < 2)
    {
      // frames order should be { T+1, T+2, T+3, .. }
      stream_data.ppFutureSurfaces[1 - i] = view;
    }
    if (view)
      count++;
  }

  if (count != pastFrames + futureFrames + 1)
  {
    CLog::Log(LOGERROR, "%s: incomplete views set.", __FUNCTION__);
    ReleaseStream(stream_data);
    return false;
  }

  // fix locked textures
  for (int i = start; i <= end; i++)
    if (i != 2 && views[i]) views[i]->StartRender();

  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_TOP)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  else if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_BOT)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  if (flags & RENDER_FLAG_FIELD0 && flags & RENDER_FLAG_BOT)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;
  if (flags & RENDER_FLAG_FIELD1 && flags & RENDER_FLAG_TOP)
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;

  bool frameProgressive = dxvaFrameFormat == D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;

  // Progressive or Interlaced video at normal rate.
  stream_data.InputFrameOrField = frameIdx;
  stream_data.OutputIndex = flags & RENDER_FLAG_FIELD1 && !frameProgressive ? 1 : 0;

  // input format
  m_pVideoContext->VideoProcessorSetStreamFrameFormat(m_pVideoProcessor, DEFAULT_STREAM_INDEX, dxvaFrameFormat);
  // input colorspace
  D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace
  {
    0,                                                  // 0 - Playback, 1 - Processing
    0,                                                  // 0 - Full (0-255), 1 - Limited (16-235) (RGB)
    views[2]->flags & CONF_FLAGS_YUVCOEF_BT709 ? 1 : 0, // 0 - BT.601, 1 - BT.709
    0,                                                  // 0 - Conventional YCbCr, 1 - xvYCC
    views[2]->flags & CONF_FLAGS_YUV_FULLRANGE ? 2 : 1, // 0 - driver defaults, 2 - Full range [0-255], 1 - Studio range [16-235] (YUV)
  };
  m_pVideoContext->VideoProcessorSetStreamColorSpace(m_pVideoProcessor, DEFAULT_STREAM_INDEX, &colorSpace);
  // Source rect
  m_pVideoContext->VideoProcessorSetStreamSourceRect(m_pVideoProcessor, DEFAULT_STREAM_INDEX, TRUE, &sourceRECT);
  // Stream dest rect
  m_pVideoContext->VideoProcessorSetStreamDestRect(m_pVideoProcessor, DEFAULT_STREAM_INDEX, TRUE, &dstRECT);
  // Output rect
  m_pVideoContext->VideoProcessorSetOutputTargetRect(m_pVideoProcessor, TRUE, &dstRECT);
  // Output color space
  // don't apply any color range conversion, this will be fixed at later stage.
  colorSpace.Usage         = 0;  // 0 - playback, 1 - video processing
  colorSpace.RGB_Range     = g_Windowing.UseLimitedColor() ? 1 : 0;  // 0 - 0-255, 1 - 16-235
  colorSpace.YCbCr_Matrix  = 1;  // 0 - BT.601, 1 = BT.709
  colorSpace.YCbCr_xvYCC   = 1;  // 0 - Conventional YCbCr, 1 - xvYCC
  colorSpace.Nominal_Range = 0;  // 2 - 0-255, 1 = 16-235, 0 - undefined
  m_pVideoContext->VideoProcessorSetOutputColorSpace(m_pVideoProcessor, &colorSpace);
  // brightness
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS, 
              CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Brightness, 0, 100, 50);
  // contrast
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST, 
              CMediaSettings::GetInstance().GetCurrentVideoSettings().m_Contrast, 0, 100, 50);
  // unused filters
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_HUE, 50, 0, 100, 50);
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_SATURATION, 50, 0, 100, 50);
  // Rotation
  m_pVideoContext->VideoProcessorSetStreamRotation(m_pVideoProcessor, DEFAULT_STREAM_INDEX, rotation != 0
                                                 , static_cast<D3D11_VIDEO_PROCESSOR_ROTATION>(rotation / 90));
  // create output view for surface.
  D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC OutputViewDesc = { D3D11_VPOV_DIMENSION_TEXTURE2D, { 0 }};
  ID3D11VideoProcessorOutputView* pOutputView = nullptr;
  hr = m_pVideoDevice->CreateVideoProcessorOutputView(target, m_pEnumerator, &OutputViewDesc, &pOutputView);
  if (S_OK != hr)
    CLog::Log(FAILED(hr) ? LOGERROR : LOGWARNING, "%s: Device returns result '%x' while creating processor output view.", __FUNCTION__, hr);

  if (SUCCEEDED(hr))
  {
    hr = m_pVideoContext->VideoProcessorBlt(m_pVideoProcessor, pOutputView, frameIdx, 1, &stream_data);
    if (S_OK != hr)
    {
      CLog::Log(FAILED(hr) ? LOGERROR : LOGWARNING, "%s: Device returns result '%x' while VideoProcessorBlt execution.", __FUNCTION__, hr);
    }
  }

  SAFE_RELEASE(pOutputView);
  ReleaseStream(stream_data);

  return !FAILED(hr);
}

#endif
