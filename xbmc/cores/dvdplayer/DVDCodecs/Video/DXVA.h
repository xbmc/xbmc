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

namespace DXVA {

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

  int   GetBuffer(AVCodecContext *avctx, AVFrame *pic);
  void  RelBuffer(AVCodecContext *avctx, AVFrame *pic);

  static bool Supports(enum PixelFormat fmt);

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }
  virtual void OnLostDevice()    { CSingleLock lock(m_section); m_state = DXVA_LOST;  m_event.Reset(); }
  virtual void OnResetDevice()   { CSingleLock lock(m_section); m_state = DXVA_RESET; m_event.Set();   }

protected:
  enum EDeviceState
  { DXVA_OPEN
  , DXVA_RESET
  , DXVA_LOST
  } m_state;

  bool  OpenTarget(const GUID &guid);

  IDirectXVideoDecoderService* m_service;
  GUID                         m_input;
  DXVA2_VideoDesc              m_format;

  LPDIRECT3DSURFACE9*          m_surfaces;
  unsigned                     m_read;
  unsigned                     m_write;

  unsigned                     m_level;

  unsigned                     m_refs;

  struct dxva_context*         m_context;

  CCriticalSection             m_section;
  CEvent                       m_event;
};

class CProcessor
  : public ID3DResource
{
public:
  CProcessor();
  ~CProcessor();

  bool           Create();
  bool           Open(UINT width, UINT height);
  bool           Open(UINT width, UINT height, D3DFORMAT format);
  bool           CreateSurfaces();
  bool           LockSurfaces(LPDIRECT3DSURFACE9* surfaces, unsigned count);
  void           Close();

  bool           IsOpened() { return m_opened; }
  unsigned       GetSize() { return m_size; }

  void           StillFrame();
  bool           Render(const RECT& dst, IDirect3DSurface9* target, const REFERENCE_TIME time, int fieldflag);

  bool           ProcessPicture(DVDVideoPicture* picture);

  CProcessor* Acquire();
  long        Release();

  virtual void OnCreateDevice()  {}
  virtual void OnDestroyDevice() { Close(); }
  virtual void OnLostDevice()    { Close(); }
  virtual void OnResetDevice()   { Close(); }

protected:
  bool           SelectProcessor();
  REFERENCE_TIME Add(IDirect3DSurface9* source);

  GUID                           m_progdevice;
  GUID                           m_bobdevice;
  GUID                           m_hqdevice;

  GUID                           m_device;

  GUID                           m_default;

  IDirectXVideoProcessorService* m_service;
  IDirectXVideoProcessor*        m_process;

  EINTERLACEMETHOD             m_CurrInterlaceMethod;
  unsigned                     m_StreamSampleFormat;
  int                          m_BFF;

  DXVA2_VideoProcessorCaps m_caps;
  DXVA2_VideoDesc  m_desc;

  DXVA2_ValueRange m_brightness;
  DXVA2_ValueRange m_contrast;
  DXVA2_ValueRange m_hue;
  DXVA2_ValueRange m_saturation;

  CCriticalSection  m_section;
  long              m_references;

  LPDIRECT3DSURFACE9* m_surfaces;
  unsigned            m_count;

  struct VideoSample
  {
	  REFERENCE_TIME Time;
	  IDirect3DSurface9* SrcSurface;
  };

  VideoSample* m_samples;
  unsigned m_index;
  unsigned m_size;

  REFERENCE_TIME m_time;

  bool m_opened;
};

};
