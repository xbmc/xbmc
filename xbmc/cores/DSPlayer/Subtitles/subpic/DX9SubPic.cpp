/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <d3d9.h>
#include <Vmr9.h>
#include "DX9SubPic.h"
#include "..\ILog.h"

//
// CDX9SubPic
//

CDX9SubPic::CDX9SubPic(IDirect3DSurface9* pSurface, CDX9SubPicAllocator *pAllocator)
  : m_pSurface(pSurface), m_pAllocator(pAllocator)
{
  D3DSURFACE_DESC d3dsd;
  ZeroMemory(&d3dsd, sizeof(d3dsd));
  if(SUCCEEDED(m_pSurface->GetDesc(&d3dsd)))
  {
    m_maxsize.SetSize(d3dsd.Width, d3dsd.Height);
    m_rcDirty.SetRect(0, 0, d3dsd.Width, d3dsd.Height);
  }
}

CDX9SubPic::~CDX9SubPic()
{
  {
    CAutoLock Lock(&CDX9SubPicAllocator::ms_SurfaceQueueLock);
    // Add surface to cache
    if (m_pAllocator)
    {
      for (std::list<CDX9SubPic *>::iterator pos = m_pAllocator->m_AllocatedSurfaces.begin(); pos != m_pAllocator->m_AllocatedSurfaces.end(); )
      {
        std::list<CDX9SubPic *>::iterator ThisPos = pos;
        CDX9SubPic *pSubPic = *pos; pos++;
        if (pSubPic == this)
        {
          m_pAllocator->m_AllocatedSurfaces.erase(ThisPos);
          break;
        }
      }
      m_pSurface.m_ptr->AddRef();
      m_pAllocator->m_FreeSurfaces.push_back(m_pSurface);
    }
  }
}


// ISubPic

STDMETHODIMP_(void*) CDX9SubPic::GetObject()
{
  Com::SmartPtr<IDirect3DTexture9> pTexture;
  if(SUCCEEDED(m_pSurface->GetContainer(IID_IDirect3DTexture9, (void**)&pTexture)))
    return (void*)(IDirect3DTexture9*)pTexture;

  return NULL;
}

STDMETHODIMP CDX9SubPic::GetDesc(SubPicDesc& spd)
{
  D3DSURFACE_DESC d3dsd;
  ZeroMemory(&d3dsd, sizeof(d3dsd));
  if(FAILED(m_pSurface->GetDesc(&d3dsd)))
    return E_FAIL;

  spd.type = 0;
  spd.w = m_size.cx;
  spd.h = m_size.cy;
  spd.bpp = 
  d3dsd.Format == D3DFMT_A8R8G8B8 ? 32 : 
  d3dsd.Format == D3DFMT_A4R4G4B4 ? 16 : 0;
  spd.pitch = 0;
  spd.bits = NULL;
  spd.vidrect = m_vidrect;

  return S_OK;
}

STDMETHODIMP CDX9SubPic::CopyTo(ISubPic* pSubPic)
{
  HRESULT hr;
  if(FAILED(hr = __super::CopyTo(pSubPic)))
    return hr;

  if(m_rcDirty.IsRectEmpty())
    return S_FALSE;

  Com::SmartPtr<IDirect3DDevice9> pD3DDev;
  if(!m_pSurface || FAILED(m_pSurface->GetDevice(&pD3DDev)) || !pD3DDev)
    return E_FAIL;

  IDirect3DTexture9* pSrcTex = (IDirect3DTexture9*)GetObject();
  Com::SmartPtr<IDirect3DSurface9> pSrcSurf;
  pSrcTex->GetSurfaceLevel(0, &pSrcSurf);
  D3DSURFACE_DESC srcDesc;
  pSrcSurf->GetDesc(&srcDesc);

  IDirect3DTexture9* pDstTex = (IDirect3DTexture9*)pSubPic->GetObject();
  Com::SmartPtr<IDirect3DSurface9> pDstSurf;
  pDstTex->GetSurfaceLevel(0, &pDstSurf);
  D3DSURFACE_DESC dstDesc;
  pDstSurf->GetDesc(&dstDesc);

  RECT r;
  SetRect(&r, 0, 0, min(srcDesc.Width, dstDesc.Width), min(srcDesc.Height, dstDesc.Height));
  POINT p = { 0, 0 };
  hr = pD3DDev->UpdateSurface(pSrcSurf, &r, pDstSurf, &p);

  return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CDX9SubPic::ClearDirtyRect(DWORD color)
{
  if(m_rcDirty.IsRectEmpty())
    return S_FALSE;

  Com::SmartPtr<IDirect3DDevice9> pD3DDev;
  if(!m_pSurface || FAILED(m_pSurface->GetDevice(&pD3DDev)) || !pD3DDev)
    return E_FAIL;

  SubPicDesc spd;
  if(SUCCEEDED(Lock(spd)))
  {
    int h = m_rcDirty.Height();

    BYTE* ptr = (BYTE*)spd.bits + spd.pitch*m_rcDirty.top + (m_rcDirty.left*spd.bpp>>3);

    if(spd.bpp == 16)
    {
      while(h-- > 0)
      {
        memsetw(ptr, color, 2 * m_rcDirty.Width());
        ptr += spd.pitch;
      }
    }
    else if(spd.bpp == 32)
    {
      while(h-- > 0)
      {
        memsetd(ptr, color, 4 * m_rcDirty.Width());
        ptr += spd.pitch;
      }
    }
/*
    DWORD* ptr = (DWORD*)bm.bits;
    DWORD* end = ptr + bm.h*bm.wBytes/4;
    while(ptr < end) *ptr++ = color;
*/
    Unlock(NULL);
  }

//    HRESULT hr = pD3DDev->ColorFill(m_pSurface, m_rcDirty, color);
  
  m_rcDirty.SetRectEmpty();

  return S_OK;
}

STDMETHODIMP CDX9SubPic::Lock(SubPicDesc& spd)
{

  D3DSURFACE_DESC d3dsd;
  ZeroMemory(&d3dsd, sizeof(d3dsd));
  if(FAILED(m_pSurface->GetDesc(&d3dsd)))
    return E_FAIL;

  D3DLOCKED_RECT LockedRect;
  ZeroMemory(&LockedRect, sizeof(LockedRect));
  if(FAILED(m_pSurface->LockRect(&LockedRect, NULL, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_NOSYSLOCK)))
    return E_FAIL;

  spd.type = 0;
  spd.w = m_size.cx;
  spd.h = m_size.cy;
  spd.bpp = 
    d3dsd.Format == D3DFMT_A8R8G8B8 ? 32 : 
    d3dsd.Format == D3DFMT_A4R4G4B4 ? 16 : 0;
  spd.pitch = LockedRect.Pitch;
  spd.bits = LockedRect.pBits;
  spd.vidrect = m_vidrect;

  return S_OK;
}

STDMETHODIMP CDX9SubPic::Unlock(RECT* pDirtyRect)
{
  m_pSurface->UnlockRect();

  if(pDirtyRect)
  {
    m_rcDirty = *pDirtyRect;
    if (!((Com::SmartRect*)pDirtyRect)->IsRectEmpty())
    {
      m_rcDirty.InflateRect(1, 1);
      m_rcDirty.left &= ~127;
      m_rcDirty.top &= ~63;
      m_rcDirty.right = (m_rcDirty.right + 127) & ~127;
      m_rcDirty.bottom = (m_rcDirty.bottom + 63) & ~63;
      m_rcDirty &= Com::SmartRect(Com::SmartPoint(0, 0), m_size);
    }
  }
  else
  {
    m_rcDirty = Com::SmartRect(Com::SmartPoint(0, 0), m_size);
  }

  Com::SmartPtr<IDirect3DTexture9> pTexture = (IDirect3DTexture9*)GetObject();
  if (pTexture && !((Com::SmartRect*)pDirtyRect)->IsRectEmpty())
    pTexture->AddDirtyRect(&m_rcDirty);

  return S_OK;
}

STDMETHODIMP CDX9SubPic::AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget)
{
  ASSERT(pTarget == NULL);

  if(!pSrc || !pDst)
    return E_POINTER;

  Com::SmartRect src(*pSrc), dst(*pDst);

  Com::SmartPtr<IDirect3DDevice9> pD3DDev;
  Com::SmartPtr<IDirect3DTexture9> pTexture = (IDirect3DTexture9*)GetObject();
  if(!pTexture || FAILED(pTexture->GetDevice(&pD3DDev)) || !pD3DDev)
    return E_NOINTERFACE;

  HRESULT hr;

  do
  {
    D3DSURFACE_DESC d3dsd;
    ZeroMemory(&d3dsd, sizeof(d3dsd));
    if(FAILED(pTexture->GetLevelDesc(0, &d3dsd)) /*|| d3dsd.Type != D3DRTYPE_TEXTURE*/)
      break;

        float w = (float)d3dsd.Width;
        float h = (float)d3dsd.Height;

    struct
    {
      float x, y, z, rhw;
      float tu, tv;
    }
    pVertices[] =
    {
      {(float)dst.left, (float)dst.top, 0.5f, 2.0f, (float)src.left / w, (float)src.top / h},
      {(float)dst.right, (float)dst.top, 0.5f, 2.0f, (float)src.right / w, (float)src.top / h},
      {(float)dst.left, (float)dst.bottom, 0.5f, 2.0f, (float)src.left / w, (float)src.bottom / h},
      {(float)dst.right, (float)dst.bottom, 0.5f, 2.0f, (float)src.right / w, (float)src.bottom / h},
    };
/*
	for(ptrdiff_t i = 0; i < countof(pVertices); i++)
    {
      pVertices[i].x -= 0.5;
      pVertices[i].y -= 0.5;
    }
*/

    hr = pD3DDev->SetTexture(0, pTexture);

    DWORD abe, sb, db;
    hr = pD3DDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &abe);
    hr = pD3DDev->GetRenderState(D3DRS_SRCBLEND, &sb);
    hr = pD3DDev->GetRenderState(D3DRS_DESTBLEND, &db);

    hr = pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    hr = pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
    hr = pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
    hr = pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    hr = pD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // pre-multiplied src and ...
    hr = pD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA); // ... inverse alpha channel for dst

    hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    hr = pD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    hr = pD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    hr = pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    hr = pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    hr = pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    hr = pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    hr = pD3DDev->SetPixelShader(NULL);

    hr = pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    hr = pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pVertices, sizeof(pVertices[0]));

    pD3DDev->SetTexture(0, NULL);

    pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, abe);
    pD3DDev->SetRenderState(D3DRS_SRCBLEND, sb);
    pD3DDev->SetRenderState(D3DRS_DESTBLEND, db);

    return S_OK;
  } while(0);

  return E_FAIL;
}

STDMETHODIMP CDX9SubPic::GetTexture( Com::SmartPtr<IDirect3DTexture9>& pTexture )
{
  pTexture = (IDirect3DTexture9*)GetObject();
  if(! pTexture)
    return E_NOINTERFACE;
  
  return S_OK;
}
//
// CDX9SubPicAllocator
//

CDX9SubPicAllocator::CDX9SubPicAllocator(IDirect3DDevice9* pD3DDev, SIZE maxsize, bool fPow2Textures) 
  : ISubPicAllocatorImpl(maxsize, true, fPow2Textures)
  , m_pD3DDev(pD3DDev), m_maxsize(maxsize)
{
}

CCritSec CDX9SubPicAllocator::ms_SurfaceQueueLock;


CDX9SubPicAllocator::~CDX9SubPicAllocator()
{
  FreeTextures();
}


void CDX9SubPicAllocator::GetStats(int &_nFree, int &_nAlloc)
{
  CAutoLock Lock(&ms_SurfaceQueueLock);
  _nFree = m_FreeSurfaces.size();
  _nAlloc = m_AllocatedSurfaces.size();
}

void CDX9SubPicAllocator::FreeTextures()
{
  // Clear the allocator of any remaining subpics
  CAutoLock Lock(&ms_SurfaceQueueLock);
  for (std::list<CDX9SubPic *>::iterator pos = m_AllocatedSurfaces.begin(); pos != m_AllocatedSurfaces.end(); )
  {
    CDX9SubPic *pSubPic = *pos; pos++;
    pSubPic->m_pAllocator = NULL;
    delete pSubPic;
  }
  m_AllocatedSurfaces.clear();

  for (std::list<Com::SmartPtrForList<IDirect3DSurface9>>::iterator it = m_FreeSurfaces.begin();
    it != m_FreeSurfaces.end(); it++)
    it->FullRelease();

  m_FreeSurfaces.clear();
}

void CDX9SubPicAllocator::ClearCache()
{
  {
    // Clear the allocator of any remaining subpics
    CAutoLock Lock(&ms_SurfaceQueueLock);
    for (std::list<CDX9SubPic *>::iterator pos = m_AllocatedSurfaces.begin(); pos != m_AllocatedSurfaces.end(); )
    {
      CDX9SubPic *pSubPic = *pos; pos++;
      pSubPic->m_pAllocator = NULL;
    }
    m_AllocatedSurfaces.clear();
    m_FreeSurfaces.clear();
    
    FreeTextures();
    m_pStatic = NULL; 
  }
}

// ISubPicAllocator

STDMETHODIMP CDX9SubPicAllocator::ChangeDevice(IUnknown* pDev)
{
  ClearCache();
  Com::SmartQIPtr<IDirect3DDevice9> pD3DDev = pDev;
  if(!pD3DDev) return E_NOINTERFACE;

  CAutoLock cAutoLock(this);
  m_pD3DDev = pD3DDev;

  return __super::ChangeDevice(pDev);
}

STDMETHODIMP CDX9SubPicAllocator::SetMaxTextureSize(SIZE MaxTextureSize)
{
  if ((m_maxsize == MaxTextureSize)
    && (m_cursize == MaxTextureSize))
    return S_OK;

  ClearCache();
  m_maxsize  = MaxTextureSize;
  SetCurSize(MaxTextureSize);
  return S_OK;
}

// ISubPicAllocatorImpl

bool CDX9SubPicAllocator::Alloc(bool fStatic, ISubPic** ppSubPic)
{
  if(!ppSubPic) 
    return(false);

  CAutoLock cAutoLock(this);

  *ppSubPic = NULL;

  Com::SmartPtr<IDirect3DSurface9> pSurface;

  int Width = m_maxsize.cx;
  int Height = m_maxsize.cy;

  if(m_fPow2Textures)
  {
    Width = Height = 1;
    while(Width < m_maxsize.cx) Width <<= 1;
    while(Height < m_maxsize.cy) Height <<= 1;
  }
  if (!fStatic)
  {
    CAutoLock cAutoLock(&ms_SurfaceQueueLock);
    std::list<Com::SmartPtrForList<IDirect3DSurface9>>::iterator FreeSurf = m_FreeSurfaces.begin();
    if (FreeSurf != m_FreeSurfaces.end())
    {
      pSurface = m_FreeSurfaces.front();
      m_FreeSurfaces.pop_front();
    }
  }

  if (!pSurface)
  {
    Com::SmartPtr<IDirect3DTexture9> pTexture;
    if(FAILED(m_pD3DDev->CreateTexture(Width, Height, 1, 0, D3DFMT_A8R8G8B8, fStatic?D3DPOOL_SYSTEMMEM:D3DPOOL_DEFAULT, &pTexture, NULL)))
      return(false);

    if(FAILED(pTexture->GetSurfaceLevel(0, &pSurface)))
      return(false);
  }

  if(!(*ppSubPic = DNew CDX9SubPic(pSurface, fStatic ? 0 : this)))
    return(false);

  (*ppSubPic)->AddRef();

  if (!fStatic)
  {
    CAutoLock cAutoLock(&ms_SurfaceQueueLock);
    m_AllocatedSurfaces.push_front((CDX9SubPic *)*ppSubPic);
  }

  return(true);
}
