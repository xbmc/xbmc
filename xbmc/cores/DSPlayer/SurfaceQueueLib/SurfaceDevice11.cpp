// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "SurfaceQueueImpl.h"

//-----------------------------------------------------------------------------
// Implementation of D3D11 Device Wrapper.  This is a simple wrapper around the
// public D3D11 APIs that are necessary for the shared surface queue.  See
// the comments in SharedSurfaceQueue.h to descriptions of these functions.
//-----------------------------------------------------------------------------
CSurfaceQueueDeviceD3D11::CSurfaceQueueDeviceD3D11(ID3D11Device* pD3D11Device) :
    m_pDevice(pD3D11Device)
{
    ASSERT(m_pDevice);
    m_pDevice->AddRef();
}

CSurfaceQueueDeviceD3D11::~CSurfaceQueueDeviceD3D11()
{
    m_pDevice->Release();
}

HRESULT CSurfaceQueueDeviceD3D11::CreateSharedSurface(
                                UINT Width, UINT Height, 
                                DXGI_FORMAT format, 
                                IUnknown** ppUnknown,
                                HANDLE* pHandle)
{
    ASSERT(m_pDevice);
    ASSERT(ppUnknown);
    ASSERT(pHandle);

    HRESULT hr;

    ID3D11Texture2D** ppTexture = (ID3D11Texture2D**)ppUnknown;

    D3D11_TEXTURE2D_DESC Desc;
    Desc.Width              = Width;
    Desc.Height             = Height;
    Desc.MipLevels          = 1;
    Desc.ArraySize          = 1;
    Desc.Format             = format;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Usage              = D3D11_USAGE_DEFAULT;
    Desc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    Desc.CPUAccessFlags     = 0;
    Desc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;

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

HRESULT CSurfaceQueueDeviceD3D11::OpenSurface(
                                    HANDLE hSharedHandle, 
                                    void** ppSurface, 
                                    UINT, 
                                    UINT, 
                                    DXGI_FORMAT)
{
    return m_pDevice->OpenSharedResource(hSharedHandle, __uuidof(ID3D11Texture2D), ppSurface);
}

HRESULT CSurfaceQueueDeviceD3D11::GetSharedHandle(IUnknown* pUnknown, HANDLE* pHandle)
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

HRESULT CSurfaceQueueDeviceD3D11::CreateCopyResource(DXGI_FORMAT format, UINT width, UINT height, IUnknown** ppRes)
{
    ASSERT(ppRes);
    ASSERT(m_pDevice);

    D3D11_TEXTURE2D_DESC Desc;
    Desc.Width              = width;
    Desc.Height             = height;
    Desc.MipLevels          = 1;
    Desc.ArraySize          = 1;
    Desc.Format             = format;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Usage              = D3D11_USAGE_STAGING;
    Desc.BindFlags          = 0;
    Desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
    Desc.MiscFlags          = 0;
   
    return m_pDevice->CreateTexture2D(&Desc, NULL, reinterpret_cast<ID3D11Texture2D**>(ppRes)); 
}

HRESULT CSurfaceQueueDeviceD3D11::CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height)
{
    HRESULT hr;
    
    D3D11_BOX UnitBox = {0, 0, 0, width, height, 1};
    
    ID3D11DeviceContext*    pContext = NULL;
    ID3D11Resource*         pSrcRes = NULL;
    ID3D11Resource*         pDstRes = NULL;

    m_pDevice->GetImmediateContext(&pContext);
    ASSERT(pContext);

    if (FAILED(hr = pDst->QueryInterface(__uuidof(ID3D11Resource), (void**)&pDstRes)))
    {
        goto end;
    }
    
    if (FAILED(hr = pSrc->QueryInterface(__uuidof(ID3D11Resource), (void**)&pSrcRes)))
    {
        goto end;
    }

    pContext->CopySubresourceRegion(
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
    if (pContext)
    {
        pContext->Release();
    }

    return hr;
}

HRESULT CSurfaceQueueDeviceD3D11::LockSurface(IUnknown* pSurface, DWORD flags)
{
    ASSERT(pSurface);

    HRESULT hr = S_OK;
    D3D11_MAPPED_SUBRESOURCE region;

    ID3D11Resource*         pResource   = NULL;
    ID3D11DeviceContext*    pContext    = NULL;
    DWORD                   d3d11flags  = 0;

    m_pDevice->GetImmediateContext(&pContext);
    ASSERT(pContext);
    
    if (flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        d3d11flags |= D3D11_MAP_FLAG_DO_NOT_WAIT;
    }
   
    if (FAILED(hr = pSurface->QueryInterface(__uuidof(ID3D11Resource), (void**)&pResource)))
    {
        goto end;
    }

    hr = pContext->Map(pResource, 0, D3D11_MAP_READ, d3d11flags, &region); 

end:
    if (pResource)
    {
        pResource->Release();
    }
    if (pContext)
    {
        pContext->Release();
    }
    return hr;
}

HRESULT CSurfaceQueueDeviceD3D11::UnlockSurface(IUnknown* pSurface)
{
    ASSERT(pSurface);

    HRESULT hr = S_OK;

    ID3D11DeviceContext*    pContext    = NULL;
    ID3D11Resource*         pResource   = NULL;

    m_pDevice->GetImmediateContext(&pContext);
    ASSERT(pContext);

    if (FAILED(hr = pSurface->QueryInterface(__uuidof(ID3D11Resource), (void**)&pResource)))
    {
        goto end;
    }

    pContext->Unmap(pResource, 0);

end:
    if (pResource)
    {
        pResource->Release();
    }
    if (pContext)
    {
        pContext->Release();
    }
    return hr;
}

BOOL CSurfaceQueueDeviceD3D11::ValidateREFIID(REFIID id)
{
    return (id == __uuidof(ID3D11Texture2D)) ||
		   (id == __uuidof(IDXGISurface));
}


