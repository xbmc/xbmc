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

#include "dxva1wrapper.h"
CDXDecWrapper::CDXDecWrapper(IAMVideoAccelerator *decoder, int count, void **surface)
: CUnknown("CDXDecWrapper",NULL)
{ 
  m_pAMVideoAccelerator = decoder;
  surface_count = count;
  surface_id = (void**)calloc(count, sizeof(*surface));
  for (int i = 0; i < surface_count; i++)
    surface_id[i] = surface[i];
  
  DWORD req_count = 0;
  if (FAILED(m_pAMVideoAccelerator->GetInternalCompBufferInfo(&req_count, NULL)))
    req_count = 0;
  buffer_info_count = req_count;
  buffer_info = (AMVACompBufferInfo *) calloc(req_count, sizeof(*buffer_info));
 
  if (req_count > 0 && FAILED(m_pAMVideoAccelerator->GetInternalCompBufferInfo(&req_count, buffer_info)))
      buffer_info_count = 0;

}

CDXDecWrapper::~CDXDecWrapper()
{
  free(buffer_info);
  free(surface_id);
}

int CDXDecWrapper::GetSurfaceIndex(void *id)
{
  for (int i = 0; i < surface_count; i++) 
  {
    if (surface_id[i] == id)
        return i;
  }
    ASSERT(0);
    return 0;
}

STDMETHODIMP CDXDecWrapper::GetVideoDecoderService(IDirectXVideoDecoderService **ppService)
{
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::GetCreationParameters(GUID *pDeviceGuid, DXVA2_VideoDesc *pVideoDesc, DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 ***pDecoderRenderTargets, UINT *pNumSurfaces)
{
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::GetBuffer(UINT BufferType, void **ppBuffer, UINT *pBufferSize)
{
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::ReleaseBuffer(UINT BufferType)
{
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::BeginFrame(IDirect3DSurface9 *pRenderTarget, void *pvPVPData)
{
  DWORD index = GetSurfaceIndex(pRenderTarget);
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::EndFrame(HANDLE *pHandleComplete)
{
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::Execute(const DXVA2_DecodeExecuteParams *pExecuteParams)
{
  return E_NOTIMPL;
}