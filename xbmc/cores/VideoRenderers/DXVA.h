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

#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDCodecs/Video/DXVA.h"
#include "guilib/D3DResource.h"
#include "threads/Event.h"
#include <dxva2api.h>
#include <deque>
#include <vector>
#include "settings/VideoSettings.h"
#include "guilib/Geometry.h"

namespace DXVA {

class CProcessor
  : public ID3DResource
{
public:
  CProcessor();
 ~CProcessor();

  virtual bool           PreInit();
  virtual void           UnInit();
  virtual bool           Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format);
  virtual void           Close();
  virtual CRenderPicture *Convert(DVDVideoPicture* picture);
  virtual bool           Render(CRect src, CRect dst, IDirect3DSurface9* target, IDirect3DSurface9** source, DWORD flags, UINT frameIdx);
  virtual unsigned       Size() { if (m_service) return m_size; return 0; }
  virtual unsigned       PastRefs() { return m_max_back_refs; }

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); Close(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); Close(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); Close(); }

protected:
  virtual bool LoadSymbols();
  virtual bool UpdateSize(const DXVA2_VideoDesc& dsc);
  virtual bool CreateSurfaces();
  virtual bool OpenProcessor();
  virtual bool SelectProcessor();
  virtual void EvaluateQuirkNoDeintProcForProg();

  IDirectXVideoProcessorService* m_service;
  IDirectXVideoProcessor*        m_process;
  GUID                           m_device;

  DXVA2_VideoProcessorCaps m_caps;
  DXVA2_VideoDesc  m_desc;

  DXVA2_ValueRange m_brightness;
  DXVA2_ValueRange m_contrast;
  DXVA2_ValueRange m_hue;
  DXVA2_ValueRange m_saturation;
  unsigned         m_size;
  unsigned         m_max_back_refs;
  unsigned         m_max_fwd_refs;
  EDEINTERLACEMODE m_deinterlace_mode;
  EINTERLACEMETHOD m_interlace_method;
  bool             m_progressive; // true for progressive source or to force ignoring interlacing flags.

  CCriticalSection    m_section;
  CSurfaceContext*    m_context;
  bool                m_quirk_nodeintprocforprog;
  static CCriticalSection    m_dlSection;
  static HMODULE      m_dlHandle;
  static DXVA2CreateVideoServicePtr m_DXVA2CreateVideoService;
};

};
