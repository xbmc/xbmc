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

struct ProcessorFormats
{
  std::vector<DXGI_FORMAT> m_input;
  std::vector<DXGI_FORMAT> m_output;
  bool m_valid{false};
};

struct ProcessorConversion
{
  DXGI_FORMAT m_inputFormat{DXGI_FORMAT_UNKNOWN};
  DXGI_COLOR_SPACE_TYPE m_inputCS{DXGI_COLOR_SPACE_RESERVED};
  DXGI_FORMAT m_outputFormat{DXGI_FORMAT_UNKNOWN};
  DXGI_COLOR_SPACE_TYPE m_outputCS{DXGI_COLOR_SPACE_RESERVED};
};

using ProcessorConversions = std::vector<ProcessorConversion>;

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
  /*!
   * \brief Test whether the dxva video processor supports SDR to SDR conversion.
   * Support is assumed to exist on systems that don't support the
   * ID3D11VideoProcessorEnumerator1 interface.
   * \return conversion supported yes/no
  */
  bool IsSDRSupported();
  ProcessorCapabilities ProbeProcessorCaps();
  /*!
   * \brief Check if a conversion is supported by the dxva processor.
   * \param inputFormat the input format
   * \param inputCS the input color space
   * \param outputFormat the output format
   * \param outputCS the output color space
   * \return true when the conversion is supported, false when it is not
   * or the API used to validate is not availe (Windows < 10)
  */
  bool CheckConversion(DXGI_FORMAT inputFormat,
                       DXGI_COLOR_SPACE_TYPE inputCS,
                       DXGI_FORMAT outputFormat,
                       DXGI_COLOR_SPACE_TYPE outputCS);
  /*!
   * \brief Outputs in the log a list of conversions supported by the DXVA processor.
   * \param inputFormat the source format
   * \param heuristicsInputCS the input color space that will be used for playback
   * \param inputNativeCS the input color space that would be used with a direct mapping
   * from avcodec to D3D11, without any workarounds or tricks.
   * \param outputFormat the destination format
   * \param heuristicsOutputCS the output color space that will be used for playback
   */
  void LogSupportedConversions(const DXGI_FORMAT& inputFormat,
                               const DXGI_COLOR_SPACE_TYPE heuristicsInputCS,
                               const DXGI_COLOR_SPACE_TYPE inputNativeCS,
                               const DXGI_FORMAT& outputFormat,
                               const DXGI_COLOR_SPACE_TYPE heuristicsOutputCS);

  ComPtr<ID3D11VideoProcessorEnumerator> Get() { return m_pEnumerator; }
  ComPtr<ID3D11VideoProcessorEnumerator1> Get1() { return m_pEnumerator1; }

protected:
  void UnInit();
  InputFormat QueryHDRtoHDRSupport() const;
  InputFormat QueryHDRtoSDRSupport() const;
  bool QuerySDRSupport() const;
  /*!
   * \brief Retrieve the list of DXGI_FORMAT supported by the DXVA processor
   * \param inputFormats yes/no populate the input formats vector of the returned structure
   * \param outputFormats yes/no populate the output formats vector of the returned structure
   * \return requested list of input and/or output formats.
   */
  ProcessorFormats GetProcessorFormats(bool inputFormats, bool outputFormats) const;
  /*!
   * \brief Retrieve the list of RGB DXGI_FORMAT supported as output by the DXVA
   * processor \return Vector of formats
   */
  std::vector<DXGI_FORMAT> GetProcessorRGBOutputFormats() const;
  /*!
   * \brief Check if a conversion is supported by the dxva processor.
   * \param inputFormat the input format
   * \param inputCS the input color space
   * \param outputFormat the output format
   * \param outputCS the output color space
   * \return true when the conversion is supported, false when it is not
   * or the API used to validate is not available (Windows < 10)
  */
  bool CheckConversionInternal(DXGI_FORMAT inputFormat,
                               DXGI_COLOR_SPACE_TYPE inputCS,
                               DXGI_FORMAT outputFormat,
                               DXGI_COLOR_SPACE_TYPE outputCS) const;
  /*!
   * \brief Iterate over all combinations of the input parameters and return a list of
   * the combinations that are supported conversions.
   * \param inputFormat The input format
   * \param inputColorSpaces The possible source color spaces
   * \param outputFormats The possible output formats
   * \param outputColorSpaces The possible output color spaces
   * \return List of the supported conversion.
   */
  ProcessorConversions ListConversions(
      DXGI_FORMAT inputFormat,
      const std::vector<DXGI_COLOR_SPACE_TYPE>& inputColorSpaces,
      const std::vector<DXGI_FORMAT>& outputFormats,
      const std::vector<DXGI_COLOR_SPACE_TYPE>& outputColorSpaces) const;

  CCriticalSection m_section;

  ComPtr<ID3D11VideoProcessorEnumerator> m_pEnumerator;
  ComPtr<ID3D11VideoProcessorEnumerator1> m_pEnumerator1;
  DXGI_FORMAT m_input_dxgi_format{DXGI_FORMAT_UNKNOWN};
};
}; // namespace DXVA
