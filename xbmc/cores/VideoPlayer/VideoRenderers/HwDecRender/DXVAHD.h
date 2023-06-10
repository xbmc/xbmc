/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDCodecs/Video/DXVA.h"
#include "VideoRenderers/Windows/RendererBase.h"
#include "guilib/D3DResource.h"
#include "utils/Geometry.h"

#include <mutex>

#include <wrl/client.h>

class CRenderBuffer;

namespace DXVA {

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

const size_t NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

struct DXGIColorSpaceArgs
{
  AVColorPrimaries primaries = AVCOL_PRI_UNSPECIFIED;
  AVColorSpace color_space = AVCOL_SPC_UNSPECIFIED;
  AVColorTransferCharacteristic color_transfer = AVCOL_TRC_UNSPECIFIED;
  bool full_range = false;

  DXGIColorSpaceArgs(const CRenderBuffer& buf)
  {
    primaries = buf.primaries;
    color_space = buf.color_space;
    color_transfer = buf.color_transfer;
    full_range = buf.full_range;
  }
  DXGIColorSpaceArgs(const VideoPicture& picture)
  {
    primaries = static_cast<AVColorPrimaries>(picture.color_primaries);
    color_space = static_cast<AVColorSpace>(picture.color_space);
    color_transfer = static_cast<AVColorTransferCharacteristic>(picture.color_transfer);
    full_range = picture.color_range == 1;
  }
};

struct ProcColorSpaces;

class CProcessorHD : public ID3DResource
{
public:
  explicit CProcessorHD();
 ~CProcessorHD();

  bool PreInit() const;
  void UnInit();
  bool Open(UINT width, UINT height, const VideoPicture& picture);
  void Close();
  bool Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer **views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness);
  uint8_t PastRefs() const { return m_max_back_refs; }
  bool IsFormatSupported(DXGI_FORMAT format, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT support) const;
  /*!
   * \brief Evaluate if the DXVA processor supports converting between two formats and color spaces.
   * Always returns true when the Windows 10+ API is not available or cannot be called successfully.
   * \param inputFormat The source format
   * \param outputFormat The destination format
   * \param picture Picture information used to derive the color spaces
   * \return true if the conversion is supported, false otherwise
   */
  bool IsFormatConversionSupported(DXGI_FORMAT inputFormat,
                                   DXGI_FORMAT outputFormat,
                                   const VideoPicture& picture);
  /*!
   * \brief Outputs in the log a list of conversions supported by the DXVA processor.
   * \param inputFormat The source format
   * \param outputFormat The destination format
   * \param picture Picture information used to derive the color spaces
   */
  void ListSupportedConversions(const DXGI_FORMAT& inputFormat,
                                const DXGI_FORMAT& outputFormat,
                                const VideoPicture& picture);

  // ID3DResource overrides
  void OnCreateDevice() override  {}
  void OnDestroyDevice(bool) override
  {
    std::unique_lock<CCriticalSection> lock(m_section);
    UnInit();
  }

  static bool IsBT2020Supported();
  static bool IsPQ10PassthroughSupported(const DXGI_FORMAT dxgi_format);
  static bool IsSuperResolutionSuitable(const VideoPicture& picture);
  void TryEnableVideoSuperResolution();
  bool IsVideoSuperResolutionEnabled() const { return m_superResolutionEnabled; }

protected:
  bool ReInit();
  bool InitProcessor();
  bool CheckFormats() const;
  bool OpenProcessor();
  void ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const;
  ID3D11VideoProcessorInputView* GetInputView(CRenderBuffer* view) const;
  /*!
   * \brief Calculate the color spaces of the input and output of the processor
   * \param csArgs Arguments for the calculations
   * \return the input and output color spaces
   */
  ProcColorSpaces CalculateDXGIColorSpaces(const DXGIColorSpaceArgs& csArgs) const;
  static DXGI_COLOR_SPACE_TYPE GetDXGIColorSpaceSource(const DXGIColorSpaceArgs& csArgs,
                                                       bool supportHDR,
                                                       bool supportHLG,
                                                       bool BT2020Left,
                                                       bool HDRLeft);
  static DXGI_COLOR_SPACE_TYPE GetDXGIColorSpaceTarget(const DXGIColorSpaceArgs& csArgs,
                                                       bool supportHDR,
                                                       bool limitedRange);
  /*!
   * \brief Converts ffmpeg AV parameters to a DXGI color space
   * \param csArgs ffmpeg AV picture parameters
   * \return DXGI color space
   */
  static DXGI_COLOR_SPACE_TYPE AvToDxgiColorSpace(const DXGIColorSpaceArgs& csArgs);
  /*!
   * \brief Retrieve the list of DXGI_FORMAT supported as output by the DXVA processor
   * \return Vector of formats
   */
  std::vector<DXGI_FORMAT> GetProcessorOutputFormats() const;

  void EnableIntelVideoSuperResolution();
  void EnableNvidiaRTXVideoSuperResolution();

  CCriticalSection m_section;

  uint32_t m_width = 0;
  uint32_t m_height = 0;
  uint8_t  m_max_back_refs = 0;
  uint8_t  m_max_fwd_refs = 0;
  uint32_t m_procIndex = 0;
  D3D11_VIDEO_PROCESSOR_CAPS m_vcaps = {};
  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS m_rateCaps = {};
  bool m_bSupportHLG = false;
  bool m_HDR10Left{false};
  bool m_BT2020Left{false};
  bool m_hasMetadataHDR10Support{false};

  struct ProcAmpInfo
  {
    bool bSupported;
    D3D11_VIDEO_PROCESSOR_FILTER_RANGE Range;
  };
  ProcAmpInfo m_Filters[NUM_FILTERS]{};
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_pVideoDevice;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_pVideoContext;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> m_pEnumerator;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator1> m_pEnumerator1;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessor> m_pVideoProcessor;

  AVColorPrimaries m_color_primaries{AVCOL_PRI_UNSPECIFIED};
  AVColorTransferCharacteristic m_color_transfer{AVCOL_TRC_UNSPECIFIED};

  bool m_forced8bit{false};
  bool m_superResolutionEnabled{false};
};

};
