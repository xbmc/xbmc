/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <vector>
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDCodecs/Video/DXVA.h"
#include "guilib/D3DResource.h"
#include "guilib/Geometry.h"

namespace DXVA {

// ProcAmp filters d3d11 filters
const D3D11_VIDEO_PROCESSOR_FILTER PROCAMP_FILTERS[] =
{
  D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS,
  D3D11_VIDEO_PROCESSOR_FILTER_CONTRAST,
  D3D11_VIDEO_PROCESSOR_FILTER_HUE,
  D3D11_VIDEO_PROCESSOR_FILTER_SATURATION
};

enum PROCESSOR_VIEW_TYPE
{
  PROCESSOR_VIEW_TYPE_UNKNOWN = 0,
  PROCESSOR_VIEW_TYPE_DECODER = 1,
  PROCESSOR_VIEW_TYPE_PROCESSOR = 2
};

const DWORD NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

class CProcessorHD : public ID3DResource
{
public:
  CProcessorHD();
 ~CProcessorHD();

  bool PreInit();
  void UnInit();
  bool Open(UINT width, UINT height, unsigned int flags, unsigned int format, DXGI_FORMAT dxva_format);
  void Close();
  CRenderPicture *Convert(const VideoPicture &picture) const;
  bool Render(CRect src, CRect dst, ID3D11Resource* target, ID3D11View **views, DWORD flags, UINT frameIdx, UINT rotation);
  uint8_t Size() const { if (m_pVideoProcessor) return m_size; return 0; }
  uint8_t PastRefs() const { return m_max_back_refs; }

  // ID3DResource overrides
  void OnCreateDevice() override  {}
  void OnDestroyDevice(bool fatal) override { CSingleLock lock(m_section); UnInit(); }
  void OnLostDevice() override    { CSingleLock lock(m_section); UnInit(); }
  void OnResetDevice() override   { CSingleLock lock(m_section); Close();  }

protected:
  bool ReInit();
  bool InitProcessor();
  bool ConfigureProcessor(unsigned int format, DXGI_FORMAT dxva_format);
  bool OpenProcessor();
  bool CreateSurfaces();
  bool ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def) const;
  ID3D11VideoProcessorInputView* GetInputView(ID3D11View* view) const;
  bool IsFormatSupported(DXGI_FORMAT format, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT support) const;

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_flags = 0;
  uint32_t m_renderFormat = 0;
  uint8_t  m_size = 0;
  uint8_t  m_max_back_refs = 0;
  uint8_t  m_max_fwd_refs = 0;

  struct ProcAmpInfo
  {
    bool bSupported;
    D3D11_VIDEO_PROCESSOR_FILTER_RANGE  Range;
  };
  ProcAmpInfo m_Filters[NUM_FILTERS];

  DXGI_FORMAT m_textureFormat;
  ID3D11VideoDevice *m_pVideoDevice;
  ID3D11VideoContext *m_pVideoContext;
  ID3D11VideoProcessorEnumerator *m_pEnumerator;
  D3D11_VIDEO_PROCESSOR_CAPS m_vcaps;
  ID3D11VideoProcessor *m_pVideoProcessor;
  CSurfaceContext *m_context;
  CCriticalSection m_section;

  uint32_t m_procIndex = 0;
  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS m_rateCaps;
  D3D11_TEXTURE2D_DESC m_texDesc;
  PROCESSOR_VIEW_TYPE m_eViewType;
};

};
