/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "settings/VideoSettings.h"
#include "DllAvCodec.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "guilib/D3DResource.h"
#include "threads/Event.h"
#include <dxva2api.h>
#include <deque>
#include <vector>

namespace DXVA {

class CProcessor;

class CDecoder
  : public CDVDVideoCodecFFmpeg::IHardwareDecoder
  , public ID3DResource
{
public:
  CDecoder();
 ~CDecoder();
  virtual bool Open      (AVCodecContext* avctx, const enum PixelFormat);
  virtual int  Decode    (AVCodecContext* avctx, AVFrame* frame);
  virtual bool GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture);
  virtual int  Check     (AVCodecContext* avctx);
  virtual void Close();
  virtual const std::string Name() { return "dxva2"; }

  bool  OpenProcessor();
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

  EINTERLACEMETHOD             m_CurrInterlaceMethod;
  unsigned                     m_SampleFormat;
  unsigned                     m_StreamSampleFormat;

  struct dxva_context*         m_context;

  CProcessor*                  m_processor;

  CCriticalSection             m_section;
  CEvent                       m_event;
};

class CProcessor
  : public ID3DResource
{
public:
  CProcessor();
 ~CProcessor();

  bool           Open(const DXVA2_VideoDesc& dsc);
  void           Close();
  void           HoldSurface(IDirect3DSurface9* surface);
  REFERENCE_TIME Add(IDirect3DSurface9* source);
  bool           Render(const RECT& dst, IDirect3DSurface9* target, const REFERENCE_TIME time, int fieldflag);
  int            Size() { return m_size; }

  CProcessor* Acquire();
  long        Release();

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); Close(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); Close(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); Close(); }

  IDirectXVideoProcessorService* m_service;
  IDirectXVideoProcessor*        m_process;
  GUID                           m_device;

  unsigned                     m_SampleFormat;
  int                          m_BFF;

  DXVA2_VideoProcessorCaps m_caps;
  DXVA2_VideoDesc  m_desc;

  DXVA2_ValueRange m_brightness;
  DXVA2_ValueRange m_contrast;
  DXVA2_ValueRange m_hue;
  DXVA2_ValueRange m_saturation;
  REFERENCE_TIME   m_time;
  unsigned         m_size;

  typedef std::deque<DXVA2_VideoSample> SSamples;
  SSamples          m_sample;

  CCriticalSection  m_section;
  long              m_references;

protected:
  std::vector<IDirect3DSurface9*> m_heldsurfaces;
};

};
