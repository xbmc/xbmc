/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "DX9AllocatorPresenter.h"


  class CVMR9AllocatorPresenter
    : public CDX9AllocatorPresenter
    , public IVMRSurfaceAllocator9
    , public IVMRImagePresenter9
    , public IVMRWindowlessControl9
  {
  protected:
    Com::SmartPtr<IVMRSurfaceAllocatorNotify9>          m_pIVMRSurfAllocNotify;
    std::vector<Com::SmartQIPtr<IDirect3DSurface9>>     m_pSurfaces;
    bool                                                m_fUseInternalTimer;
    REFERENCE_TIME                                      m_rtPrevStart;

    HRESULT     CreateDevice(CStdString &_Error);
    void        DeleteSurfaces();

  public:
    CVMR9AllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString &_Error);

    DECLARE_IUNKNOWN
    STDMETHODIMP                    NonDelegatingQueryInterface(REFIID riid, void** ppv);

    // ISubPicAllocatorPresenter
    STDMETHODIMP                    CreateRenderer(IUnknown** ppRenderer);
    STDMETHODIMP_(void)             SetTime(REFERENCE_TIME rtNow);
    
    // IVMRSurfaceAllocator9
    STDMETHODIMP                    InitializeDevice(DWORD_PTR dwUserID, VMR9AllocationInfo* lpAllocInfo, DWORD* lpNumBuffers);
    STDMETHODIMP                    TerminateDevice(DWORD_PTR dwID);
    STDMETHODIMP                    GetSurface(DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags, IDirect3DSurface9** lplpSurface);
    STDMETHODIMP                    AdviseNotify(IVMRSurfaceAllocatorNotify9* lpIVMRSurfAllocNotify);

    // IVMRImagePresenter9
    STDMETHODIMP                    StartPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP                    StopPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP                    PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo* lpPresInfo);

    // IVMRWindowlessControl9
    STDMETHODIMP                    GetNativeVideoSize(LONG* lpWidth, LONG* lpHeight, LONG* lpARWidth, LONG* lpARHeight);
    STDMETHODIMP                    GetMinIdealVideoSize(LONG* lpWidth, LONG* lpHeight);
    STDMETHODIMP                    GetMaxIdealVideoSize(LONG* lpWidth, LONG* lpHeight);
    STDMETHODIMP                    SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
    STDMETHODIMP                    GetVideoPosition(LPRECT lpSRCRect, LPRECT lpDSTRect);
    STDMETHODIMP                    GetAspectRatioMode(DWORD* lpAspectRatioMode);
    STDMETHODIMP                    SetAspectRatioMode(DWORD AspectRatioMode);
    STDMETHODIMP                    SetVideoClippingWindow(HWND hwnd);
    STDMETHODIMP                    RepaintVideo(HWND hwnd, HDC hdc);
    STDMETHODIMP                    DisplayModeChanged();
    STDMETHODIMP                    GetCurrentImage(BYTE** lpDib);
    STDMETHODIMP                    SetBorderColor(COLORREF Clr);
    STDMETHODIMP                    GetBorderColor(COLORREF* lpClr);
    
    // D3D Reset
    void                            BeforeDeviceReset();
    void                            AfterDeviceReset();
  };

