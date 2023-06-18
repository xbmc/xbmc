/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoRenderers/Windows/RendererBase.h"
#include "guilib/D3DResource.h"

#include <mutex>

#include <wrl/client.h>

namespace DXVA
{

using namespace Microsoft::WRL;

// ProcAmp filters d3d11 filters
struct ProcAmpFilter
{
  D3D11_VIDEO_PROCESSOR_FILTER filter;
  D3D11_VIDEO_PROCESSOR_FILTER_CAPS cap;
  const char* name;
};

// clang-format off
const ProcAmpFilter PROCAMP_FILTERS[] = {
    {D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS,
    D3D11_VIDEO_PROCESSOR_FILTER_CAPS_BRIGHTNESS, "Brightness"},
    {D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST,
    D3D11_VIDEO_PROCESSOR_FILTER_CAPS_CONTRAST, "Contrast"},
    {D3D11_VIDEO_PROCESSOR_FILTER_HUE,
    D3D11_VIDEO_PROCESSOR_FILTER_CAPS_HUE, "Hue"},
    {D3D11_VIDEO_PROCESSOR_FILTER_SATURATION,
    D3D11_VIDEO_PROCESSOR_FILTER_CAPS_SATURATION, "Saturation"},
    {D3D11_VIDEO_PROCESSOR_FILTER_NOISE_REDUCTION,
    D3D11_VIDEO_PROCESSOR_FILTER_CAPS_NOISE_REDUCTION, "Noise Reduction"},
    {D3D11_VIDEO_PROCESSOR_FILTER_EDGE_ENHANCEMENT,
     D3D11_VIDEO_PROCESSOR_FILTER_CAPS_EDGE_ENHANCEMENT, "Edge Enhancement"},
    {D3D11_VIDEO_PROCESSOR_FILTER_ANAMORPHIC_SCALING,
     D3D11_VIDEO_PROCESSOR_FILTER_CAPS_ANAMORPHIC_SCALING, "Anamorphic Scaling"},
    {D3D11_VIDEO_PROCESSOR_FILTER_STEREO_ADJUSTMENT,
     D3D11_VIDEO_PROCESSOR_FILTER_CAPS_STEREO_ADJUSTMENT, "Stereo Adjustment"}};
// clang-format on

constexpr size_t NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

struct ProcAmpInfo
{
  bool bSupported;
  D3D11_VIDEO_PROCESSOR_FILTER_RANGE Range;
};

struct ProcessorCapabilities
{
  bool m_valid{false};

  uint32_t m_procIndex{0};
  D3D11_VIDEO_PROCESSOR_CAPS m_vcaps{};
  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS m_rateCaps{};
  ProcAmpInfo m_Filters[NUM_FILTERS]{};
  bool m_bSupportHLG{false};
  bool m_HDR10Left{false};
  bool m_BT2020Left{false};
  bool m_hasMetadataHDR10Support{false};
};

enum InputFormat
{
  None,
  TopLeft,
  Left
};

class CEnumeratorHD : public ID3DResource
{
public:
  CEnumeratorHD();
  virtual ~CEnumeratorHD();

  bool Open(unsigned int width, unsigned int height, DXGI_FORMAT input_dxgi_format);
  void Close();

  // ID3DResource overrides
  void OnCreateDevice() override {}
  void OnDestroyDevice(bool) override
  {
    std::unique_lock<CCriticalSection> lock(m_section);
    UnInit();
  }

  bool IsBT2020Supported();
  bool IsPQ10PassthroughSupported();
  ProcessorCapabilities ProbeProcessorCaps();

  ComPtr<ID3D11VideoProcessorEnumerator> Get() { return m_pEnumerator; }
  ComPtr<ID3D11VideoProcessorEnumerator1> Get1() { return m_pEnumerator1; }

protected:
  void UnInit();
  InputFormat QueryHDRtoHDRSupport() const;
  InputFormat QueryHDRtoSDRSupport() const;

  CCriticalSection m_section;

  ComPtr<ID3D11VideoProcessorEnumerator> m_pEnumerator;
  ComPtr<ID3D11VideoProcessorEnumerator1> m_pEnumerator1;
  DXGI_FORMAT m_input_dxgi_format{DXGI_FORMAT_UNKNOWN};
};
}; // namespace DXVA
