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

#include <dxva2api.h>
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

const DWORD NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

class CProcessorHD : ID3DResource
  //: public CProcessor
{
public:
  CProcessorHD();
 ~CProcessorHD();

  virtual bool           PreInit();
  virtual void           UnInit();
  virtual bool           Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format);
  virtual void           Close();
  virtual CRenderPicture *Convert(DVDVideoPicture* picture);
  virtual bool           Render(CRect src, CRect dst, ID3D11Resource* target, ID3D11View **views, DWORD flags, UINT frameIdx, UINT rotation);
  virtual unsigned       Size() { if (m_pVideoProcessor) return m_size; return 0; }
  virtual unsigned       PastRefs() { return m_max_back_refs; }
  virtual void           ApplySupportedFormats(std::vector<ERenderFormat> * formats);

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); UnInit(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); UnInit(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); Close(); }

protected:
  virtual bool UpdateSize(const DXVA2_VideoDesc& dsc);
  virtual bool ReInit();
  virtual bool InitProcessor();
  virtual bool CreateSurfaces();
  virtual bool OpenProcessor();
  virtual bool ApplyFilter(D3D11_VIDEO_PROCESSOR_FILTER filter, int value, int min, int max, int def);
  ID3D11VideoProcessorInputView* GetInputView(ID3D11View* view);

  unsigned int             m_width;
  unsigned int             m_height;
  unsigned int             m_flags;
  unsigned int             m_renderFormat;
  unsigned                 m_size;
  unsigned                 m_max_back_refs;
  unsigned                 m_max_fwd_refs;

  struct ProcAmpInfo
  {
    bool                                bSupported;
    D3D11_VIDEO_PROCESSOR_FILTER_RANGE  Range;
  };
  ProcAmpInfo              m_Filters[NUM_FILTERS];

  // dx 11
  DXGI_FORMAT                     m_textureFormat;
  ID3D11VideoDevice*              m_pVideoDevice;
  ID3D11VideoContext*             m_pVideoContext;
  ID3D11VideoProcessorEnumerator* m_pEnumerator;
  D3D11_VIDEO_PROCESSOR_CAPS      m_vcaps;
  ID3D11VideoProcessor*           m_pVideoProcessor;
  bool                            m_bStereoEnabled = false;
  CSurfaceContext*                m_context;
  CCriticalSection                m_section;

  unsigned int                    m_procIndex;
  D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS m_rateCaps;
  D3D11_TEXTURE2D_DESC            m_texDesc;
};

};
