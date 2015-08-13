// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "SurfaceQueueImpl.h"

//-----------------------------------------------------------------------------
// Implementation of D3D9 Device Wrapper.  This is a simple wrapper around the
// public D3D9Ex APIs that are necessary for the shared surface queue.  See
// the comments in SharedSurfaceQueue.h to descriptions of these functions.
//-----------------------------------------------------------------------------

//
// D3D9Ex does not have an API to get shared handles.  We replicate that functionality
// using setprivatedata.
//
static GUID SharedHandleGuid = {0x91facf2d, 0xe464, 0x4495, 0x84, 0xa6, 0x37, 0xbe, 0xd3, 0x56, 0x8d, 0xa3};

//
// This function will convert from DXGI formats (d3d10/d3d11) to D3D9 formats.
// Most formtas are not cross api shareable and for those the function will
// return D3DFMT_UNKNOWN.
//
D3DFORMAT DXGIToCrossAPID3D9Format(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return D3DFMT_A8R8G8B8;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: 
            return D3DFMT_A8R8G8B8;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return D3DFMT_X8R8G8B8;
        case DXGI_FORMAT_R8G8B8A8_UNORM: 
            return D3DFMT_A8B8G8R8;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: 
            return D3DFMT_A8B8G8R8;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return D3DFMT_A2B10G10R10;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: 
            return D3DFMT_A16B16G16R16F;
        default:
            return D3DFMT_UNKNOWN;
    };
}


CSurfaceQueueDeviceD3D9::CSurfaceQueueDeviceD3D9(IDirect3DDevice9Ex* pD3D9Device) :
    m_pDevice(pD3D9Device)
{
    ASSERT(m_pDevice);
    m_pDevice->AddRef();
}

CSurfaceQueueDeviceD3D9::~CSurfaceQueueDeviceD3D9()
{
    m_pDevice->Release();
}

HRESULT CSurfaceQueueDeviceD3D9::CreateSharedSurface(
                                UINT Width, UINT Height, 
                                DXGI_FORMAT Format, 
                                IUnknown** ppTexture,
                                HANDLE* pHandle)
{
    ASSERT(m_pDevice);

    D3DFORMAT D3D9Format;

    if ((D3D9Format = DXGIToCrossAPID3D9Format(Format)) == D3DFMT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;

    *pHandle = NULL;

    hr = m_pDevice->CreateTexture(Width, Height, 1,
                                  D3DUSAGE_RENDERTARGET,
                                  D3D9Format,
                                  D3DPOOL_DEFAULT,
                                  (IDirect3DTexture9**)ppTexture,
                                  pHandle);
    return hr;
}

HRESULT CSurfaceQueueDeviceD3D9::OpenSurface(
                                    HANDLE hSharedHandle, 
                                    void** ppUnknown, 
                                    UINT Width, 
                                    UINT Height, 
                                    DXGI_FORMAT Format)
{
    
    D3DFORMAT D3D9Format;

    // If the format is not cross api shareable the utility function will return
    // D3DFMT_UNKNOWN
    if ((D3D9Format = DXGIToCrossAPID3D9Format(Format)) == D3DFMT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    IDirect3DTexture9** ppTexture = (IDirect3DTexture9**)ppUnknown;

    hr = m_pDevice->CreateTexture(Width, Height, 1,
                                  D3DUSAGE_RENDERTARGET,
                                  D3D9Format,
                                  D3DPOOL_DEFAULT,
                                  ppTexture,
                                  &hSharedHandle);
    if (SUCCEEDED(hr))
    {
        // Store the shared handle
        hr = (*ppTexture)->SetPrivateData(SharedHandleGuid, &hSharedHandle, sizeof(HANDLE), 0);

        if (FAILED(hr))
        {
            (*ppTexture)->Release();
            *ppTexture = NULL;
        }
    }

    return hr;
}

HRESULT CSurfaceQueueDeviceD3D9::GetSharedHandle(IUnknown* pUnknown, HANDLE* pHandle)
{
    ASSERT(pUnknown);
    ASSERT(pHandle);
    
    HRESULT hr = S_OK;
    
    *pHandle = NULL;
    IDirect3DTexture9* pTexture;

    if (FAILED(hr = pUnknown->QueryInterface(__uuidof(IDirect3DTexture9), (void**)&pTexture)))
    {
        return hr;
    }

    DWORD size = sizeof(HANDLE);

    hr = pTexture->GetPrivateData(SharedHandleGuid, pHandle, &size);
    pTexture->Release();

    return hr;
}

HRESULT CSurfaceQueueDeviceD3D9::CreateCopyResource(DXGI_FORMAT Format, UINT width, UINT height, IUnknown** ppRes)
{
      
    D3DFORMAT D3D9Format;

    if ((D3D9Format = DXGIToCrossAPID3D9Format(Format)) == D3DFMT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    return m_pDevice->CreateRenderTarget(
            width, height,
            D3D9Format,
            D3DMULTISAMPLE_NONE,
            0,
            TRUE,
            (IDirect3DSurface9**)ppRes,
            NULL
            );
}

HRESULT CSurfaceQueueDeviceD3D9::CopySurface(IUnknown* pDst, IUnknown* pSrc, UINT width, UINT height)
{
    ASSERT(pDst);
    ASSERT(pSrc);
    ASSERT(m_pDevice);

    HRESULT             hr          = S_OK;
    IDirect3DSurface9*  pSrcSurf    = NULL;
    IDirect3DSurface9*  pDstSurf    = NULL;
    IDirect3DTexture9*  pSrcTex     = NULL;
    RECT                rect        = {0, 0, width, height };
   
    // The source should be a IDirect3DTexture9.  We need to QI for it and then get the
    // top most surface from it.
    if (FAILED(hr = pSrc->QueryInterface(__uuidof(IDirect3DTexture9), (void**)&pSrcTex)))
    {
        goto end;
    }
    if (FAILED(hr = pSrcTex->GetSurfaceLevel(0, &pSrcSurf)))
    {
        goto end;
    }
    // The dst is a IDirect3DSurface9 so we can simply QI for it.
    if (FAILED(hr = pDst->QueryInterface(__uuidof(IDirect3DSurface9), (void**)&pDstSurf)))
    {
        goto end;
    }

    hr = m_pDevice->StretchRect(pSrcSurf, &rect, pDstSurf, &rect, D3DTEXF_NONE);

end:
    if (pSrcTex)
    {
        pSrcTex->Release();
    }
    if (pSrcSurf)
    {
        pSrcSurf->Release();
    }
    if (pDstSurf)
    {
        pDstSurf->Release();
    }
    return hr;
}

HRESULT CSurfaceQueueDeviceD3D9::LockSurface(IUnknown* pSurface, DWORD flags)
{
    ASSERT(pSurface);

    HRESULT             hr          = S_OK;
    IDirect3DSurface9*  pSurf       = NULL;
    DWORD               d3d9flags   = D3DLOCK_READONLY;
    D3DLOCKED_RECT      region;

    
    if (flags & SURFACE_QUEUE_FLAG_DO_NOT_WAIT)
    {
        d3d9flags |= D3DLOCK_DONOTWAIT;
    }

    if (FAILED(hr = pSurface->QueryInterface(__uuidof(IDirect3DSurface9), (void**)&pSurf)))
    {
        goto end;
    }
   
    hr = pSurf->LockRect(&region, NULL, d3d9flags);

end:
    if (pSurf)
    {
        pSurf->Release();
    }
    if (hr == D3DERR_WASSTILLDRAWING)
    {
        hr = DXGI_ERROR_WAS_STILL_DRAWING;
    }
    return hr;
}

HRESULT CSurfaceQueueDeviceD3D9::UnlockSurface(IUnknown* pSurface)
{
    ASSERT(pSurface);

    HRESULT             hr          = S_OK;
    IDirect3DSurface9*  pSurf       = NULL;

    if (FAILED(hr = pSurface->QueryInterface(__uuidof(IDirect3DSurface9), (void**)&pSurf)))
    {
        goto end;
    }

    hr = pSurf->UnlockRect();

end:
    if (pSurf)
    {
        pSurf->Release();
    }
    return hr;
}

BOOL CSurfaceQueueDeviceD3D9::ValidateREFIID(REFIID id)
{
    return id == __uuidof(IDirect3DTexture9);
}

