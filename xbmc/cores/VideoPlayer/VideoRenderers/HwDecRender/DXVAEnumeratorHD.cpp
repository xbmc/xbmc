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
    BOOL supported = 0;
    HRESULT hr;

    // Check if HLG color space conversion is supported by driver
    hr = m_pEnumerator1->CheckVideoProcessorFormatConversion(
        DXGI_FORMAT_P010, DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020, format,
        DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709, &supported);
    result.m_bSupportHLG = SUCCEEDED(hr) && !!supported;

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
  BOOL supported = 0;

  // Check if HDR10 (TOP LEFT) input color space is supported by video driver
  HRESULT hr = m_pEnumerator1->CheckVideoProcessorFormatConversion(
      m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020,
      DXGI_FORMAT_R10G10B10A2_UNORM, destColor, &supported);
  const bool HDRtopLeft = SUCCEEDED(hr) && static_cast<bool>(supported);

  // Check if HDR10 (LEFT) input color space is supported by video driver
  hr = m_pEnumerator1->CheckVideoProcessorFormatConversion(
      m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020,
      DXGI_FORMAT_R10G10B10A2_UNORM, destColor, &supported);
  const bool HDRleft = SUCCEEDED(hr) && static_cast<bool>(supported);

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
  BOOL supported = 0;

  // Check if BT.2020 (LEFT) input color space is supported by video driver
  HRESULT hr = m_pEnumerator1->CheckVideoProcessorFormatConversion(
      m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020, destFormat, destColor,
      &supported);
  const bool left = SUCCEEDED(hr) && static_cast<bool>(supported);

  // Check if BT.2020 (TOP LEFT) input color space is supported by video driver
  hr = m_pEnumerator1->CheckVideoProcessorFormatConversion(
      m_input_dxgi_format, DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020, destFormat, destColor,
      &supported);
  const bool topLeft = SUCCEEDED(hr) && static_cast<bool>(supported);

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
