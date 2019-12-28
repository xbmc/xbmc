/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// setting that here because otherwise SampleFormat is defined to AVSampleFormat
// which we don't use here
#define FF_API_OLD_SAMPLE_FMT 0
#define DEFAULT_STREAM_INDEX (0)

#include "DXVAHD.h"
#include "platform/win32/WIN32Util.h"
#include "rendering/dx/RenderContext.h"
#include "rendering/dx/DeviceResources.h"
#include "VideoRenderers/RenderManager.h"
#include "VideoRenderers/RenderFlags.h"
#include "VideoRenderers/windows/RendererBase.h"
#include "utils/log.h"

#include <Windows.h>
#include <d3d11_4.h>

using namespace DXVA;
using namespace Microsoft::WRL;

#define LOGIFERROR(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::LogF(LOGERROR, "failed executing "#a" at line %d with error %x", __LINE__, res); \
  } \
} while(0);

CProcessorHD::CProcessorHD()
{
  DX::Windowing()->Register(this);
}

CProcessorHD::~CProcessorHD()
{
  DX::Windowing()->Unregister(this);
  UnInit();
}

void CProcessorHD::UnInit()
{
  CSingleLock lock(m_section);
  Close();
}

void CProcessorHD::Close()
{
  CSingleLock lock(m_section);
  m_pEnumerator = nullptr;
  m_pVideoProcessor = nullptr;
  m_pVideoContext = nullptr;
  m_pVideoDevice = nullptr;
  m_bSupportHDR10 = false;
}

bool CProcessorHD::PreInit() const
{
  ComPtr<ID3D11VideoDevice> pVideoDevice;
  ComPtr<ID3D11VideoProcessorEnumerator> pEnumerator;
  ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(pD3DDevice.As(&pVideoDevice)))
  {
    CLog::LogF(LOGWARNING, "failed to get video device.");
    return false;
  }

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc1 = {};
  desc1.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  desc1.InputWidth = 640;
  desc1.InputHeight = 480;
  desc1.OutputWidth = 640;
  desc1.OutputHeight = 480;
  desc1.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  // try to create video enum
  if (FAILED(pVideoDevice->CreateVideoProcessorEnumerator(&desc1, &pEnumerator)))
  {
    CLog::LogF(LOGWARNING, "failed to create Video Enumerator.");
    return false;
  }
  return true;
}

bool CProcessorHD::InitProcessor()
{
  m_pVideoDevice = nullptr;
  m_pVideoContext = nullptr;
  m_pEnumerator = nullptr;

  ComPtr<ID3D11DeviceContext1> pD3DDeviceContext = DX::DeviceResources::Get()->GetImmediateContext();
  ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(pD3DDeviceContext.As(&m_pVideoContext)))
  {
    CLog::LogF(LOGWARNING, "video context initialization is failed.");
    return false;
  }
  if (FAILED(pD3DDevice.As(&m_pVideoDevice)))
  {
    CLog::LogF(LOGWARNING, "video device initialization is failed.");
    return false;
  }

  CLog::LogF(LOGDEBUG, "initing video enumerator with params: %dx%d.", m_width, m_height);

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc = {};
  contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  contentDesc.InputWidth = m_width;
  contentDesc.InputHeight = m_height;
  contentDesc.OutputWidth = m_width;
  contentDesc.OutputHeight = m_height;
  contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  if (FAILED(m_pVideoDevice->CreateVideoProcessorEnumerator(&contentDesc, m_pEnumerator.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGWARNING, "failed to init video enumerator with params: %dx%d.", m_width, m_height);
    return false;
  }

  if (FAILED(m_pEnumerator->GetVideoProcessorCaps(&m_vcaps)))
  {
    CLog::LogF(LOGWARNING, "failed to get processor caps.");
    return false;
  }

  CLog::LogF(LOGDEBUG, "video processor has %d rate conversion.", m_vcaps.RateConversionCapsCount);
  CLog::LogF(LOGDEBUG, "video processor has %#x feature caps.", m_vcaps.FeatureCaps);
  CLog::LogF(LOGDEBUG, "video processor has %#x device caps.", m_vcaps.DeviceCaps);
  CLog::LogF(LOGDEBUG, "video processor has %#x input format caps.", m_vcaps.InputFormatCaps);
  CLog::LogF(LOGDEBUG, "video processor has %d max input streams.", m_vcaps.MaxInputStreams);
  CLog::LogF(LOGDEBUG, "video processor has %d max stream states.", m_vcaps.MaxStreamStates);
  if ((m_bSupportHDR10 = m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_METADATA_HDR10))
    CLog::LogF(LOGDEBUG, "video processor supports HDR10.");

  if (0 != (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LEGACY))
    CLog::LogF(LOGWARNING, "the video driver does not support full video processing capabilities.");

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

  CLog::LogF(LOGDEBUG, "selected video processor index: %d.", m_procIndex);

  LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(m_procIndex, &m_rateCaps))
  m_max_fwd_refs  = std::min(m_rateCaps.FutureFrames, 2u);
  m_max_back_refs = std::min(m_rateCaps.PastFrames,  4u);

  CLog::LogF(LOGNOTICE, "supported deinterlace methods: blend:%s, bob:%s, adaptive:%s, mocomp:%s.",
    (m_rateCaps.ProcessorCaps & 0x1) != 0 ? "yes" : "no", // BLEND
    (m_rateCaps.ProcessorCaps & 0x2) != 0 ? "yes" : "no", // BOB
    (m_rateCaps.ProcessorCaps & 0x4) != 0 ? "yes" : "no", // ADAPTIVE
    (m_rateCaps.ProcessorCaps & 0x8) != 0 ? "yes" : "no"  // MOTION_COMPENSATION
    );

  CLog::LogF(LOGDEBUG, "selected video processor allows %d future frames and %d past frames.", m_rateCaps.FutureFrames, m_rateCaps.PastFrames);

  //m_size = m_max_back_refs + 1 + m_max_fwd_refs;  // refs + 1 display

  // Get the image filtering capabilities.
  for (size_t i = 0; i < NUM_FILTERS; i++)
  {
    if (m_vcaps.FilterCaps & (1 << i))
    {
      m_Filters[i].Range = {};
      m_Filters[i].bSupported = SUCCEEDED(m_pEnumerator->GetVideoProcessorFilterRange(PROCAMP_FILTERS[i], &m_Filters[i].Range));

      if (m_Filters[i].bSupported)
      {
        CLog::LogF(LOGDEBUG, "filter %d has following params - max: %d, min: %d, default: %d",
          PROCAMP_FILTERS[i], m_Filters[i].Range.Maximum, m_Filters[i].Range.Minimum, m_Filters[i].Range.Default);
      }
    }
    else
    {
      CLog::LogF(LOGDEBUG, "filter %d not supported by processor.", PROCAMP_FILTERS[i]);
      m_Filters[i].bSupported = false;
    }
  }

  if (m_bSupportHDR10)
    m_hdr10Display = DX::DeviceResources::Get()->GetHdr10Display();

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

  CLog::LogF(LOGERROR, "unsupported format %d for %d.", format, support);
  return false;
}

bool CProcessorHD::CheckFormats() const
{
  // check default output format (as render target)
  return IsFormatSupported(DX::Windowing()->GetBackBuffer().GetFormat(), D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT);
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
  Close();

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

  CLog::LogF(LOGDEBUG, "creating processor.");

  // create processor
  HRESULT hr = m_pVideoDevice->CreateVideoProcessor(m_pEnumerator.Get(), m_procIndex, m_pVideoProcessor.ReleaseAndGetAddressOf());
  if (FAILED(hr))
  {
    CLog::LogF(LOGDEBUG, "failed creating video processor with error %x.", hr);
    return false;
  }

  // Output background color (black)
  D3D11_VIDEO_COLOR color;
  color.YCbCr = { 0.0625f, 0.5f, 0.5f, 1.0f }; // black color
  m_pVideoContext->VideoProcessorSetOutputBackgroundColor(m_pVideoProcessor.Get(), TRUE, &color);

  return true;
}

void CProcessorHD::ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const
{
  if (filter >= static_cast<D3D11_VIDEO_PROCESSOR_FILTER>(NUM_FILTERS))
    return;

  // Unsupported filter. Ignore.
  if (!m_Filters[filter].bSupported)
    return;

  D3D11_VIDEO_PROCESSOR_FILTER_RANGE range = m_Filters[filter].Range;
  int val;

  if(value > def)
    val = range.Default + (range.Maximum - range.Default) * (value - def) / (max - def);
  else if(value < def)
    val = range.Default + (range.Minimum - range.Default) * (value - def) / (min - def);
  else
    val = range.Default;

  m_pVideoContext->VideoProcessorSetStreamFilter(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, filter, val != range.Default, val);
}

ID3D11VideoProcessorInputView* CProcessorHD::GetInputView(CRenderBuffer* view) const
{
  ComPtr<ID3D11VideoProcessorInputView> inputView;
  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC vpivd = {0, D3D11_VPIV_DIMENSION_TEXTURE2D, {0, 0}};

  ComPtr<ID3D11Resource> resource;
  unsigned arrayIdx = 0;
  HRESULT hr = view->GetResource(resource.GetAddressOf(), &arrayIdx);
  if (SUCCEEDED(hr))
  {
    vpivd.Texture2D.ArraySlice = arrayIdx;
    hr = m_pVideoDevice->CreateVideoProcessorInputView(resource.Get(), m_pEnumerator.Get(), &vpivd, inputView.GetAddressOf());
  }

  if (FAILED(hr) || hr == S_FALSE)
    CLog::LogF(LOGERROR, "cannot create processor input view.");

  return inputView.Detach();
}

DXGI_COLOR_SPACE_TYPE CProcessorHD::GetDXGIColorSpace(CRenderBuffer* view, bool supportHDR)
{
  // RGB
  if (view->color_space == AVCOL_SPC_RGB)
  {
    if (!view->full_range)
    {
      if (view->primaries == AVCOL_PRI_BT2020)
      {
        if (view->color_transfer == AVCOL_TRC_SMPTEST2084 && supportHDR)
          return DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020;

        return DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020;
      }
      return DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709;
    }

    if (view->primaries == AVCOL_PRI_BT2020)
    {
      if (view->color_transfer == AVCOL_TRC_SMPTEST2084)
        return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

      return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020;
    }
    if (view->color_transfer == AVCOL_TRC_LINEAR ||
        view->color_transfer == AVCOL_TRC_LOG)
      return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

    return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
  }
  // UHDTV
  if (view->primaries == AVCOL_PRI_BT2020)
  {
    if (view->color_transfer == AVCOL_TRC_SMPTEST2084 && supportHDR) // HDR
      // Could also be:
      // DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020
      return DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020;

    if (view->color_transfer == AVCOL_TRC_ARIB_STD_B67) // HLG
      return DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020;

    if (view->full_range)
      return DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020;

    // Could also be:
    // DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020
    return DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020;
  }
  // SDTV
  if (view->primaries == AVCOL_PRI_BT470BG ||
      view->primaries == AVCOL_PRI_SMPTE170M)
  {
    if (view->full_range)
      return DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601;

    return DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601;
  }
  // HDTV
  if (view->full_range)
  {
    if (view->color_transfer == AVCOL_TRC_SMPTE170M)
      return DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601;

    return DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709;
  }

  return DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
}

bool CProcessorHD::Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer** views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness)
{
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
  const int futureFrames = std::min(providedFuture, m_rateCaps.FutureFrames);
  const int pastFrames = std::min(providedPast, m_rateCaps.PastFrames);
  std::vector<ID3D11VideoProcessorInputView*> pastViews(pastFrames, nullptr);
  std::vector<ID3D11VideoProcessorInputView*> futureViews(futureFrames, nullptr);

  D3D11_VIDEO_PROCESSOR_STREAM stream_data = {};
  stream_data.Enable = TRUE;
  stream_data.PastFrames = pastFrames;
  stream_data.FutureFrames = futureFrames;
  stream_data.ppPastSurfaces = pastViews.data();
  stream_data.ppFutureSurfaces = futureViews.data();

  std::vector<ComPtr<ID3D11VideoProcessorInputView>> all_views;
  const int start = 2 - futureFrames;
  const int end = 2 + pastFrames;
  int count = 0;

  for (int i = start; i <= end; i++)
  {
    if (!views[i])
      continue;

    ComPtr<ID3D11VideoProcessorInputView> view;
    view.Attach(GetInputView(views[i]));

    if (i > 2)
    {
      // frames order should be { ?, T-3, T-2, T-1 }
      pastViews[2 + pastFrames - i] = view.Get();
    }
    else if (i == 2)
    {
      stream_data.pInputSurface = view.Get();
    }
    else if (i < 2)
    {
      // frames order should be { T+1, T+2, T+3, .. }
      futureViews[1 - i] = view.Get();
    }
    if (view)
    {
      count++;
      all_views.push_back(view);
    }
  }

  if (count != pastFrames + futureFrames + 1)
  {
    CLog::LogF(LOGERROR, "incomplete views set.");
    return false;
  }

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
  m_pVideoContext->VideoProcessorSetStreamFrameFormat(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, dxvaFrameFormat);
  // Source rect
  m_pVideoContext->VideoProcessorSetStreamSourceRect(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, TRUE, &sourceRECT);
  // Stream dest rect
  m_pVideoContext->VideoProcessorSetStreamDestRect(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, TRUE, &dstRECT);
  // Output rect
  m_pVideoContext->VideoProcessorSetOutputTargetRect(m_pVideoProcessor.Get(), TRUE, &dstRECT);

  ComPtr<ID3D11VideoContext1> videoCtx1;
  if (SUCCEEDED(m_pVideoContext.As(&videoCtx1)))
  {
    const DXGI_COLOR_SPACE_TYPE source_color = GetDXGIColorSpace(views[2], m_bSupportHDR10);
    DXGI_COLOR_SPACE_TYPE target_color = DX::Windowing()->UseLimitedColor()
                                             ? DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709
                                             : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    if (DX::DeviceResources::Get()->IsHDROutput())
    {
      if ((views[2]->color_transfer == AVCOL_TRC_SMPTE2084 ||
           views[2]->color_transfer == AVCOL_TRC_ARIB_STD_B67) &&
          views[2]->primaries == AVCOL_PRI_BT2020)
      {
        target_color = DX::Windowing()->UseLimitedColor()
                           ? DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020
                           : DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
      }
      else if (views[2]->primaries == AVCOL_PRI_BT2020)
      {
        target_color = DX::Windowing()->UseLimitedColor()
                           ? DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020
                           : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020;
      }
    }

    videoCtx1->VideoProcessorSetStreamColorSpace1(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX,
                                                  source_color);
    videoCtx1->VideoProcessorSetOutputColorSpace1(m_pVideoProcessor.Get(), target_color);
    // makes target available for processing in shaders
    videoCtx1->VideoProcessorSetOutputShaderUsage(m_pVideoProcessor.Get(), 1);

    if (DX::DeviceResources::Get()->IsHDROutput() && m_bSupportHDR10 &&
        views[2]->color_transfer == AVCOL_TRC_SMPTE2084 && views[2]->primaries == AVCOL_PRI_BT2020)
    {
      ComPtr<ID3D11VideoContext2> videoCtx2;
      if (SUCCEEDED(m_pVideoContext.As(&videoCtx2)))
      {
        // Passes stream SEI HDR metadata to VideoProcessor (refresh changes during playback)
        DXGI_HDR_METADATA_HDR10 hdr10Stream = CRendererBase::GetDXGIHDR10MetaData(views[2]);
        videoCtx2->VideoProcessorSetStreamHDRMetaData(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX,
                                                      DXGI_HDR_METADATA_TYPE_HDR10,
                                                      sizeof(hdr10Stream), &hdr10Stream);

        // Passes Display HDR parameters (EDID) to VideoProcessor
        videoCtx2->VideoProcessorSetOutputHDRMetaData(m_pVideoProcessor.Get(),
                                                      DXGI_HDR_METADATA_TYPE_HDR10,
                                                      sizeof(m_hdr10Display), &m_hdr10Display);
      }
    }
  }
  else
  {
    // input colorspace
    bool isBT601 = views[2]->color_space == AVCOL_SPC_BT470BG || views[2]->color_space == AVCOL_SPC_SMPTE170M;
    D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace
    {
      0,                            // 0 - Playback, 1 - Processing
      views[2]->full_range ? 0 : 1, // 0 - Full (0-255), 1 - Limited (16-235) (RGB)
      isBT601 ? 1 : 0,              // 0 - BT.601, 1 - BT.709
      0,                            // 0 - Conventional YCbCr, 1 - xvYCC
      views[2]->full_range ? 2 : 1  // 0 - driver defaults, 2 - Full range [0-255], 1 - Studio range [16-235] (YUV)
    };
    m_pVideoContext->VideoProcessorSetStreamColorSpace(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, &colorSpace);
    // Output color space
    // don't apply any color range conversion, this will be fixed at later stage.
    colorSpace.Usage = 0;  // 0 - playback, 1 - video processing
    colorSpace.RGB_Range = DX::Windowing()->UseLimitedColor() ? 1 : 0;  // 0 - 0-255, 1 - 16-235
    colorSpace.YCbCr_Matrix = 1;  // 0 - BT.601, 1 = BT.709
    colorSpace.YCbCr_xvYCC = 1;  // 0 - Conventional YCbCr, 1 - xvYCC
    colorSpace.Nominal_Range = 0;  // 2 - 0-255, 1 = 16-235, 0 - undefined
    m_pVideoContext->VideoProcessorSetOutputColorSpace(m_pVideoProcessor.Get(), &colorSpace);
  }

  // brightness
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS, static_cast<int>(brightness), 0, 100, 50);
  // contrast
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST, static_cast<int>(contrast), 0, 100, 50);
  // unused filters
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_HUE, 50, 0, 100, 50);
  ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_SATURATION, 50, 0, 100, 50);
  // Rotation
  m_pVideoContext->VideoProcessorSetStreamRotation(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, rotation != 0,
                                                   static_cast<D3D11_VIDEO_PROCESSOR_ROTATION>(rotation / 90));

  // create output view for surface.
  D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC OutputViewDesc = { D3D11_VPOV_DIMENSION_TEXTURE2D, { 0 }};
  ComPtr<ID3D11VideoProcessorOutputView> pOutputView;
  HRESULT hr = m_pVideoDevice->CreateVideoProcessorOutputView(target, m_pEnumerator.Get(), &OutputViewDesc, &pOutputView);
  if (S_OK != hr)
    CLog::LogF(FAILED(hr) ? LOGERROR : LOGWARNING, "video device returns result '%x' while creating processor output view.", hr);

  if (SUCCEEDED(hr))
  {
    hr = m_pVideoContext->VideoProcessorBlt(m_pVideoProcessor.Get(), pOutputView.Get(), frameIdx, 1, &stream_data);
    if (S_OK != hr)
    {
      CLog::LogF(FAILED(hr) ? LOGERROR : LOGWARNING, "video device returns result '%x' while VideoProcessorBlt execution.", hr);
    }
  }

  return !FAILED(hr);
}
