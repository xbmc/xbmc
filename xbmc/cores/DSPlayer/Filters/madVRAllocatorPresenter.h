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
#include "MadvrCallback.h"
#include "utils/log.h"

class CmadVRAllocatorPresenter
  : public ISubPicAllocatorPresenterImpl,
  public IPaintCallbackMadvr
{

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
      return m_pDXRAP ? m_pDXRAP->SetDevice(pD3DDev) : E_UNEXPECTED;
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

  void ConfigureMadvr();
  Com::SmartPtr<IUnknown> m_pDXR;
  Com::SmartPtr<ISubRenderCallback2> m_pSRCB;
  Com::SmartSize m_ScreenSize;
  EXCLUSIVEMODECALLBACK m_exclusiveCallback;
  bool m_bIsFullscreen;
  bool m_firstBoot;
  bool m_isEnteringExclusive;
  int m_shaderStage;

  HRESULT RenderToTexture(IDirect3DTexture9* pTexture, IDirect3DSurface9* pSurface);
  HRESULT RenderTexture(IDirect3DVertexBuffer9* pVertexBuf, IDirect3DTexture9* pTexture);

  HRESULT SetupOSDVertex(IDirect3DVertexBuffer9* pVertextBuf);
  HRESULT StoreMadDeviceState();
  HRESULT SetupMadDeviceState();
  HRESULT RestoreMadDeviceState();

  IDirect3DSurface9 *m_pKodiSurface = nullptr;
  IDirect3DTexture9 *m_pKodiTexture = nullptr;
  IDirect3DSurface9 *m_pMadvrSurface = nullptr;
  IDirect3DTexture9 *m_pMadvrTexture = nullptr;
  IDirect3DVertexBuffer9* m_pMadvrVertexBuffer = nullptr;
  HANDLE m_pSharedHandle = nullptr;

  IDirect3DDevice9Ex* m_pD3DDeviceKodi = nullptr;
  IDirect3DDevice9Ex* m_pD3DDeviceMadVR = nullptr;

  DWORD m_dwWidth = 0;
  DWORD m_dwHeight = 0;

  // stored mad device state
  IDirect3DVertexShader9* m_pOldVS = nullptr;
  IDirect3DVertexBuffer9* m_pOldStreamData = nullptr;
  IDirect3DBaseTexture9* m_pOldTexture = nullptr;

  DWORD m_dwOldFVF = 0;
  DWORD m_dwOldALPHABLENDENABLE = 0;
  DWORD m_dwOldSRCALPHA = 0;
  DWORD m_dwOldINVSRCALPHA = 0;
  UINT  m_nOldOffsetInBytes = 0;
  UINT  m_nOldStride = 0;
  RECT  m_oldScissorRect;

  DWORD mD3DRS_CULLMODE = 0;
  DWORD mD3DRS_LIGHTING = 0;
  DWORD mD3DRS_ZENABLE = 0;
  DWORD mD3DRS_ALPHABLENDENABLE = 0;
  DWORD mD3DRS_SRCBLEND = 0;
  DWORD mD3DRS_DESTBLEND = 0;

  IDirect3DPixelShader9* mPix = nullptr;

public:

  CmadVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString &_Error);
  virtual ~CmadVRAllocatorPresenter();

  static void __stdcall ExclusiveCallback(LPVOID context, int event);

  DECLARE_IUNKNOWN
  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

  // ISubPicAllocatorPresenter
  HRESULT SetDevice(IDirect3DDevice9* pD3DDev);
  HRESULT Render(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME atpf, int left, int top, int bottom, int right, int width, int height);
  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
  STDMETHODIMP_(void) SetPosition(RECT w, RECT v);
  STDMETHODIMP_(SIZE) GetVideoSize(bool fCorrectAR);
  STDMETHODIMP_(bool) Paint(bool fAll);
  STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size);
  STDMETHODIMP SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget);

  //IPaintCallbackMadvr
  virtual bool IsEnteringExclusive(){ return m_isEnteringExclusive; }
  virtual void OsdRedrawFrame();
  virtual void SetMadvrPixelShader();
  virtual void RestoreMadvrSettings();
  virtual void SetResolution();
  virtual bool ParentWindowProc(HWND hWnd, UINT uMsg, WPARAM *wParam, LPARAM *lParam, LRESULT *ret);
  virtual void SetMadvrPosition(CRect wndRect, CRect videoRect);
  virtual void SettingSetScaling(CStdStringW path, int scaling);
  virtual void SettingSetDoubling(CStdStringW path, int iValue);
  virtual void SettingSetDoublingCondition(CStdStringW path, int condition);
  virtual void SettingSetQuadrupleCondition(CStdStringW path, int condition);
  virtual void SettingSetDeintActive(CStdStringW path, int iValue);
  virtual void SettingSetDeintForce(CStdStringW path, int iValue);
  virtual void SettingSetSmoothmotion(CStdStringW path, int iValue);
  virtual void SettingSetDithering(CStdStringW path, int iValue);
  virtual void SettingSetBool(CStdStringW path, BOOL bValue);
  virtual void SettingSetInt(CStdStringW path, int iValue);
  virtual CStdString GetDXVADecoderDescription();
};

