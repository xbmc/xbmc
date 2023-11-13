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

struct ProcColorSpaces;

class CProcessorHD : public ID3DResource
{
public:
  explicit CProcessorHD();
 ~CProcessorHD();

  void UnInit();
  bool Open(const VideoPicture& picture, std::shared_ptr<DXVA::CEnumeratorHD> enumerator);
  void Close();
  bool Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer **views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness);
  uint8_t PastRefs() const { return std::min(m_procCaps.m_rateCaps.PastFrames, 4u); }

  /*!
   * \brief Configure the processor for the provided conversion.
   * \param conversion the conversion
   * \return success status, true = success, false = error
   */
  bool SetConversion(const ProcessorConversion& conversion);

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
  bool Supports(ERENDERFEATURE feature) const;

protected:
  bool ReInit();
  bool InitProcessor();
  bool CheckFormats() const;
  bool OpenProcessor();
  void ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const;
  ComPtr<ID3D11VideoProcessorInputView> GetInputView(CRenderBuffer* view) const;
  /*!
   * \brief Apply new video settings if there was a change. Returns true if a parameter changed, false otherwise.
   */
  bool CheckVideoParameters(const CRect& src,
                            const CRect& dst,
                            const UINT& rotation,
                            const float& contrast,
                            const float& brightness,
                            const CRenderBuffer& rb);

  void EnableIntelVideoSuperResolution();
  void EnableNvidiaRTXVideoSuperResolution();

  CCriticalSection m_section;

  ComPtr<ID3D11VideoDevice> m_pVideoDevice;
  ComPtr<ID3D11VideoContext> m_pVideoContext;
  ComPtr<ID3D11VideoProcessor> m_pVideoProcessor;
  std::shared_ptr<CEnumeratorHD> m_enumerator;

  AVColorPrimaries m_color_primaries{AVCOL_PRI_UNSPECIFIED};
  AVColorTransferCharacteristic m_color_transfer{AVCOL_TRC_UNSPECIFIED};
  ProcessorCapabilities m_procCaps;

  bool m_superResolutionEnabled{false};
  ProcessorConversion m_conversion;
  bool m_isValidConversion{false};

  /*!
   * \brief true when at least one frame has been processed successfully since init
   */
  bool m_configured{false};

  // Members to compare the current frame with the previous frame
  UINT m_lastInputFrameOrField{0};
  UINT m_lastOutputIndex{0};
  CRect m_lastSrc{};
  CRect m_lastDst{};
  UINT m_lastRotation{0};
  float m_lastContrast{.0f};
  float m_lastBrightness{.0f};
  ProcessorConversion m_lastConversion{};
  AVColorSpace m_lastColorSpace{AVCOL_SPC_UNSPECIFIED};
  bool m_lastFullRange{false};
};
};
