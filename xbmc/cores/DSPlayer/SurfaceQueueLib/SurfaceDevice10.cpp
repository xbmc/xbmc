// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "SurfaceQueueImpl.h"

//-----------------------------------------------------------------------------
// Implementation of D3D10 Device Wrapper.  This is a simple wrapper around the
// public D3D10 APIs that are necessary for the shared surface queue.  See
// the comments in SharedSurfaceQueue.h to descriptions of these functions.
//-----------------------------------------------------------------------------
CSurfaceQueueDeviceD3D10::CSurfaceQueueDeviceD3D10(ID3D10Device* pD3D10Device) :
    m_pDevice(pD3D10Device)
{
    ASSERT(m_pDevice);
    m_pDevice->AddRef();
}

CSurfaceQueueDeviceD3D10::~CSurfaceQueueDeviceD3D10()
{
    m_pDevice->Release();
}

HRESULT CSurfaceQueueDeviceD3D10::CreateSharedSurface(
                                UINT Width, UINT Height, 
                                DXGI_FORMAT format, 
                                IUnknown** ppUnknown,
                                HANDLE* pHandle)
{
    ASSERT(m_pDevice);
    ASSERT(ppUnknown);
    ASSERT(pHandle);

    HRESULT hr;

    ID3D10Texture2D** ppTexture = (ID3D10Texture2D**)ppUnknown;

    D3D10_TEXTURE2D_DESC Desc;
    Desc.Width              = Width;
    Desc.Height             = Height;
    Desc.MipLevels          = 1;
    Desc.ArraySize          = 1;
    Desc.Format             = format;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Usage              = D3D10_USAGE_DEFAULT;
    Desc.BindFlags          = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    Desc.CPUAccessFlags     = 0;
    Desc.MiscFlags          = D3D10_RESOURCE_MISC_SHARED;

    hr = m_pDevice->CreateTexture2D(&Desc, NULL, ppTexture);

    if (SUCCEEDED(hr))
    {
        if (FAILED( GetSharedHandle(*ppUnknown, pHandle)))
        {
            (*ppTexture)->Release();
            (*ppTexture) = NULL;
        }
    }

    return hr;
}

HRESULT CSurfaceQueueDeviceD3D10::OpenSurface(
                                    HANDLE hSharedHandle, 
                                    void** ppSurface, 
                                    UINT, 
                                    UINT, 
                                    DXGI_FORMAT)
{
    return m_pDevice->OpenSharedResource(hSharedHandle, __uuidof(ID3D10Texture2D), ppSurface);
}

HRESULT CSurfaceQueueDeviceD3D10::GetSharedHandle(IUnknown* pUnknown, HANDLE* pHandle)
{
    ASSERT(pUnknown);
    ASSERT(pHandle);

    HRESULT hr = S_OK;
    
    *pHandle = NULL;
    IDXGIResource* pSurface;

    if (FAILED(hr = pUnknown->QueryInterface(__uuidof(IDXGIResource), (void**)&pSurface)))
    {
        return hr;
    }

    hr = pSurface->GetSharedHandle(pHandle);
    pSurface->Release();

    return hr;
}

HRESULT CSurfaceQueueDeviceD3D10::CreateCopyResource(DXGI_FORMAT format, UINT width, UINT height, IUnknown** ppRes)
{
    ASSERT(ppRes);
    ASSERT(m_pDevice);

    D3D10_TEXTURE2D_DESC Desc;
    Desc.Width              = width;
    Desc.Height             = height;
    Desc.MipLevels          = 1;
    Desc.ArraySize          = 1;
    Desc.Format             = format;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Usage              = D3D10_USAGE_STAGING;
    Desc.BindFlags          = 0;
    Desc.CPUAccessFlags     = D3D10_CPU_ACCESS_READ;
    Desc.MiscFlags          = 0;
   
    return m_pDevice->CreateTexture2D(&Desc, NULL, reinterpret_cast<ID3D10Texture2D**>(ppRes)); 
}

HRESULT CSurfaceQueueDeviceD3D10::CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height)
{
    HRESULT hr;
    
    D3D10_BOX UnitBox = {0, 0, 0, width, height, 1};
    
    ID3D10Resource* pSrcRes = NULL;
    ID3D10Resource* pDstRes = NULL;

    if (FAILED(hr = pDst->QueryInterface(__uuidof(ID3D10Resource), (void**)&pDstRes)))
    {
        goto end;
    }
    
    if (FAILED(hr = pSrc->QueryInterface(__uuidof(ID3D10Resource), (void**)&pSrcRes)))
    {
        goto end;
    }

    m_pDevice->CopySubresourceRegion(
            pDstRes, 
            0, 
            0, 0, 0, //(x, y, z)
            pSrcRes,
            0, 
            &UnitBox);
end:
    if (pSrcRes)
    {
        pSrcRes->Release();
    }
    if (pDstRes)
    {
        pDstRes->Release();
    }

    return hr;
}

HRESULT CSurfaceQueueDeviceD3D10::LockSurface(IUnknown* pSurface, DWORD flags)
{
    ASSERT(pSurface);

    HRESULT                 hr          = S_OK;
    ID3D10Texture2D*        pTex2D      = NULL;
    DWORD                   d3d10flags  = 0;
    D3D10_MAPPED_TEXTURE2D  region;

    if (flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        flags |= D3D10_MAP_FLAG_DO_NOT_WAIT;
    }
    
    if (FAILED(hr = pSurface->QueryInterface(__uuidof(ID3D10Texture2D), (void**)&pTex2D)))
    {
        goto end;
    }
    
    hr = pTex2D->Map(0, D3D10_MAP_READ, d3d10flags, &region);

end:
    if (pTex2D)
    {
        pTex2D->Release();
    }
    return hr;
}

HRESULT CSurfaceQueueDeviceD3D10::UnlockSurface(IUnknown* pSurface)
{
    ASSERT(pSurface);

    HRESULT hr = S_OK;
    ID3D10Texture2D* pTex2D = NULL;

    if (FAILED(hr = pSurface->QueryInterface(__uuidof(ID3D10Texture2D), (void**)&pTex2D)))
    {
		return hr;
    }

    pTex2D->Unmap(0);
    pTex2D->Release();

	return hr;
}

BOOL CSurfaceQueueDeviceD3D10::ValidateREFIID(REFIID id)
{
    return (id == __uuidof(ID3D10Texture2D)) ||
		   (id == __uuidof(IDXGISurface));
}

