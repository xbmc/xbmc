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
#include "guilib/D3DResource.h"
#include "utils/Geometry.h"

#include <dxgi1_5.h>
#include <wrl/client.h>

class CRenderBuffer;

namespace DXVA {

// ProcAmp filters d3d11 filters
const D3D11_VIDEO_PROCESSOR_FILTER PROCAMP_FILTERS[] =
{
  D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS,
  D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST,
  D3D11_VIDEO_PROCESSOR_FILTER_HUE,
  D3D11_VIDEO_PROCESSOR_FILTER_SATURATION
};

const size_t NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

class CProcessorHD : public ID3DResource
{
public:
  explicit CProcessorHD();
 ~CProcessorHD();

  bool PreInit() const;
  void UnInit();
  bool Open(UINT width, UINT height);
  void Close();
  bool Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer **views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness);
  uint8_t PastRefs() const { return m_max_back_refs; }
  bool IsFormatSupported(DXGI_FORMAT format, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT support) const;
  bool HasHDR10Support() const { return m_bSupportHDR10; }

  // ID3DResource overrides
  void OnCreateDevice() override  {}
  void OnDestroyDevice(bool) override { CSingleLock lock(m_section); UnInit(); }

  static DXGI_COLOR_SPACE_TYPE GetDXGIColorSpace(CRenderBuffer*, bool);

protected:
  bool ReInit();
  bool InitProcessor();
  bool CheckFormats() const;
  bool OpenProcessor();
  void ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const;
  ID3D11VideoProcessorInputView* GetInputView(CRenderBuffer* view) const;

  CCriticalSection m_section;

  uint32_t m_width = 0;
  uint32_t m_height = 0;
  uint8_t  m_max_back_refs = 0;
  uint8_t  m_max_fwd_refs = 0;
  uint32_t m_procIndex = 0;
  D3D11_VIDEO_PROCESSOR_CAPS m_vcaps = {};
  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS m_rateCaps = {};
  bool m_bSupportHDR10 = false;
  DXGI_HDR_METADATA_HDR10 m_hdr10Display = {};

  struct ProcAmpInfo
  {
    bool bSupported;
    D3D11_VIDEO_PROCESSOR_FILTER_RANGE Range;
  };
  ProcAmpInfo m_Filters[NUM_FILTERS]{};
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_pVideoDevice;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_pVideoContext;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> m_pEnumerator;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessor> m_pVideoProcessor;
};

};
