/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDCodecs/Video/DXVA.h"
#include "guilib/D3DResource.h"
#include "utils/Geometry.h"

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

const DWORD NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

class CProcessorHD : public ID3DResource
{
public:
  CProcessorHD();
 ~CProcessorHD();

  bool PreInit();
  void UnInit();
  bool Open(UINT width, UINT height);
  void Close();
  bool Render(CRect src, CRect dst, ID3D11Resource* target, CRenderBuffer **views, DWORD flags, UINT frameIdx, UINT rotation, float contrast, float brightness);
  uint8_t Size() const { return m_pVideoProcessor ? m_size : 0; }
  uint8_t PastRefs() const { return m_max_back_refs; }

  // ID3DResource overrides
  void OnCreateDevice() override  {}
  void OnDestroyDevice(bool fatal) override { CSingleLock lock(m_section); UnInit(); }

protected:
  bool ReInit();
  bool InitProcessor();
  bool CheckFormats() const;
  bool OpenProcessor();
  bool ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const;
  ID3D11VideoProcessorInputView* GetInputView(CRenderBuffer* view) const;
  bool IsFormatSupported(DXGI_FORMAT format, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT support) const;

  CCriticalSection m_section;

  uint32_t m_width;
  uint32_t m_height;
  uint8_t  m_size;
  uint8_t  m_max_back_refs;
  uint8_t  m_max_fwd_refs;
  uint32_t m_procIndex;
  D3D11_VIDEO_PROCESSOR_CAPS m_vcaps;
  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS m_rateCaps;

  struct ProcAmpInfo
  {
    bool bSupported;
    D3D11_VIDEO_PROCESSOR_FILTER_RANGE Range;
  };
  ProcAmpInfo m_Filters[NUM_FILTERS];
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_pVideoDevice;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_pVideoContext;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> m_pEnumerator;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessor> m_pVideoProcessor;
};

};
