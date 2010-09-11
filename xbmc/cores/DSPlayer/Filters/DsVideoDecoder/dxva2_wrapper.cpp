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

#include "dxva2_wrapper.h"
#include "log.h"
#include "DSUtil/DSUtil.h"
#define MAX_RETRY_ON_PENDING		50
#define DO_DXVA_PENDING_LOOP(x)		nTry = 0; \
									while (FAILED(hr = x) && nTry<MAX_RETRY_ON_PENDING) \
									{ \
										if (hr != E_PENDING) break; \
										Sleep(1); \
										nTry++; \
									}

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

inline int GetDXVA1BufferType(UINT type) 
{
  if (type <= DXVA2_BitStreamDateBufferType)
    return type + 1;
  switch (type) 
  {
    case DXVA2_PictureParametersBufferType:
      return DXVA_PICTURE_DECODE_BUFFER;
    case DXVA2_MacroBlockControlBufferType:
      return DXVA_MACROBLOCK_CONTROL_BUFFER;
    case DXVA2_ResidualDifferenceBufferType:
      return DXVA_RESIDUAL_DIFFERENCE_BUFFER;
    case DXVA2_DeblockingControlBufferType:
      return DXVA_DEBLOCKING_CONTROL_BUFFER;
    case DXVA2_InverseQuantizationMatrixBufferType:
      return DXVA_INVERSE_QUANTIZATION_MATRIX_BUFFER;
    case DXVA2_SliceControlBufferType:
      return DXVA_SLICE_CONTROL_BUFFER;
    case DXVA2_BitStreamDateBufferType:
      return DXVA_BITSTREAM_DATA_BUFFER;
    case DXVA2_MotionVectorBuffer:
      return DXVA_MOTION_VECTOR_BUFFER;
    case DXVA2_FilmGrainBuffer:
      return DXVA_FILM_GRAIN_BUFFER;
    default:
      return -1;
  }
}

STDMETHODIMP CDXDecWrapper::GetVideoDecoderService(IDirectXVideoDecoderService **ppService)
{
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::GetCreationParameters(GUID *pDeviceGuid, DXVA2_VideoDesc *pVideoDesc, DXVA2_ConfigPictureDecode *pConfig, IDirect3DSurface9 ***pDecoderRenderTargets, UINT *pNumSurfaces)
{
  CLog::Log(LOGWARNING,"%s",__FUNCTION__);
  return E_NOTIMPL;
}

STDMETHODIMP CDXDecWrapper::GetBuffer(UINT BufferType, void **ppBuffer, UINT *pBufferSize)
{
  DWORD index = 0;
  int type_dxva1 = GetDXVA1BufferType(BufferType);
  if (type_dxva1 < 0 || type_dxva1 >= buffer_info_count)
      return E_FAIL;
 
  BYTE *buffer;
  LONG stride;
  int nTry;
  HRESULT hr;
  for (int i = 0; i < 10; i++)
  {
    hr = DO_DXVA_PENDING_LOOP (m_pAMVideoAccelerator->QueryRenderStatus ((DWORD)-1, (DWORD)0, 0));
    //In case the buffer is in use
    if (SUCCEEDED(hr))
      break;
    else
    {
      CStdString error = GetDshowError(hr);
      CLog::Log(LOGERROR,"%s %s", __FUNCTION__, error.c_str());
    }
    Sleep(1);
  }
  
  hr = DO_DXVA_PENDING_LOOP(m_pAMVideoAccelerator->GetBuffer(type_dxva1, index, FALSE, (void**)&buffer, &stride));
  if (FAILED(hr))
  {
    CStdString error = GetDshowError(hr);
    CLog::Log(LOGERROR,"%s %s", __FUNCTION__, error.c_str());
    return E_FAIL;
  }
    
 
  *ppBuffer = buffer;
  *pBufferSize = buffer_info[type_dxva1].dwBytesToAllocate;
  return S_OK;
}

STDMETHODIMP CDXDecWrapper::ReleaseBuffer(UINT BufferType)
{
  return S_OK;
  DWORD index = 0;
  int type_dxva1 = GetDXVA1BufferType(BufferType);
  if (type_dxva1 < 0)
    return E_FAIL;
  int nTry;
  HRESULT hr;
  return DO_DXVA_PENDING_LOOP(m_pAMVideoAccelerator->ReleaseBuffer(type_dxva1, index));
}

STDMETHODIMP CDXDecWrapper::BeginFrame(IDirect3DSurface9 *pRenderTarget, void *pvPVPData)
{
  DWORD index = GetSurfaceIndex(pRenderTarget);
  AMVABeginFrameInfo info;
  info.dwDestSurfaceIndex = index;
  info.dwSizeInputData    = sizeof(index);
  info.pInputData         = &index;
  info.dwSizeOutputData   = 0;
  info.pOutputData        = NULL;
  int nTry;
  HRESULT hr;
  hr = DO_DXVA_PENDING_LOOP(m_pAMVideoAccelerator->BeginFrame(&info));
  if (SUCCEEDED(hr))
  {
    current_surface_index = index;
    return S_OK;
  }
  else
    return E_FAIL;

}

STDMETHODIMP CDXDecWrapper::EndFrame(HANDLE *pHandleComplete)
{
  AMVAEndFrameInfo info;
  info.dwSizeMiscData = sizeof(current_surface_index);
  info.pMiscData      = &current_surface_index;
  int nTry;
  HRESULT hr;
  hr = DO_DXVA_PENDING_LOOP(m_pAMVideoAccelerator->EndFrame(&info));
  if (FAILED(hr))
  {
    CStdString error = GetDshowError(hr);
    CLog::Log(LOGERROR,"%s %s", __FUNCTION__, error.c_str());
    return hr;
  }
  return S_OK;
  
}

STDMETHODIMP CDXDecWrapper::Execute(const DXVA2_DecodeExecuteParams *pExecuteParams)
{
  const int pNumCompBufferInfo = pExecuteParams->NumCompBuffers;
  AMVABUFFERINFO info[6];
  DXVA_BufferDescription dsc[6];
  for (int i = 0; i < pNumCompBufferInfo; i++) 
  {
    const DXVA2_DecodeBufferDesc *b = &pExecuteParams->pCompressedBuffers[i];
    dsc[i].dwTypeIndex = GetDXVA1BufferType(b->CompressedBufferType);
    dsc[i].dwBufferIndex    = 0;
    dsc[i].dwDataOffset     = b->DataOffset;
    dsc[i].dwDataSize       = b->DataSize;
    dsc[i].dwFirstMBaddress = b->FirstMBaddress;
    dsc[i].dwNumMBsInBuffer = b->NumMBsInBuffer;
    dsc[i].dwWidth          = b->Width;
    dsc[i].dwHeight         = b->Height;
    dsc[i].dwStride         = b->Stride;
    dsc[i].dwReservedBits   = b->ReservedBits;
    info[i].dwTypeIndex     = GetDXVA1BufferType(b->CompressedBufferType);
    info[i].dwBufferIndex   = 0;
    info[i].dwDataOffset    = b->DataOffset;
    info[i].dwDataSize      = b->DataSize;
  }
 
  DWORD function = 0x01000000;
  DWORD result;
  HRESULT hr = m_pAMVideoAccelerator->Execute((DWORD)0x01000000, dsc, 
                                              sizeof(DXVA_BufferDescription) * pNumCompBufferInfo,
                                              &result, sizeof(result), pNumCompBufferInfo, info);
  if (SUCCEEDED(hr))
  {
    for (int i=0; i<pNumCompBufferInfo; i++)
		{
			ASSERT (SUCCEEDED (m_pAMVideoAccelerator->ReleaseBuffer (info[i].dwTypeIndex, info[i].dwBufferIndex)));
			
		}
  }
  return hr;
}