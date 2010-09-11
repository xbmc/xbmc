/*
 *      Copyright (C) 2010 Team XBMC
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
#include "dxva2api.h"
#include "streams.h"
#include "dxva.h"
#include "videoacc.h"
#include "amva.h"
class CDXDecWrapper 
  : public CUnknown
  , public IDirectXVideoDecoder
{
public:
  CDXDecWrapper(IAMVideoAccelerator *decoder, int count, void **surface);
  virtual ~CDXDecWrapper();
  DECLARE_IUNKNOWN
  STDMETHODIMP GetVideoDecoderService(IDirectXVideoDecoderService **ppService);
  STDMETHODIMP GetCreationParameters(GUID *pDeviceGuid, DXVA2_VideoDesc *pVideoDesc, DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 ***pDecoderRenderTargets, UINT *pNumSurfaces);
  STDMETHODIMP GetBuffer(UINT BufferType, void **ppBuffer, UINT *pBufferSize);
  STDMETHODIMP ReleaseBuffer(UINT BufferType);
  STDMETHODIMP BeginFrame(IDirect3DSurface9 *pRenderTarget, void *pvPVPData);
  STDMETHODIMP EndFrame(HANDLE *pHandleComplete);
  STDMETHODIMP Execute(const DXVA2_DecodeExecuteParams *pExecuteParams);
  int GetCurrentIndex() { return current_surface_index; }
protected:
  int GetSurfaceIndex(void *id);
  IAMVideoAccelerator *m_pAMVideoAccelerator;
  int  surface_count;
  void **surface_id;
  int  buffer_info_count;
  int  current_surface_index;
  AMVACompBufferInfo *buffer_info;

};



typedef struct {
    IDirectXVideoDecoder     *wrapper;
    //IDirectXVideoDecoderVtbl* vtbl;
 
    IAMVideoAccelerator *decoder;
 
 
    int  surface_count;
    void **surface_id;
 
    /* */
    int                buffer_info_count;
    AMVACompBufferInfo *buffer_info;
} va_wrapper;