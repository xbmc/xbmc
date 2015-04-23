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

#include "threads/Event.h"
#include "DXVA.h"
#include <dxva2api.h>
#include "guilib/Geometry.h"
#include <dxvahd.h>

namespace DXVA {

// ProcAmp filters
const DXVAHD_FILTER PROCAMP_FILTERS[] =
{
    DXVAHD_FILTER_BRIGHTNESS,
    DXVAHD_FILTER_CONTRAST,
    DXVAHD_FILTER_HUE,
    DXVAHD_FILTER_SATURATION
};

const DWORD NUM_FILTERS = ARRAYSIZE(PROCAMP_FILTERS);

typedef HRESULT (__stdcall *DXVAHDCreateVideoServicePtr)(IDirect3DDevice9Ex *pD3DDevice, const DXVAHD_CONTENT_DESC *pContentDesc, DXVAHD_DEVICE_USAGE Usage, PDXVAHDSW_Plugin pPlugin, IDXVAHD_Device **ppDevice);

class CProcessorHD
  : public CProcessor
{
public:
  CProcessorHD();
 ~CProcessorHD();

  virtual bool           PreInit();
  virtual void           UnInit();
  virtual bool           Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format);
  virtual void           Close();
  virtual bool           Render(CRect src, CRect dst, IDirect3DSurface9* target, IDirect3DSurface9 **source, DWORD flags, UINT frameIdx);
  virtual unsigned       Size() { if (m_pDXVAHD) return m_size; return 0; }
  virtual unsigned       PastRefs() { return m_max_back_refs; }

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); UnInit(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); UnInit(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); Close(); }

protected:
  virtual bool LoadSymbols();
  virtual bool UpdateSize(const DXVA2_VideoDesc& dsc);
  virtual bool ReInit();
  virtual bool CreateSurfaces();
  virtual bool OpenProcessor();
  virtual bool ApplyFilter(DXVAHD_FILTER filter, int value, int min, int max, int def);

  IDXVAHD_Device          *m_pDXVAHD;      // DXVA-HD device.
  IDXVAHD_VideoProcessor  *m_pDXVAVP;      // DXVA-HD video processor.
  DXVAHD_VPDEVCAPS         m_VPDevCaps;
  DXVAHD_VPCAPS            m_VPCaps;
  unsigned int             m_width;
  unsigned int             m_height;
  D3DFORMAT                m_format;
  unsigned int             m_flags;
  unsigned int             m_renderFormat;

  struct ProcAmpInfo
  {
    bool                      bSupported;
    DXVAHD_FILTER_RANGE_DATA  Range;
  };
  ProcAmpInfo              m_Filters[NUM_FILTERS];

  static DXVAHDCreateVideoServicePtr m_DXVAHDCreateVideoService;
};

};
