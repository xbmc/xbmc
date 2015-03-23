/*
 * $Id$
 *
 * (C) 2006-2011 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "AllocatorCommon.h"
#include "mvrInterfaces.h"
#include "IPaintCallback.h"
#include "utils/log.h"

class CmadVRAllocatorPresenter
  : public ISubPicAllocatorPresenterImpl,
  public IPaintCallbackMadvr
{

  class COsdRenderCallback : public CUnknown, public IOsdRenderCallback, public CCritSec
  {
    CmadVRAllocatorPresenter* m_pDXRAP;

  public:
    COsdRenderCallback(CmadVRAllocatorPresenter* pDXRAP)
      : CUnknown(_T("COsdRender"), NULL)
      , m_pDXRAP(pDXRAP) {
    }

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
      return
        QI(IOsdRenderCallback)
        __super::NonDelegatingQueryInterface(riid, ppv);
    }

    void SetDXRAP(CmadVRAllocatorPresenter* pDXRAP) {
      CAutoLock cAutoLock(this);
      m_pDXRAP = pDXRAP;
    }

    // IOsdRenderCallback

    STDMETHODIMP ClearBackground(LPCSTR name, REFERENCE_TIME frameStart, RECT *fullOutputRect, RECT *activeVideoRect){
      CAutoLock cAutoLock(this);
      return m_pDXRAP ? m_pDXRAP->ClearBackground(name, frameStart, fullOutputRect, activeVideoRect) : E_UNEXPECTED;
    }
    STDMETHODIMP RenderOsd(LPCSTR name, REFERENCE_TIME frameStart, RECT *fullOutputRect, RECT *activeVideoRect){
      CAutoLock cAutoLock(this);
      return m_pDXRAP ? m_pDXRAP->RenderOsd(name, frameStart, fullOutputRect, activeVideoRect) : E_UNEXPECTED;
    }
    STDMETHODIMP SetDevice(IDirect3DDevice9* pD3DDev) {
      CAutoLock cAutoLock(this);
      return m_pDXRAP ? m_pDXRAP->SetDevice(pD3DDev) : E_UNEXPECTED;
    }
  };

  class CSubRenderCallback : public CUnknown, public ISubRenderCallback2, public CCritSec
  {
    CmadVRAllocatorPresenter* m_pDXRAP;

  public:
    CSubRenderCallback(CmadVRAllocatorPresenter* pDXRAP)
      : CUnknown(_T("CSubRender"), NULL)
      , m_pDXRAP(pDXRAP) {
    }

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) {
      return
        QI(ISubRenderCallback)
        QI(ISubRenderCallback2)
        __super::NonDelegatingQueryInterface(riid, ppv);
    }

    void SetDXRAP(CmadVRAllocatorPresenter* pDXRAP) {
      CAutoLock cAutoLock(this);
      m_pDXRAP = pDXRAP;
    }

    // ISubRenderCallback

    STDMETHODIMP SetDevice(IDirect3DDevice9* pD3DDev) {
      CAutoLock cAutoLock(this);
      return m_pDXRAP ? m_pDXRAP->SetSubDevice(pD3DDev) : E_UNEXPECTED;
    }

    STDMETHODIMP Render(REFERENCE_TIME rtStart, int left, int top, int right, int bottom, int width, int height) {
      CAutoLock cAutoLock(this);
      return m_pDXRAP ? m_pDXRAP->Render(rtStart, 0, 0, left, top, right, bottom, width, height) : E_UNEXPECTED;
    }

    // ISubRendererCallback2

    STDMETHODIMP RenderEx(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME AvgTimePerFrame, int left, int top, int right, int bottom, int width, int height) {
      CAutoLock cAutoLock(this);
      return m_pDXRAP ? m_pDXRAP->Render(rtStart, rtStop, AvgTimePerFrame, left, top, right, bottom, width, height) : E_UNEXPECTED;
    }
  };
  Com::SmartPtr<IUnknown> m_pDXR;
  LPDIRECT3DDEVICE9 m_pD3DDevice;
  LPDIRECT3DDEVICE9 m_pD3DDeviceMadVR;
  Com::SmartPtr<ISubRenderCallback2> m_pSRCB;
  Com::SmartPtr<IOsdRenderCallback> m_pORCB;
  Com::SmartSize m_ScreenSize;
  bool  m_bIsFullscreen;
  bool m_isDeviceSet;
  bool TestRender(IDirect3DDevice9* pD3DDevice);
  CEvent m_drawIsDone;

  LPDIRECT3DVERTEXBUFFER9 m_pVB; // Buffer to hold Vertices
public:

  CmadVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString &_Error);
  virtual ~CmadVRAllocatorPresenter();

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // IOsdRenderCallback
  STDMETHODIMP ClearBackground(LPCSTR name, REFERENCE_TIME frameStart, RECT *fullOutputRect, RECT *activeVideoRect);
  STDMETHODIMP RenderOsd(LPCSTR name, REFERENCE_TIME frameStart, RECT *fullOutputRect, RECT *activeVideoRect);
  STDMETHODIMP SetDevice(IDirect3DDevice9* pD3DDev);

  // ISubPicAllocatorPresenter
  HRESULT SetSubDevice(IDirect3DDevice9* pD3DDev);
  HRESULT Render(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf, int left, int top, int bottom, int right, int width, int height);
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
  STDMETHODIMP_(void) SetPosition(RECT w, RECT v);
  STDMETHODIMP_(SIZE) GetVideoSize(bool fCorrectAR);
  STDMETHODIMP_(bool) Paint(bool fAll);
  STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
  STDMETHODIMP SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget);

  //IPaintCallbackMadvr

  LPDIRECT3DDEVICE9 GetDevice() { return m_isDeviceSet ? m_pD3DDeviceMadVR : m_pD3DDevice; }
  virtual void OsdRedrawFrame();
  virtual void SetDrawIsDone();
  virtual void SetMadvrPoisition(CRect wndRect, CRect videoRect);
  virtual void CloseMadvr();
  virtual void SettingSetScaling(CStdStringW path, int scaling);
  virtual void SettingSetDoubling(CStdStringW path, int iValue);
  virtual void SettingSetDoublingCondition(CStdStringW path, int condition);
  virtual void SettingSetQuadrupleCondition(CStdStringW path, int condition);
  virtual void SettingSetBool(CStdStringW path, BOOL bValue);

  virtual void SettingGetDoubling(CStdStringW path, int &iValue);
  virtual void SettingGetDoublingCondition(CStdStringW path, int &condition);
  virtual void SettingGetQuadrupleCondition(CStdStringW path, int &condition);
};
