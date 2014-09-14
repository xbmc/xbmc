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

#include "libavcodec/avcodec.h"
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

#define CHECK(a) \
do { \
  HRESULT res = a; \
  if(FAILED(res)) \
  { \
    CLog::Log(LOGERROR, "DXVA - failed executing "#a" at line %d with error %x", __LINE__, res); \
    return false; \
  } \
} while(0);

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

typedef HRESULT(__stdcall *DXVA2CreateVideoServicePtr)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);
class CDecoder;
class CDXVAContext
{
public:
  static bool EnsureContext(CDXVAContext **ctx, CDecoder *decoder);
  bool GetInputAndTarget(int codec, GUID &inGuid, D3DFORMAT &outFormat);
  bool GetConfig(GUID &inGuid, const DXVA2_VideoDesc *format, DXVA2_ConfigPictureDecode &config);
  bool CreateSurfaces(int width, int height, D3DFORMAT format, unsigned int count, LPDIRECT3DSURFACE9 *surfaces);
  bool CreateDecoder(GUID &inGuid, DXVA2_VideoDesc *format, const DXVA2_ConfigPictureDecode *config, LPDIRECT3DSURFACE9 *surfaces, unsigned int count, IDirectXVideoDecoder **decoder);
  void Release(CDecoder *decoder);
private:
  CDXVAContext();
  void Close();
  bool LoadSymbols();
  bool CreateContext();
  void DestroyContext();
  void QueryCaps();
  bool IsValidDecoder(CDecoder *decoder);
  static CDXVAContext *m_context;
  static CCriticalSection m_section;
  static HMODULE m_dlHandle;
  static DXVA2CreateVideoServicePtr m_DXVA2CreateVideoService;
  IDirectXVideoDecoderService* m_service;
  int m_refCount;
  UINT m_input_count;
  GUID *m_input_list;
  std::vector<CDecoder*> m_decoders;
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
  virtual unsigned GetAllowedReferences();

  bool  OpenTarget(const GUID &guid);
  bool  OpenDecoder();
  int   GetBuffer(AVCodecContext *avctx, AVFrame *pic, int flags);
  void  RelBuffer(uint8_t *data);

  static bool      Supports(enum PixelFormat fmt);

  void CloseDXVADecoder();

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
  CDXVAContext*                m_dxva_context;

  unsigned int                 m_shared;

  CCriticalSection             m_section;
  CEvent                       m_event;
};

};
