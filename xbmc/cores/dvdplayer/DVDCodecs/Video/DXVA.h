/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DllAvCodec.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "guilib/D3DResource.h"
#include "threads/Event.h"
#include "DVDResource.h"
#include <dxva2api.h>
#include <deque>
#include <vector>
#include "settings/VideoSettings.h"
#include "guilib/Geometry.h"

namespace DXVA {

class CSurfaceContext
  : public IDVDResourceCounted<CSurfaceContext>
{
public:
  CSurfaceContext();
  ~CSurfaceContext();

   void HoldSurface(IDirect3DSurface9* surface);

protected:
  std::vector<IDirect3DSurface9*> m_heldsurfaces;
};

class CDecoder
  : public CDVDVideoCodecFFmpeg::IHardwareDecoder
  , public ID3DResource
{
public:
  CDecoder();
 ~CDecoder();
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat, unsigned int surfaces);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual int  Check     (AVCodecContext* avctx);
  virtual void Close();
  virtual const std::string Name() { return "dxva2"; }

  bool  OpenTarget(const GUID &guid);
  bool  OpenDecoder();
  int   GetBuffer(AVCodecContext *avctx, AVFrame *pic);
  void  RelBuffer(AVCodecContext *avctx, AVFrame *pic);

  static bool      Supports(enum PixelFormat fmt);


protected:
  enum EDeviceState
  { DXVA_OPEN
  , DXVA_RESET
  , DXVA_LOST
  } m_state;

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); m_state = DXVA_RESET; m_event.Set();   }

  struct SVideoBuffer
  {
    SVideoBuffer();
   ~SVideoBuffer();
    void Clear();

    IDirect3DSurface9* surface;
    bool               used;
    int                age;
  };

  IDirectXVideoDecoderService* m_service;
  IDirectXVideoDecoder*        m_decoder;
  HANDLE                       m_device;
  GUID                         m_input;
  DXVA2_VideoDesc              m_format;
  static const unsigned        m_buffer_max = 32;
  SVideoBuffer                 m_buffer[m_buffer_max];
  unsigned                     m_buffer_count;
  unsigned                     m_buffer_age;
  int                          m_refs;

  struct dxva_context*         m_context;

  CSurfaceContext*             m_surface_context;

  unsigned int                 m_shared;

  CCriticalSection             m_section;
  CEvent                       m_event;
};

class CProcessor
  : public ID3DResource
{
public:
  CProcessor();
 ~CProcessor();

  bool           PreInit();
  void           UnInit();
  bool           Open(UINT width, UINT height, unsigned int flags, unsigned int format, unsigned int extended_format);
  void           Close();
  REFERENCE_TIME Add(DVDVideoPicture* picture);
  bool           Render(CRect src, CRect dst, IDirect3DSurface9* target, const REFERENCE_TIME time, DWORD flags);
  unsigned       Size() { if (m_service) return m_size; return 0; }

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); Close(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); Close(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); Close(); }

protected:
  bool UpdateSize(const DXVA2_VideoDesc& dsc);
  bool CreateSurfaces();
  bool OpenProcessor();
  bool SelectProcessor();
  void EvaluateQuirkNoDeintProcForProg();

  IDirectXVideoProcessorService* m_service;
  IDirectXVideoProcessor*        m_process;
  GUID                           m_device;

  DXVA2_VideoProcessorCaps m_caps;
  DXVA2_VideoDesc  m_desc;

  DXVA2_ValueRange m_brightness;
  DXVA2_ValueRange m_contrast;
  DXVA2_ValueRange m_hue;
  DXVA2_ValueRange m_saturation;
  REFERENCE_TIME   m_time;
  unsigned         m_size;
  unsigned         m_max_back_refs;
  unsigned         m_max_fwd_refs;
  EDEINTERLACEMODE m_deinterlace_mode;
  EINTERLACEMETHOD m_interlace_method;
  bool             m_progressive; // true for progressive source or to force ignoring interlacing flags.
  unsigned         m_index;

  struct SVideoSample
  {
    DXVA2_VideoSample sample;
    CSurfaceContext* context;
  };

  typedef std::deque<SVideoSample> SSamples;
  SSamples          m_sample;

  CCriticalSection  m_section;

  LPDIRECT3DSURFACE9* m_surfaces;
  CSurfaceContext* m_context;

  bool             m_quirk_nodeintprocforprog;
};

};
