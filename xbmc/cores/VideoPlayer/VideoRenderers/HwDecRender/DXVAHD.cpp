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

#include "VideoRenderers/RenderFlags.h"
#include "VideoRenderers/RenderManager.h"
#include "VideoRenderers/windows/RendererBase.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

#include <mutex>

#include <Windows.h>
#include <d3d11_4.h>
#include <dxgi1_5.h>

using namespace DXVA;
using namespace Microsoft::WRL;

#define LOGIFERROR(a) \
  do \
  { \
    HRESULT res = a; \
    if (FAILED(res)) \
    { \
      CLog::LogF(LOGERROR, "failed executing " #a " at line {} with error {:x}", __LINE__, res); \
    } \
  } while (0);

struct DXVA::ProcColorSpaces
{
  DXGI_COLOR_SPACE_TYPE inputColorSpace;
  DXGI_COLOR_SPACE_TYPE outputColorSpace;
};

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
  std::unique_lock<CCriticalSection> lock(m_section);
  Close();
}

void CProcessorHD::Close()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_pEnumerator = nullptr;
  m_pVideoProcessor = nullptr;
  m_pVideoContext = nullptr;
  m_pVideoDevice = nullptr;
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
  HRESULT hr{};
  m_pVideoDevice = nullptr;
  m_pVideoContext = nullptr;
  m_pEnumerator = nullptr;

  ComPtr<ID3D11DeviceContext1> pD3DDeviceContext = DX::DeviceResources::Get()->GetImmediateContext();
  ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(hr = pD3DDeviceContext.As(&m_pVideoContext)))
  {
    CLog::LogF(LOGWARNING, "video context initialization is failed. Error {}",
               DX::GetErrorDescription(hr));
    return false;
  }
  if (FAILED(hr = pD3DDevice.As(&m_pVideoDevice)))
  {
    CLog::LogF(LOGWARNING, "video device initialization is failed. Error {}",
               DX::GetErrorDescription(hr));
    return false;
  }

  CLog::LogF(LOGDEBUG, "initing video enumerator with params: {}x{}.", m_width, m_height);

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc = {};
  contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  contentDesc.InputWidth = m_width;
  contentDesc.InputHeight = m_height;
  contentDesc.OutputWidth = m_width;
  contentDesc.OutputHeight = m_height;
  contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  if (FAILED(hr = m_pVideoDevice->CreateVideoProcessorEnumerator(
                 &contentDesc, m_pEnumerator.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGWARNING, "failed to init video enumerator with params: {}x{}. Error {}", m_width,
               m_height, DX::GetErrorDescription(hr));
    return false;
  }

  if (CServiceBroker::GetLogging().IsLogLevelLogged(LOGDEBUG))
  {
    std::string inputFormats{};
    std::string outputFormats{};
    for (int fmt = DXGI_FORMAT_UNKNOWN; fmt <= DXGI_FORMAT_V408; fmt++)
    {
      UINT uiFlags;
      if (S_OK == m_pEnumerator->CheckVideoProcessorFormat((DXGI_FORMAT)fmt, &uiFlags))
      {
        if (uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT)
        {
          inputFormats.append("\n");
          inputFormats.append(DX::DXGIFormatToString((DXGI_FORMAT)fmt));
        }
        if (uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT)
        {
          outputFormats.append("\n");
          outputFormats.append(DX::DXGIFormatToString((DXGI_FORMAT)fmt));
        }
      }
    }
    CLog::LogF(LOGDEBUG, "Supported input formats:{}", inputFormats);
    CLog::LogF(LOGDEBUG, "Supported output formats:{}", outputFormats);
  }

  if (FAILED(hr = m_pEnumerator->GetVideoProcessorCaps(&m_vcaps)))
  {
    CLog::LogF(LOGWARNING, "failed to get processor caps. Error {}", DX::GetErrorDescription(hr));
    return false;
  }

  CLog::LogF(LOGDEBUG, "video processor has {} rate conversion.", m_vcaps.RateConversionCapsCount);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} feature caps.", m_vcaps.FeatureCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} device caps.", m_vcaps.DeviceCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} input format caps.", m_vcaps.InputFormatCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} auto stream caps.", m_vcaps.AutoStreamCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} stereo caps.", m_vcaps.StereoCaps);
  CLog::LogF(LOGDEBUG, "video processor has {} max input streams.", m_vcaps.MaxInputStreams);
  CLog::LogF(LOGDEBUG, "video processor has {} max stream states.", m_vcaps.MaxStreamStates);
  if (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_METADATA_HDR10)
    CLog::LogF(LOGDEBUG, "video processor supports HDR10.");

  if (0 != (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LEGACY))
    CLog::LogF(LOGWARNING, "the video driver does not support full video processing capabilities.");

  if (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_STEREO)
    CLog::LogF(LOGDEBUG, "video processor supports stereo.");

  if (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ROTATION)
    CLog::LogF(LOGDEBUG, "video processor supports rotation.");

  if (m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_SHADER_USAGE)
    CLog::LogF(LOGDEBUG, "video processor supports shader usage.");

  m_max_back_refs = 0;
  m_max_fwd_refs = 0;
  m_procIndex = 0;

  const UINT deinterlacingCaps =
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BLEND |
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BOB |
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_ADAPTIVE |
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_MOTION_COMPENSATION;
  unsigned maxProcCaps = 0;
  // try to find best processor
  for (unsigned int i = 0; i < m_vcaps.RateConversionCapsCount; i++)
  {
    D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS convCaps;
    LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(i, &convCaps))

    // check only deinterlace caps
    if ((convCaps.ProcessorCaps & deinterlacingCaps) > maxProcCaps)
    {
      m_procIndex = i;
      maxProcCaps = convCaps.ProcessorCaps & deinterlacingCaps;
    }
  }

  CLog::LogF(LOGDEBUG, "selected video processor index: {}.", m_procIndex);

  LOGIFERROR(m_pEnumerator->GetVideoProcessorRateConversionCaps(m_procIndex, &m_rateCaps))
  m_max_fwd_refs  = std::min(m_rateCaps.FutureFrames, 2u);
  m_max_back_refs = std::min(m_rateCaps.PastFrames,  4u);

  CLog::LogF(LOGINFO, "supported deinterlace methods: blend:{}, bob:{}, adaptive:{}, mocomp:{}.",
             (m_rateCaps.ProcessorCaps & 0x1) != 0 ? "yes" : "no", // BLEND
             (m_rateCaps.ProcessorCaps & 0x2) != 0 ? "yes" : "no", // BOB
             (m_rateCaps.ProcessorCaps & 0x4) != 0 ? "yes" : "no", // ADAPTIVE
             (m_rateCaps.ProcessorCaps & 0x8) != 0 ? "yes" : "no" // MOTION_COMPENSATION
  );

  CLog::LogF(LOGDEBUG, "selected video processor allows {} future frames and {} past frames.",
             m_rateCaps.FutureFrames, m_rateCaps.PastFrames);

  //m_size = m_max_back_refs + 1 + m_max_fwd_refs;  // refs + 1 display

  // Get the image filtering capabilities.
  for (size_t i = 0; i < NUM_FILTERS; i++)
  {
    if (m_vcaps.FilterCaps & PROCAMP_FILTERS[i].cap)
    {
      m_Filters[i].Range = {};
      m_Filters[i].bSupported = SUCCEEDED(m_pEnumerator->GetVideoProcessorFilterRange(
          PROCAMP_FILTERS[i].filter, &m_Filters[i].Range));

      if (m_Filters[i].bSupported)
      {
        CLog::LogF(LOGDEBUG, "filter {} has following params - max: {}, min: {}, default: {}",
                   PROCAMP_FILTERS[i].name, m_Filters[i].Range.Maximum, m_Filters[i].Range.Minimum,
                   m_Filters[i].Range.Default);
      }
    }
    else
    {
      CLog::LogF(LOGDEBUG, "filter {} not supported by processor.", PROCAMP_FILTERS[i].name);
      m_Filters[i].bSupported = false;
    }
  }

  ComPtr<ID3D11VideoProcessorEnumerator1> pEnumerator1;

  if (SUCCEEDED(m_pEnumerator.As(&pEnumerator1)))
  {
    DXGI_FORMAT format = DX::Windowing()->GetBackBuffer().GetFormat();
    BOOL supported = 0;
    HRESULT hr;

    // Check if HLG color space conversion is supported by driver
    hr = pEnumerator1->CheckVideoProcessorFormatConversion(
        DXGI_FORMAT_P010, DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020, format,
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, &supported);
    m_bSupportHLG = SUCCEEDED(hr) && !!supported;

    // Check if HDR10 RGB limited range output is supported by driver
    hr = pEnumerator1->CheckVideoProcessorFormatConversion(
        DXGI_FORMAT_P010, DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020, format,
        DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020, &supported);
    m_bSupportHDR10Limited = SUCCEEDED(hr) && !!supported;
  }

  CLog::LogF(LOGDEBUG, "HLG color space conversion is{}supported.", m_bSupportHLG ? " " : " NOT ");
  CLog::LogF(LOGDEBUG, "HDR10 RGB limited range output is{}supported.",
             m_bSupportHDR10Limited ? " " : " NOT ");

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

  CLog::LogF(LOGERROR, "unsupported format {} for {}.", DX::DXGIFormatToString(format),
             DX::D3D11VideoProcessorFormatSupportToString(support));
  return false;
}

bool CProcessorHD::CheckFormats() const
{
  // check default output format (as render target)
  return IsFormatSupported(DX::Windowing()->GetBackBuffer().GetFormat(), D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT);
}

bool CProcessorHD::IsFormatConversionSupported(DXGI_FORMAT inputFormat,
                                               DXGI_FORMAT outputFormat,
                                               const VideoPicture& picture) const
{
  // accept the conversion unless the API can be called successfully and disallows it
  BOOL supported{TRUE};
  ComPtr<ID3D11VideoProcessorEnumerator1> enumerator1;

  if (SUCCEEDED(m_pEnumerator.As(&enumerator1)))
  {
    ProcColorSpaces spaces = CalculateDXGIColorSpaces(DXGIColorSpaceArgs(picture));

    HRESULT hr;
    if (SUCCEEDED(hr = enumerator1->CheckVideoProcessorFormatConversion(
                      inputFormat, spaces.inputColorSpace, outputFormat, spaces.outputColorSpace,
                      &supported)))
    {
      CLog::LogF(LOGDEBUG, "conversion from {} / {} to {} / {} is {}supported.",
                 DX::DXGIFormatToString(inputFormat),
                 DX::DXGIColorSpaceTypeToString(spaces.inputColorSpace),
                 DX::DXGIFormatToString(outputFormat),
                 DX::DXGIColorSpaceTypeToString(spaces.outputColorSpace), supported ? "" : "not ");
    }
    else
    {
      CLog::LogF(LOGERROR, "unable to validate the format conversion, error {}",
                 DX::GetErrorDescription(hr));
    }
  }
  else
  {
    CLog::LogF(
        LOGDEBUG,
        "ID3D11VideoProcessorEnumerator1 not available on this system, accepting the conversion.");
  }
  return supported;
}

bool CProcessorHD::Open(UINT width, UINT height)
{
  Close();

  std::unique_lock<CCriticalSection> lock(m_section);

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
  std::unique_lock<CCriticalSection> lock(m_section);
  Close();

  if (!InitProcessor())
    return false;

  if (!CheckFormats())
    return false;

  return true;
}

bool CProcessorHD::OpenProcessor()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  // restore the device if it was lost
  if (!m_pEnumerator && !ReInit())
    return false;

  CLog::LogF(LOGDEBUG, "creating processor.");

  // create processor
  HRESULT hr = m_pVideoDevice->CreateVideoProcessor(m_pEnumerator.Get(), m_procIndex,
                                                    m_pVideoProcessor.ReleaseAndGetAddressOf());
  if (FAILED(hr))
  {
    CLog::LogF(LOGDEBUG, "failed creating video processor with error {}.",
               DX::GetErrorDescription(hr));
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
    CLog::LogF(LOGERROR, "CreateVideoProcessorInputView returned {}.", DX::GetErrorDescription(hr));

  return inputView.Detach();
}

DXGI_COLOR_SPACE_TYPE CProcessorHD::GetDXGIColorSpaceSource(const DXGIColorSpaceArgs& csArgs,
                                                            bool supportHDR,
                                                            bool supportHLG)
{
  // RGB
  if (csArgs.color_space == AVCOL_SPC_RGB)
  {
    if (!csArgs.full_range)
    {
      if (csArgs.primaries == AVCOL_PRI_BT2020)
      {
        if (csArgs.color_transfer == AVCOL_TRC_SMPTEST2084 && supportHDR)
          return DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020;

        return DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020;
      }
      return DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709;
    }

    if (csArgs.primaries == AVCOL_PRI_BT2020)
    {
      if (csArgs.color_transfer == AVCOL_TRC_SMPTEST2084)
        return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

      return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020;
    }
    if (csArgs.color_transfer == AVCOL_TRC_LINEAR || csArgs.color_transfer == AVCOL_TRC_LOG)
      return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

    return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
  }
  // UHDTV
  if (csArgs.primaries == AVCOL_PRI_BT2020)
  {
    // Windows 10 doesn't support HLG passthrough, always is used PQ for HDR passthrough
    if ((csArgs.color_transfer == AVCOL_TRC_SMPTEST2084 ||
         csArgs.color_transfer == AVCOL_TRC_ARIB_STD_B67) &&
        supportHDR) // is HDR display ON
      return DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020;

    // HLG transfer can be used for HLG source in SDR display if is supported
    if (csArgs.color_transfer == AVCOL_TRC_ARIB_STD_B67 && supportHLG) // driver supports HLG
      return DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020;

    if (csArgs.full_range)
      return DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020;

    return DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020;
  }
  // SDTV
  if (csArgs.primaries == AVCOL_PRI_BT470BG || csArgs.primaries == AVCOL_PRI_SMPTE170M)
  {
    if (csArgs.full_range)
      return DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601;

    return DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601;
  }
  // HDTV
  if (csArgs.full_range)
  {
    if (csArgs.color_transfer == AVCOL_TRC_SMPTE170M)
      return DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601;

    return DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709;
  }

  return DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
}

DXGI_COLOR_SPACE_TYPE CProcessorHD::GetDXGIColorSpaceTarget(const DXGIColorSpaceArgs& csArgs,
                                                            bool supportHDR)
{
  DXGI_COLOR_SPACE_TYPE color;

  color = DX::Windowing()->UseLimitedColor() ? DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709
                                             : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

  if (!DX::Windowing()->IsHDROutput())
    return color;

  // HDR10 or HLG
  if (csArgs.primaries == AVCOL_PRI_BT2020 && (csArgs.color_transfer == AVCOL_TRC_SMPTE2084 ||
                                               csArgs.color_transfer == AVCOL_TRC_ARIB_STD_B67))
  {
    if (supportHDR)
    {
      color = DX::Windowing()->UseLimitedColor() ? DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020
                                                 : DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    }
    else
    {
      color = DX::Windowing()->UseLimitedColor() ? DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020
                                                 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020;
    }
  }

  return color;
}

bool CProcessorHD::Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer** views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  // restore processor if it was lost
  if (!m_pVideoProcessor && !OpenProcessor())
    return false;

  if (!views[2])
    return false;

  RECT sourceRECT = {static_cast<LONG>(src.x1), static_cast<LONG>(src.y1),
                     static_cast<LONG>(src.x2), static_cast<LONG>(src.y2)};
  RECT dstRECT = {static_cast<LONG>(dst.x1), static_cast<LONG>(dst.y1), static_cast<LONG>(dst.x2),
                  static_cast<LONG>(dst.y2)};

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
    ProcColorSpaces spaces = CalculateDXGIColorSpaces(DXGIColorSpaceArgs(*views[2]));

    videoCtx1->VideoProcessorSetStreamColorSpace1(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX,
                                                  spaces.inputColorSpace);
    videoCtx1->VideoProcessorSetOutputColorSpace1(m_pVideoProcessor.Get(), spaces.outputColorSpace);
    // makes target available for processing in shaders
    videoCtx1->VideoProcessorSetOutputShaderUsage(m_pVideoProcessor.Get(), 1);
  }
  else
  {
    // input colorspace
    bool isBT601 = views[2]->color_space == AVCOL_SPC_BT470BG || views[2]->color_space == AVCOL_SPC_SMPTE170M;
    // clang-format off
    D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace
    {
      0u,                             // 0 - Playback, 1 - Processing
      views[2]->full_range ? 0u : 1u, // 0 - Full (0-255), 1 - Limited (16-235) (RGB)
      isBT601 ? 1u : 0u,              // 0 - BT.601, 1 - BT.709
      0u,                             // 0 - Conventional YCbCr, 1 - xvYCC
      views[2]->full_range ? 2u : 1u  // 0 - driver defaults, 2 - Full range [0-255], 1 - Studio range [16-235] (YUV)
    };
    // clang-format on
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
    CLog::LogF(FAILED(hr) ? LOGERROR : LOGWARNING, "CreateVideoProcessorOutputView returned {}.",
               DX::GetErrorDescription(hr));

  if (SUCCEEDED(hr))
  {
    hr = m_pVideoContext->VideoProcessorBlt(m_pVideoProcessor.Get(), pOutputView.Get(), frameIdx, 1, &stream_data);
    if (S_OK != hr)
    {
      CLog::LogF(FAILED(hr) ? LOGERROR : LOGWARNING,
                 "VideoProcessorBlt returned {} while VideoProcessorBlt execution.",
                 DX::GetErrorDescription(hr));
    }
  }

  return !FAILED(hr);
}

ProcColorSpaces CProcessorHD::CalculateDXGIColorSpaces(const DXGIColorSpaceArgs& csArgs) const
{
  bool supportHDR = DX::Windowing()->IsHDROutput() &&
                    (m_bSupportHDR10Limited || !DX::Windowing()->UseLimitedColor());

  return ProcColorSpaces{GetDXGIColorSpaceSource(csArgs, supportHDR, m_bSupportHLG),
                         GetDXGIColorSpaceTarget(csArgs, supportHDR)};
}
