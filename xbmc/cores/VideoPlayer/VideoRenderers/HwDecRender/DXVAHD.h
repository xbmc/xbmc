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
#include "VideoRenderers/HwDecRender/DXVAEnumeratorHD.h"
#include "VideoRenderers/Windows/RendererBase.h"
#include "guilib/D3DResource.h"
#include "utils/Geometry.h"

#include <mutex>

#include <wrl/client.h>

class CRenderBuffer;

namespace DXVA {

using namespace Microsoft::WRL;

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
  bool Open(const VideoPicture& picture);
  void Close();
  bool Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer **views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness);
  uint8_t PastRefs() const { return std::min(m_procCaps.m_rateCaps.PastFrames, 4u); }
  /*!
   * \brief Check support of the format as input texture
   * \param format the format
   * \return true supported, false not supported
   */
  bool IsFormatSupportedInput(DXGI_FORMAT format);
  /*!
   * \brief Check support of the format as input texture
   * \param format the format
   * \return true supported, false not supported
   */
  bool IsFormatSupportedOutput(DXGI_FORMAT format);
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
  void LogSupportedConversions(const DXGI_FORMAT& inputFormat,
                               const DXGI_FORMAT& outputFormat,
                               const VideoPicture& picture);

  /*!
   * \brief Set the output format of the dxva processor. Format compatibility will be verified.
   * \param format The output format
   * \return true when the processor supports the format as output and the format is changed.
   */
  bool SetOutputFormat(DXGI_FORMAT format);
  /*!
   * \brief Return a list of conversions supported by the processor.
   * \param isHdrOutput will the output be HDR yes/no
   * \return list of conversions
   */
  ProcessorConversions SupportedConversions(bool isHdrOutput);

  // ID3DResource overrides
  void OnCreateDevice() override  {}
  void OnDestroyDevice(bool) override
  {
    std::unique_lock<CCriticalSection> lock(m_section);
    UnInit();
  }

  static bool IsSuperResolutionSuitable(const VideoPicture& picture);
  void TryEnableVideoSuperResolution();
  bool IsVideoSuperResolutionEnabled() const { return m_superResolutionEnabled; }

protected:
  bool ReInit();
  bool InitProcessor();
  bool CheckFormats() const;
  bool OpenProcessor();
  void ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const;
  ComPtr<ID3D11VideoProcessorInputView> GetInputView(CRenderBuffer* view) const;
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

  void EnableIntelVideoSuperResolution();
  void EnableNvidiaRTXVideoSuperResolution();

  CCriticalSection m_section;

  uint32_t m_width = 0;
  uint32_t m_height = 0;

  ComPtr<ID3D11VideoDevice> m_pVideoDevice;
  ComPtr<ID3D11VideoContext> m_pVideoContext;
  ComPtr<ID3D11VideoProcessor> m_pVideoProcessor;
  std::unique_ptr<CEnumeratorHD> m_enumerator;

  AVColorPrimaries m_color_primaries{AVCOL_PRI_UNSPECIFIED};
  AVColorTransferCharacteristic m_color_transfer{AVCOL_TRC_UNSPECIFIED};
  bool m_full_range{false};
  ProcessorCapabilities m_procCaps;
  DXGI_FORMAT m_input_dxgi_format{DXGI_FORMAT_UNKNOWN};
  DXGI_FORMAT m_output_dxgi_format{DXGI_FORMAT_UNKNOWN};

  bool m_superResolutionEnabled{false};
};
};
