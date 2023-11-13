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
#include "VideoRenderers/windows/RendererDXVA.h"
#include "rendering/dx/RenderContext.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <mutex>

#include <Windows.h>
#include <d3d11_4.h>
#include <dxgi1_5.h>

using namespace DXVA;
using namespace Microsoft::WRL;

namespace
{
// magic constants taken from Chromium:
// https://chromium.googlesource.com/chromium/src/+/refs/heads/main/ui/gl/swap_chain_presenter.cc#180
constexpr GUID GUID_INTEL_VPE_INTERFACE = {
    0xedd1d4b9, 0x8659, 0x4cbc, {0xa4, 0xd6, 0x98, 0x31, 0xa2, 0x16, 0x3a, 0xc3}};

constexpr UINT kIntelVpeFnVersion = 0x01;
constexpr UINT kIntelVpeFnMode = 0x20;
constexpr UINT kIntelVpeFnScaling = 0x37;
constexpr UINT kIntelVpeVersion3 = 0x0003;
constexpr UINT kIntelVpeModePreproc = 0x01;
constexpr UINT kIntelVpeScalingSuperResolution = 0x2;

constexpr GUID GUID_NVIDIA_PPE_INTERFACE = {
    0xd43ce1b3, 0x1f4b, 0x48ac, {0xba, 0xee, 0xc3, 0xc2, 0x53, 0x75, 0xe6, 0xf7}};

constexpr UINT kStreamExtensionVersionV1 = 0x1;
constexpr UINT kStreamExtensionMethodSuperResolution = 0x2;
} // unnamed namespace

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
  m_enumerator = nullptr;
  Close();
}

void CProcessorHD::Close()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_pVideoProcessor = nullptr;
  m_pVideoContext = nullptr;
  m_pVideoDevice = nullptr;
  m_superResolutionEnabled = false;
  m_configured = false;
}

bool CProcessorHD::InitProcessor()
{
  HRESULT hr{};
  m_pVideoDevice = nullptr;
  m_pVideoContext = nullptr;

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

  if (!m_enumerator)
    return false;

  m_procCaps = m_enumerator->ProbeProcessorCaps();
  if (!m_procCaps.m_valid)
    return false;

  return true;
}

bool CProcessorHD::CheckFormats() const
{
  if (!m_isValidConversion)
    return true;

  // check default output format (as render target)
  return m_enumerator && m_enumerator->IsFormatSupportedOutput(m_conversion.m_outputFormat);
}

bool CProcessorHD::Open(const VideoPicture& picture,
                        std::shared_ptr<DXVA::CEnumeratorHD> enumerator)
{
  Close();

  std::unique_lock<CCriticalSection> lock(m_section);

  m_color_primaries = picture.color_primaries;
  m_color_transfer = picture.color_transfer;
  m_enumerator = enumerator;

  if (!InitProcessor())
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

  if ((!m_pVideoDevice || !m_pVideoContext || !m_enumerator || !m_procCaps.m_valid) && !ReInit())
  {
    CLog::LogF(LOGDEBUG, "invalid state, failed to re-initialize.");
    return false;
  }

  CLog::LogF(LOGDEBUG, "creating processor.");

  // create processor
  m_pVideoProcessor = m_enumerator->CreateVideoProcessor(m_procCaps.m_procIndex);

  if (!m_pVideoProcessor)
  {
    CLog::LogF(LOGDEBUG, "failed creating video processor.");
    return false;
  }

  m_pVideoContext->VideoProcessorSetStreamAutoProcessingMode(m_pVideoProcessor.Get(), 0, FALSE);
  m_pVideoContext->VideoProcessorSetStreamOutputRate(
      m_pVideoProcessor.Get(), 0, D3D11_VIDEO_PROCESSOR_OUTPUT_RATE_NORMAL, FALSE, 0);

  ComPtr<ID3D11VideoContext1> videoCtx1;
  if (SUCCEEDED(m_pVideoContext.As(&videoCtx1)))
  {
    videoCtx1->VideoProcessorSetOutputShaderUsage(m_pVideoProcessor.Get(), 1);
  }
  else
  {
    CLog::LogF(LOGWARNING, "unable to retrieve ID3D11VideoContext1 to allow usage of shaders on "
                           "video processor output surfaces.");
  }

  // Output background color (black)
  D3D11_VIDEO_COLOR color;
  color.YCbCr = { 0.0625f, 0.5f, 0.5f, 1.0f }; // black color
  m_pVideoContext->VideoProcessorSetOutputBackgroundColor(m_pVideoProcessor.Get(), TRUE, &color);

  // AMD/HDR (as of 2023-06-16): processor tone maps by default and modifies high code values
  // We want "passthrough" of the signal and to do our own tone mapping when needed.
  // Disable the functionality by pretending that the display supports all PQ levels (0-10000)
  const DXGI_ADAPTER_DESC ad = DX::DeviceResources::Get()->GetAdapterDesc();
  bool streamIsHDR =
      (m_color_primaries == AVCOL_PRI_BT2020) &&
      (m_color_transfer == AVCOL_TRC_SMPTE2084 || m_color_transfer == AVCOL_TRC_ARIB_STD_B67);

  if (m_procCaps.m_hasMetadataHDR10Support && ad.VendorId == PCIV_AMD && streamIsHDR)
  {
    ComPtr<ID3D11VideoContext2> videoCtx2;
    if (SUCCEEDED(m_pVideoContext.As(&videoCtx2)))
    {
      DXGI_HDR_METADATA_HDR10 hdr10{};
      hdr10.MaxMasteringLuminance = 10000;
      hdr10.MinMasteringLuminance = 0;

      videoCtx2->VideoProcessorSetOutputHDRMetaData(m_pVideoProcessor.Get(),
                                                    DXGI_HDR_METADATA_TYPE_HDR10,
                                                    sizeof(DXGI_HDR_METADATA_HDR10), &hdr10);

      CLog::LogF(LOGDEBUG, "video processor tone mapping disabled.");
    }
    else
    {
      CLog::LogF(LOGDEBUG,
                 "unable to retrieve ID3D11VideoContext2 to disable video processor tone mapping.");
    }
  }

  return true;
}

void CProcessorHD::ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const
{
  if (filter >= static_cast<D3D11_VIDEO_PROCESSOR_FILTER>(NUM_FILTERS))
    return;

  // Unsupported filter. Ignore.
  if (!m_procCaps.m_Filters[filter].bSupported)
    return;

  D3D11_VIDEO_PROCESSOR_FILTER_RANGE range = m_procCaps.m_Filters[filter].Range;
  int val;

  if(value > def)
    val = range.Default + (range.Maximum - range.Default) * (value - def) / (max - def);
  else if(value < def)
    val = range.Default + (range.Minimum - range.Default) * (value - def) / (min - def);
  else
    val = range.Default;

  m_pVideoContext->VideoProcessorSetStreamFilter(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, filter, val != range.Default, val);
}

ComPtr<ID3D11VideoProcessorInputView> CProcessorHD::GetInputView(CRenderBuffer* view) const
{
  if (m_enumerator)
  {
    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC vpivd = {0, D3D11_VPIV_DIMENSION_TEXTURE2D, {0, 0}};

    ComPtr<ID3D11Resource> resource;
    unsigned arrayIdx = 0;
    HRESULT hr = view->GetResource(resource.GetAddressOf(), &arrayIdx);
    if (SUCCEEDED(hr))
    {
      vpivd.Texture2D.ArraySlice = arrayIdx;
      return m_enumerator->CreateVideoProcessorInputView(resource.Get(), &vpivd);
    }
  }
  return {};
}

bool CProcessorHD::CheckVideoParameters(const CRect& src,
                                        const CRect& dst,
                                        const UINT& rotation,
                                        const float& contrast,
                                        const float& brightness,
                                        const CRenderBuffer& rb)
{
  bool updatedParameter{false};

  if (!m_configured || m_lastSrc != src)
  {
    const RECT sourceRECT = {static_cast<LONG>(src.x1), static_cast<LONG>(src.y1),
                             static_cast<LONG>(src.x2), static_cast<LONG>(src.y2)};

    m_pVideoContext->VideoProcessorSetStreamSourceRect(m_pVideoProcessor.Get(),
                                                       DEFAULT_STREAM_INDEX, TRUE, &sourceRECT);
    m_lastSrc = src;
    updatedParameter = true;
  }

  if (!m_configured || m_lastDst != dst)
  {
    const RECT dstRECT = {static_cast<LONG>(dst.x1), static_cast<LONG>(dst.y1),
                          static_cast<LONG>(dst.x2), static_cast<LONG>(dst.y2)};

    m_pVideoContext->VideoProcessorSetStreamDestRect(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX,
                                                     TRUE, &dstRECT);
    m_pVideoContext->VideoProcessorSetOutputTargetRect(m_pVideoProcessor.Get(), TRUE, &dstRECT);

    m_lastDst = dst;
    updatedParameter = true;
  }

  if (!m_configured || m_lastBrightness != brightness)
  {
    ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS, static_cast<int>(brightness), 0, 100, 50);

    m_lastBrightness = brightness;
    updatedParameter = true;
  }

  if (!m_configured || m_lastContrast != contrast)
  {
    ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST, static_cast<int>(contrast), 0, 100, 50);

    m_lastContrast = contrast;
    updatedParameter = true;
  }

  // unused filters - set once and forget
  if (!m_configured)
  {
    ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_HUE, 50, 0, 100, 50);
    ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER_SATURATION, 50, 0, 100, 50);
  }

  if (!m_configured || m_lastRotation != rotation)
  {
    m_pVideoContext->VideoProcessorSetStreamRotation(
        m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX, rotation != 0,
        static_cast<D3D11_VIDEO_PROCESSOR_ROTATION>(rotation / 90));

    m_lastRotation = rotation;
    updatedParameter = true;
  }

  ComPtr<ID3D11VideoContext1> videoCtx1;
  if (SUCCEEDED(m_pVideoContext.As(&videoCtx1)))
  {
    if (!m_configured || m_lastConversion != m_conversion)
    {
      videoCtx1->VideoProcessorSetStreamColorSpace1(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX,
                                                    m_conversion.m_inputCS);
      videoCtx1->VideoProcessorSetOutputColorSpace1(m_pVideoProcessor.Get(),
                                                    m_conversion.m_outputCS);

      m_lastConversion = m_conversion;
      updatedParameter = true;
    }
  }
  else if (!m_configured || m_lastColorSpace != rb.color_space || m_lastFullRange != rb.full_range)
  {
    // input colorspace
    bool isBT601 = rb.color_space == AVCOL_SPC_BT470BG || rb.color_space == AVCOL_SPC_SMPTE170M;
    // clang-format off
    D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace
    {
      0u,                             // 0 - Playback, 1 - Processing
      rb.full_range ? 0u : 1u, // 0 - Full (0-255), 1 - Limited (16-235) (RGB)
      isBT601 ? 1u : 0u,              // 0 - BT.601, 1 - BT.709
      0u,                             // 0 - Conventional YCbCr, 1 - xvYCC
      rb.full_range ? 2u : 1u  // 0 - driver defaults, 2 - Full range [0-255], 1 - Studio range [16-235] (YUV)
    };
    // clang-format on
    m_pVideoContext->VideoProcessorSetStreamColorSpace(m_pVideoProcessor.Get(),
                                                       DEFAULT_STREAM_INDEX, &colorSpace);
    // Output color space
    // don't apply any color range conversion, this will be fixed at later stage.
    colorSpace.Usage = 0; // 0 - playback, 1 - video processing
    colorSpace.RGB_Range = DX::Windowing()->UseLimitedColor() ? 1 : 0; // 0 - 0-255, 1 - 16-235
    colorSpace.YCbCr_Matrix = 1; // 0 - BT.601, 1 = BT.709
    colorSpace.YCbCr_xvYCC = 1; // 0 - Conventional YCbCr, 1 - xvYCC
    colorSpace.Nominal_Range = 0; // 2 - 0-255, 1 = 16-235, 0 - undefined
    m_pVideoContext->VideoProcessorSetOutputColorSpace(m_pVideoProcessor.Get(), &colorSpace);

    m_lastColorSpace = rb.color_space;
    m_lastFullRange = rb.full_range;
    updatedParameter = true;
  }

  return updatedParameter;
}

bool CProcessorHD::Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer** views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  // restore processor if it was lost
  if (!m_pVideoProcessor && !OpenProcessor())
    return false;

  if (!views[2])
    return false;

  const bool updatedParam =
      CheckVideoParameters(src, dst, rotation, contrast, brightness, *views[2]);

  D3D11_VIDEO_FRAME_FORMAT dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;

  if ((flags & RENDER_FLAG_FIELD0) && (flags & RENDER_FLAG_TOP))
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  else if ((flags & RENDER_FLAG_FIELD1) && (flags & RENDER_FLAG_BOT))
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  else if ((flags & RENDER_FLAG_FIELD0) && (flags & RENDER_FLAG_BOT))
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;
  else if ((flags & RENDER_FLAG_FIELD1) && (flags & RENDER_FLAG_TOP))
    dxvaFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST;

  m_pVideoContext->VideoProcessorSetStreamFrameFormat(m_pVideoProcessor.Get(), DEFAULT_STREAM_INDEX,
                                                      dxvaFrameFormat);

  D3D11_VIDEO_PROCESSOR_STREAM stream_data = {};
  stream_data.Enable = TRUE;

  const bool frameProgressive = dxvaFrameFormat == D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;

  // Progressive or Interlaced video at normal rate.
  const bool secondField = ((flags & RENDER_FLAG_FIELD1) && !frameProgressive) ? 1 : 0;
  stream_data.InputFrameOrField = frameIdx + (secondField ? 1 : 0);
  stream_data.OutputIndex = secondField;

  // Render() gets called once for each displayed frame, following the pattern necessary to adapt
  // the source fps to the display fps, with repetitions as needed (ex. 3:2 for 23.98fps at 59Hz)
  // However there is no need to render the same frame more than once, the intermediate target is
  // not cleared and the output of the previous processing is still there.
  // For nVidia deinterlacing it's more than an optimization. The processor must see each field
  // only once or it won't switch from bob to a more advanced algorithm.
  // for ex. when playing 25i at 60fps, decoded frames A B => output A0 A1 B0 B1 B1
  // B1 field is repeated and the second B1 must be skipped.
  // Exception: always process when a parameter changes to provide immediate feedback to the user

  if (m_configured && m_lastInputFrameOrField == stream_data.InputFrameOrField &&
      m_lastOutputIndex == stream_data.OutputIndex && !updatedParam)
    return true;

  m_lastInputFrameOrField = stream_data.InputFrameOrField;
  m_lastOutputIndex = stream_data.OutputIndex;

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
  const int futureFrames = std::min(providedFuture, m_procCaps.m_rateCaps.FutureFrames);
  const int pastFrames = std::min(providedPast, m_procCaps.m_rateCaps.PastFrames);
  std::vector<ID3D11VideoProcessorInputView*> pastViews(pastFrames, nullptr);
  std::vector<ID3D11VideoProcessorInputView*> futureViews(futureFrames, nullptr);

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

    ComPtr<ID3D11VideoProcessorInputView> view = GetInputView(views[i]);

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

  // create output view for surface.
  const D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputViewDesc = {D3D11_VPOV_DIMENSION_TEXTURE2D,
                                                                 {0}};
  ComPtr<ID3D11VideoProcessorOutputView> pOutputView =
      m_enumerator->CreateVideoProcessorOutputView(target, &outputViewDesc);

  HRESULT hr{};
  if (pOutputView)
  {
    hr = m_pVideoContext->VideoProcessorBlt(m_pVideoProcessor.Get(), pOutputView.Get(), 0, 1,
                                            &stream_data);
    if (S_OK != hr)
    {
      CLog::LogF(FAILED(hr) ? LOGERROR : LOGWARNING,
                 "VideoProcessorBlt returned {} while VideoProcessorBlt execution.",
                 DX::GetErrorDescription(hr));
    }
  }

  if (!m_configured)
    m_configured = true;

  return pOutputView && !FAILED(hr);
}

bool CProcessorHD::IsSuperResolutionSuitable(const VideoPicture& picture)
{
  if (picture.iWidth > 1920)
    return false;

  const UINT outputWidth = DX::Windowing()->GetBackBuffer().GetWidth();

  if (outputWidth <= picture.iWidth)
    return false;

  if (picture.color_primaries == AVCOL_PRI_BT2020 ||
      picture.color_transfer == AVCOL_TRC_SMPTE2084 ||
      picture.color_transfer == AVCOL_TRC_ARIB_STD_B67)
    return false;

  return true;
}

void CProcessorHD::TryEnableVideoSuperResolution()
{
  if (!m_pVideoContext || !m_pVideoProcessor)
    return;

  const DXGI_ADAPTER_DESC ad = DX::DeviceResources::Get()->GetAdapterDesc();

  if (ad.VendorId == PCIV_Intel)
  {
    EnableIntelVideoSuperResolution();
  }
  else if (ad.VendorId == PCIV_NVIDIA)
  {
    EnableNvidiaRTXVideoSuperResolution();
  }
}

void CProcessorHD::EnableIntelVideoSuperResolution()
{
  UINT param = 0;

  struct IntelVpeExt
  {
    UINT function;
    void* param;
  };

  IntelVpeExt ext{0, &param};

  ext.function = kIntelVpeFnVersion;
  param = kIntelVpeVersion3;

  HRESULT hr = m_pVideoContext->VideoProcessorSetOutputExtension(
      m_pVideoProcessor.Get(), &GUID_INTEL_VPE_INTERFACE, sizeof(ext), &ext);
  if (FAILED(hr))
  {
    CLog::LogF(LOGWARNING, "Failed to set the Intel VPE version with error {}.",
               DX::GetErrorDescription(hr));
    return;
  }

  ext.function = kIntelVpeFnMode;
  param = kIntelVpeModePreproc;

  hr = m_pVideoContext->VideoProcessorSetOutputExtension(
      m_pVideoProcessor.Get(), &GUID_INTEL_VPE_INTERFACE, sizeof(ext), &ext);
  if (FAILED(hr))
  {
    CLog::LogF(LOGWARNING, "Failed to set the Intel VPE mode with error {}.",
               DX::GetErrorDescription(hr));
    return;
  }

  ext.function = kIntelVpeFnScaling;
  param = kIntelVpeScalingSuperResolution;

  hr = m_pVideoContext->VideoProcessorSetStreamExtension(
      m_pVideoProcessor.Get(), 0, &GUID_INTEL_VPE_INTERFACE, sizeof(ext), &ext);
  if (FAILED(hr))
  {
    CLog::LogF(LOGWARNING, "Failed to set the Intel VPE scaling type with error {}.",
               DX::GetErrorDescription(hr));
    return;
  }

  CLog::LogF(LOGINFO, "Intel Video Super Resolution request enable successfully");
  m_superResolutionEnabled = true;
}

void CProcessorHD::EnableNvidiaRTXVideoSuperResolution()
{
  struct NvidiaStreamExt
  {
    UINT version;
    UINT method;
    UINT enable;
  };

  NvidiaStreamExt ext = {kStreamExtensionVersionV1, kStreamExtensionMethodSuperResolution, 1u};

  HRESULT hr = m_pVideoContext->VideoProcessorSetStreamExtension(
      m_pVideoProcessor.Get(), 0, &GUID_NVIDIA_PPE_INTERFACE, sizeof(ext), &ext);
  if (FAILED(hr))
  {
    CLog::LogF(LOGWARNING, "Failed to set the NVIDIA video process stream extension with error {}.",
               DX::GetErrorDescription(hr));
    return;
  }

  CLog::LogF(LOGINFO, "RTX Video Super Resolution request enable successfully");
  m_superResolutionEnabled = true;
}

bool CProcessorHD::SetConversion(const ProcessorConversion& conversion)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  if (!m_enumerator)
    return false;

  if (!m_enumerator || !m_enumerator->IsFormatSupportedInput(conversion.m_inputFormat) ||
      !m_enumerator->IsFormatSupportedOutput(conversion.m_outputFormat))
    return false;

  if (m_enumerator->IsEnumerator1Available() &&
      !m_enumerator->CheckConversion(conversion.m_inputFormat, conversion.m_inputCS,
                                     conversion.m_outputFormat, conversion.m_outputCS))
  {
    CLog::LogF(LOGERROR, "Conversion {} is not supported", conversion.ToString());
    return false;
  }

  m_conversion = conversion;
  m_isValidConversion = true;

  return true;
}

bool CProcessorHD::Supports(ERENDERFEATURE feature) const
{
  switch (feature)
  {
    case RENDERFEATURE_BRIGHTNESS:
      return m_procCaps.m_Filters[D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS].bSupported;
    case RENDERFEATURE_CONTRAST:
      return m_procCaps.m_Filters[D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST].bSupported;
    case RENDERFEATURE_ROTATION:
      return (m_procCaps.m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ROTATION);
    default:
      return false;
  }
}
