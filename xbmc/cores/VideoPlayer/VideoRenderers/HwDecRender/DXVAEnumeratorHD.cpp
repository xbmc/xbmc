/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DXVAEnumeratorHD.h"

#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

#include <mutex>

#include <Windows.h>
#include <d3d11_4.h>
#include <dxgi1_5.h>

using namespace DXVA;
using namespace Microsoft::WRL;

CEnumeratorHD::CEnumeratorHD()
{
  DX::Windowing()->Register(this);
}

CEnumeratorHD::~CEnumeratorHD()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  DX::Windowing()->Unregister(this);
  UnInit();
}

void CEnumeratorHD::UnInit()
{
  Close();
}

void CEnumeratorHD::Close()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  m_pEnumerator1 = nullptr;
  m_pEnumerator = nullptr;
}

bool CEnumeratorHD::Open(unsigned int width, unsigned int height, DXGI_FORMAT input_dxgi_format)
{
  Close();

  std::unique_lock<CCriticalSection> lock(m_section);

  HRESULT hr{};
  ComPtr<ID3D11Device> pD3DDevice = DX::DeviceResources::Get()->GetD3DDevice();
  ComPtr<ID3D11VideoDevice> pVideoDevice;

  if (FAILED(hr = pD3DDevice.As(&pVideoDevice)))
  {
    CLog::LogF(LOGWARNING, "video device initialization is failed. Error {}",
               DX::GetErrorDescription(hr));
    return false;
  }

  CLog::LogF(LOGDEBUG, "initializing video enumerator with params: {}x{}.", width, height);

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc = {};
  contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;
  contentDesc.InputWidth = width;
  contentDesc.InputHeight = height;
  contentDesc.OutputWidth = width;
  contentDesc.OutputHeight = height;
  contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  if (FAILED(hr = pVideoDevice->CreateVideoProcessorEnumerator(
                 &contentDesc, m_pEnumerator.ReleaseAndGetAddressOf())))
  {
    CLog::LogF(LOGWARNING, "failed to init video enumerator with params: {}x{}. Error {}", width,
               height, DX::GetErrorDescription(hr));
    return false;
  }

  if (FAILED(hr = m_pEnumerator.As(&m_pEnumerator1)))
  {
    CLog::LogF(LOGDEBUG, "ID3D11VideoProcessorEnumerator1 not available on this system. Message {}",
               DX::GetErrorDescription(hr));
  }

  m_input_dxgi_format = input_dxgi_format;

  return true;
}

ProcessorCapabilities CEnumeratorHD::ProbeProcessorCaps()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  if (!m_pEnumerator)
    return {};

  HRESULT hr{};
  ProcessorCapabilities result{};

  if (CServiceBroker::GetLogging().IsLogLevelLogged(LOGDEBUG) &&
      CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
  {
    ProcessorFormats formats = GetProcessorFormats(true, true);

    if (formats.m_valid)
    {
      CLog::LogFC(LOGDEBUG, LOGVIDEO, "Supported input formats:");

      for (const auto& it : formats.m_input)
        CLog::LogFC(LOGDEBUG, LOGVIDEO, "{}", DX::DXGIFormatToString(it));

      CLog::LogFC(LOGDEBUG, LOGVIDEO, "Supported output formats:");

      for (const auto& it : formats.m_output)
        CLog::LogFC(LOGDEBUG, LOGVIDEO, "{}", DX::DXGIFormatToString(it));
    }
  }

  if (FAILED(hr = m_pEnumerator->GetVideoProcessorCaps(&result.m_vcaps)))
  {
    CLog::LogF(LOGWARNING, "failed to get processor caps. Error {}", DX::GetErrorDescription(hr));
    return {};
  }

  CLog::LogF(LOGDEBUG, "video processor has {} rate conversion.",
             result.m_vcaps.RateConversionCapsCount);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} feature caps.", result.m_vcaps.FeatureCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} device caps.", result.m_vcaps.DeviceCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} input format caps.",
             result.m_vcaps.InputFormatCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} auto stream caps.",
             result.m_vcaps.AutoStreamCaps);
  CLog::LogF(LOGDEBUG, "video processor has {:#x} stereo caps.", result.m_vcaps.StereoCaps);
  CLog::LogF(LOGDEBUG, "video processor has {} max input streams.", result.m_vcaps.MaxInputStreams);
  CLog::LogF(LOGDEBUG, "video processor has {} max stream states.", result.m_vcaps.MaxStreamStates);
  if (result.m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_METADATA_HDR10)
  {
    CLog::LogF(LOGDEBUG, "video processor supports HDR10 metadata.");
    result.m_hasMetadataHDR10Support = true;
  }

  if (0 != (result.m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LEGACY))
    CLog::LogF(LOGWARNING, "the video driver does not support full video processing capabilities.");

  if (result.m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_STEREO)
    CLog::LogF(LOGDEBUG, "video processor supports stereo.");

  if (result.m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ROTATION)
    CLog::LogF(LOGDEBUG, "video processor supports rotation.");

  if (result.m_vcaps.FeatureCaps & D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_SHADER_USAGE)
    CLog::LogF(LOGDEBUG, "video processor supports shader usage.");

  const UINT deinterlacingCaps =
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BLEND |
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BOB |
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_ADAPTIVE |
      D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_MOTION_COMPENSATION;
  unsigned maxProcCaps = 0;
  // try to find best processor
  for (unsigned int i = 0; i < result.m_vcaps.RateConversionCapsCount; i++)
  {
    D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS convCaps;
    if (FAILED(hr = m_pEnumerator->GetVideoProcessorRateConversionCaps(i, &convCaps)))
    {
      CLog::LogF(LOGWARNING, "unable to retrieve processor rate conversion caps {}. Error {}", i,
                 DX::GetErrorDescription(hr));
      continue;
    }

    // check only deinterlace caps
    if ((convCaps.ProcessorCaps & deinterlacingCaps) > maxProcCaps)
    {
      result.m_procIndex = i;
      maxProcCaps = convCaps.ProcessorCaps & deinterlacingCaps;
    }
  }

  CLog::LogF(LOGDEBUG, "selected video processor index: {}.", result.m_procIndex);

  if (SUCCEEDED(hr = m_pEnumerator->GetVideoProcessorRateConversionCaps(result.m_procIndex,
                                                                        &result.m_rateCaps)))
  {
    CLog::LogF(LOGINFO, "supported deinterlace methods: blend:{}, bob:{}, adaptive:{}, mocomp:{}.",
               (result.m_rateCaps.ProcessorCaps & 0x1) != 0 ? "yes" : "no", // BLEND
               (result.m_rateCaps.ProcessorCaps & 0x2) != 0 ? "yes" : "no", // BOB
               (result.m_rateCaps.ProcessorCaps & 0x4) != 0 ? "yes" : "no", // ADAPTIVE
               (result.m_rateCaps.ProcessorCaps & 0x8) != 0 ? "yes" : "no" // MOTION_COMPENSATION
    );
  }
  else
  {
    CLog::LogF(LOGWARNING,
               "unable to retrieve processor rate conversion caps {}, the deinterlacing "
               "capabilities are unknown. Error {}",
               result.m_procIndex, DX::GetErrorDescription(hr));
  }

  CLog::LogF(
      LOGDEBUG,
      "selected video processor recommends {} future frames and {} past frames for best results.",
      result.m_rateCaps.FutureFrames, result.m_rateCaps.PastFrames);

  // Get the image filtering capabilities.
  for (size_t i = 0; i < NUM_FILTERS; i++)
  {
    if (result.m_vcaps.FilterCaps & PROCAMP_FILTERS[i].cap)
    {
      result.m_Filters[i].Range = {};
      result.m_Filters[i].bSupported = SUCCEEDED(m_pEnumerator->GetVideoProcessorFilterRange(
          PROCAMP_FILTERS[i].filter, &result.m_Filters[i].Range));

      if (result.m_Filters[i].bSupported)
      {
        CLog::LogF(LOGDEBUG, "filter {} has following params - max: {}, min: {}, default: {}",
                   PROCAMP_FILTERS[i].name, result.m_Filters[i].Range.Maximum,
                   result.m_Filters[i].Range.Minimum, result.m_Filters[i].Range.Default);
      }
    }
    else
    {
      CLog::LogF(LOGDEBUG, "filter {} not supported by processor.", PROCAMP_FILTERS[i].name);
      result.m_Filters[i].bSupported = false;
    }
  }

  if (m_pEnumerator1)
  {
    DXGI_FORMAT format = DX::Windowing()->GetBackBuffer().GetFormat();

    // Check if HLG color space conversion is supported by driver
    result.m_bSupportHLG =
        CheckConversionInternal(DXGI_FORMAT_P010, DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020,
                                format, DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);

    CLog::LogF(LOGDEBUG, "HLG color space conversion is{}supported.",
               result.m_bSupportHLG ? " " : " NOT ");

    result.m_BT2020Left = (QueryHDRtoSDRSupport() == Left);
    result.m_HDR10Left = (QueryHDRtoHDRSupport() == Left);
  }

  result.m_valid = true;

  return result;
}

DXVA::InputFormat CEnumeratorHD::QueryHDRtoHDRSupport() const
{
  // Not initialized yet
  if (!m_pEnumerator)
    return None;

  if (!m_pEnumerator1)
  {
    CLog::LogF(LOGWARNING,
               "ID3D11VideoProcessorEnumerator1 required to evaluate support is not available.");
    return None;
  }

  const DXGI_COLOR_SPACE_TYPE destColor = DX::Windowing()->UseLimitedColor()
                                              ? DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020
                                              : DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

  // Check if HDR10 (TOP LEFT) input color space is supported by video driver
  const bool HDRtopLeft = CheckConversionInternal(m_input_dxgi_format,
                                                  DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020,
                                                  DXGI_FORMAT_R10G10B10A2_UNORM, destColor);

  // Check if HDR10 (LEFT) input color space is supported by video driver
  const bool HDRleft =
      CheckConversionInternal(m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020,
                              DXGI_FORMAT_R10G10B10A2_UNORM, destColor);

  CLog::LogF(LOGDEBUG,
             "HDR10 input color spaces support with HDR {} range output: "
             "YCBCR_STUDIO_G2084_LEFT_P2020: {}, "
             "YCBCR_STUDIO_G2084_TOPLEFT_P2020: {}",
             DX::Windowing()->UseLimitedColor() ? "limited" : "full", HDRleft ? "yes" : "no",
             HDRtopLeft ? "yes" : "no");

  if (HDRtopLeft)
  {
    if (HDRleft)
      CLog::LogF(LOGDEBUG, "Preferred UHD Blu-Ray top left chroma siting will be used");

    return TopLeft;
  }
  else if (HDRleft)
  {
    return Left;
  }

  return None;
}

bool CEnumeratorHD::IsPQ10PassthroughSupported()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (QueryHDRtoHDRSupport() == None)
  {
    CLog::LogF(
        LOGWARNING,
        "Input color space HDR10 is not supported by video processor with HDR {} range output. "
        "DXVA will not be used.",
        DX::Windowing()->UseLimitedColor() ? "limited" : "full");

    return false;
  }

  return true;
}

InputFormat CEnumeratorHD::QueryHDRtoSDRSupport() const
{
  // Not initialized yet
  if (!m_pEnumerator)
    return None;

  if (!m_pEnumerator1)
  {
    CLog::LogF(LOGWARNING,
               "ID3D11VideoProcessorEnumerator1 required to evaluate support is not available.");
    return None;
  }

  const DXGI_FORMAT destFormat = DX::Windowing()->GetBackBuffer().GetFormat();
  const DXGI_COLOR_SPACE_TYPE destColor = DX::Windowing()->UseLimitedColor()
                                              ? DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709
                                              : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

  // Check if BT.2020 (LEFT) input color space is supported by video driver
  const bool left = CheckConversionInternal(
      m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020, destFormat, destColor);

  // Check if BT.2020 (TOP LEFT) input color space is supported by video driver
  const bool topLeft = CheckConversionInternal(
      m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020, destFormat, destColor);

  CLog::LogF(LOGDEBUG,
             "BT.2020 input color spaces supported with SDR {} range output: "
             "YCBCR_STUDIO_G22_LEFT_P2020: {}, "
             "YCBCR_STUDIO_G22_TOPLEFT_P2020: {}",
             DX::Windowing()->UseLimitedColor() ? "limited" : "full", left ? "yes" : "no",
             topLeft ? "yes" : "no");

  if (topLeft)
  {
    if (left)
      CLog::LogF(LOGDEBUG, "Preferred UHD Blu-Ray top left chroma siting will be used");

    return TopLeft;
  }
  else if (left)
  {
    return Left;
  }

  return None;
}

bool CEnumeratorHD::IsBT2020Supported()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (QueryHDRtoSDRSupport() == None)
  {
    CLog::LogF(
        LOGWARNING,
        "Input color space BT.2020 is not supported by video processor with SDR {} range output. "
        "DXVA will not be used.",
        DX::Windowing()->UseLimitedColor() ? "limited" : "full");

    return false;
  }

  return true;
}

bool CEnumeratorHD::QuerySDRSupport() const
{
  // Not initialized yet
  if (!m_pEnumerator)
    return false;

  // Full range SDR output: this has always been standard functionality, assume it's present.
  if (!DX::Windowing()->UseLimitedColor())
    return true;

  if (!m_pEnumerator1)
  {
    CLog::LogF(LOGWARNING,
               "ID3D11VideoProcessorEnumerator1 not available - assuming SDR output is possible.");
    return true;
  }

  const DXGI_FORMAT destFormat = DX::Windowing()->GetBackBuffer().GetFormat();

  // Check if BT.709 input color space is supported by video driver
  return CheckConversionInternal(m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709,
                                 destFormat, DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709);
}

bool CEnumeratorHD::IsSDRSupported()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (!QuerySDRSupport())
  {
    CLog::LogF(
        LOGWARNING,
        "Input color space BT.709 is not supported by video processor with SDR {} range output. "
        "DXVA will not be used.",
        DX::Windowing()->UseLimitedColor() ? "limited" : "full");

    return false;
  }

  return true;
}

ProcessorFormats CEnumeratorHD::GetProcessorFormats(bool inputFormats, bool outputFormats) const
{
  // Not initialized yet
  if (!m_pEnumerator)
    return {};

  ProcessorFormats formats;
  HRESULT hr{};
  UINT uiFlags{0};
  for (int fmt = DXGI_FORMAT_UNKNOWN; fmt <= DXGI_FORMAT_V408; fmt++)
  {
    const DXGI_FORMAT dxgiFormat = static_cast<DXGI_FORMAT>(fmt);
    if (S_OK == (hr = m_pEnumerator->CheckVideoProcessorFormat(dxgiFormat, &uiFlags)))
    {
      if ((uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT) && inputFormats)
        formats.m_input.push_back(dxgiFormat);
      if ((uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT) && outputFormats)
        formats.m_output.push_back(dxgiFormat);
    }
    else
    {
      CLog::LogF(LOGWARNING,
                 "Unable to retrieve support of the dxva processor for format {}, error {}",
                 DX::DXGIFormatToString(dxgiFormat), DX::GetErrorDescription(hr));
      return formats;
    }
  }
  formats.m_valid = true;

  return formats;
}

std::vector<DXGI_FORMAT> CEnumeratorHD::GetProcessorRGBOutputFormats()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  ProcessorFormats formats = GetProcessorFormats(false, true);
  if (!formats.m_valid)
    return {};

  std::vector<DXGI_FORMAT> result;
  result.reserve(formats.m_output.size());
  for (const auto& format : formats.m_output)
  {
    const std::string name = DX::DXGIFormatToString(format);

    if (name.find('R') != std::string::npos && name.find('G') != std::string::npos &&
        name.find('B') != std::string::npos)
    {
      result.emplace_back(format);
    }
  }

  return result;
}

bool CEnumeratorHD::CheckConversion(DXGI_FORMAT inputFormat,
                                    DXGI_COLOR_SPACE_TYPE inputCS,
                                    DXGI_FORMAT outputFormat,
                                    DXGI_COLOR_SPACE_TYPE outputCS)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return CheckConversionInternal(inputFormat, inputCS, outputFormat, outputCS);
}

bool CEnumeratorHD::CheckConversionInternal(DXGI_FORMAT inputFormat,
                                            DXGI_COLOR_SPACE_TYPE inputCS,
                                            DXGI_FORMAT outputFormat,
                                            DXGI_COLOR_SPACE_TYPE outputCS) const
{
  // Not initialized yet or OS < Windows 10
  if (!m_pEnumerator || !m_pEnumerator1)
    return false;

  HRESULT hr;
  BOOL supported;

  if (SUCCEEDED(hr = m_pEnumerator1->CheckVideoProcessorFormatConversion(
                    inputFormat, inputCS, outputFormat, outputCS, &supported)))
  {
    return (supported == TRUE);
  }
  else
  {
    CLog::LogF(LOGERROR, "unable to validate the format conversion, error {}",
               DX::GetErrorDescription(hr));
    return false;
  }
}

ProcessorConversions CEnumeratorHD::ListConversions(
    DXGI_FORMAT inputFormat,
    const std::vector<DXGI_COLOR_SPACE_TYPE>& inputColorSpaces,
    const std::vector<DXGI_FORMAT>& outputFormats,
    const std::vector<DXGI_COLOR_SPACE_TYPE>& outputColorSpaces)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  // Not initialized yet, or under Windows 10
  if (!m_pEnumerator || !m_pEnumerator1)
    return {};

  ProcessorConversions result;

  for (const DXGI_COLOR_SPACE_TYPE& inputCS : inputColorSpaces)
  {
    for (const DXGI_FORMAT& outputFormat : outputFormats)
    {
      for (const DXGI_COLOR_SPACE_TYPE& outputCS : outputColorSpaces)
      {
        if (CheckConversionInternal(inputFormat, inputCS, outputFormat, outputCS))
        {
          result.push_back(
              ProcessorConversion{m_input_dxgi_format, inputCS, outputFormat, outputCS});
        }
      }
    }
  }
  return result;
}

void CEnumeratorHD::LogSupportedConversions(const DXGI_FORMAT& inputFormat,
                                            const DXGI_COLOR_SPACE_TYPE heuristicsInputCS,
                                            const DXGI_COLOR_SPACE_TYPE inputNativeCS,
                                            const DXGI_FORMAT& outputFormat,
                                            const DXGI_COLOR_SPACE_TYPE heuristicsOutputCS)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  if (!m_pEnumerator)
    return;

  // Windows 8 and above compatible code

  HRESULT hr;
  UINT uiFlags;

  if (FAILED(hr = m_pEnumerator->CheckVideoProcessorFormat(inputFormat, &uiFlags)))
  {
    CLog::LogFC(LOGDEBUG, LOGVIDEO,
                "unable to retrieve processor support of input format {}. Error {}",
                DX::DXGIFormatToString(inputFormat), DX::GetErrorDescription(hr));
    return;
  }
  else if (!(uiFlags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT))
  {
    CLog::LogFC(LOGDEBUG, LOGVIDEO,
                "input format {} not supported by the processor. No conversion possible.",
                DX::DXGIFormatToString(inputFormat));
    return;
  }

  // Windows 10 and above from this point on
  if (!m_pEnumerator1)
    return;

  CLog::LogFC(LOGDEBUG, LOGVIDEO, "The source is {} / {}", DX::DXGIFormatToString(inputFormat),
              DX::DXGIColorSpaceTypeToString(inputNativeCS));

  const bool supported =
      CheckConversionInternal(inputFormat, inputNativeCS, outputFormat, heuristicsOutputCS);

  CLog::LogFC(LOGDEBUG, LOGVIDEO, "conversion from {} / {} to {} / {} is {}supported.",
              DX::DXGIFormatToString(inputFormat), DX::DXGIColorSpaceTypeToString(inputNativeCS),
              DX::DXGIFormatToString(outputFormat),
              DX::DXGIColorSpaceTypeToString(heuristicsOutputCS), supported ? "" : "NOT ");

  // Possible input color spaces: YCbCr only
  std::vector<DXGI_COLOR_SPACE_TYPE> ycbcrColorSpaces;
  // Possible output color spaces: RGB only
  std::vector<DXGI_COLOR_SPACE_TYPE> rgbColorSpaces;

  for (UINT colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
       colorSpace < DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020; ++colorSpace)
  {
    const DXGI_COLOR_SPACE_TYPE cs = static_cast<DXGI_COLOR_SPACE_TYPE>(colorSpace);

    constexpr std::string_view rgb("RGB_");
    if (DX::DXGIColorSpaceTypeToString(cs).compare(0, rgb.size(), rgb) == 0)
      rgbColorSpaces.push_back(cs);

    constexpr std::string_view ycbcr("YCBCR_");
    if (DX::DXGIColorSpaceTypeToString(cs).compare(0, ycbcr.size(), ycbcr) == 0)
      ycbcrColorSpaces.push_back(cs);
  }

  // Only probe the output formats of RGB/BGR type supported by the processor.
  const std::vector<DXGI_FORMAT> outputFormats = GetProcessorRGBOutputFormats();

  // Color spaces supported directly by the swap chain - as a set for easy lookup
  const std::vector<DXGI_COLOR_SPACE_TYPE> bbcs =
      DX::DeviceResources::Get()->GetSwapChainColorSpaces();
  const std::set<DXGI_COLOR_SPACE_TYPE> backbufferColorSpaces(bbcs.begin(), bbcs.end());

  // The input format cannot be worked around and is fixed.
  // Restrieve all combinations of the allowed
  // - input color spaces
  // - output formats
  // - output color spaces

  const ProcessorConversions conversions =
      ListConversions(inputFormat, ycbcrColorSpaces, outputFormats, rgbColorSpaces);

  std::string conversionsString;

  for (const ProcessorConversion& c : conversions)
  {
    conversionsString.append("\n");
    conversionsString.append(StringUtils::Format(
        "{} {} / {}{} {:<{}} to {} {:<{}} / {}{} {:<{}}", "*",
        DX::DXGIFormatToString(c.m_inputFormat), (c.m_inputCS == heuristicsInputCS) ? "*" : " ",
        (c.m_inputCS == inputNativeCS) ? "N" : " ", DX::DXGIColorSpaceTypeToString(c.m_inputCS), 32,
        (c.m_outputFormat == outputFormat) ? "*" : " ", DX::DXGIFormatToString(c.m_outputFormat),
        26, (c.m_outputCS == heuristicsOutputCS) ? "*" : " ",
        (backbufferColorSpaces.find(c.m_outputCS) != backbufferColorSpaces.end()) ? "bb" : "  ",
        DX::DXGIColorSpaceTypeToString(c.m_outputCS), 32));
  }

  CLog::LogFC(LOGDEBUG, LOGVIDEO,
              "Supported conversions from format {}\n(*: values picked by "
              "heuristics, N native input color space, bb supported as swap chain backbuffer){}",
              DX::DXGIFormatToString(inputFormat), conversionsString);
}
