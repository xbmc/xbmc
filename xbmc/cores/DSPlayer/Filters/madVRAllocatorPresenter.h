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
  void SetResolution(float fps);
  Com::SmartPtr<IUnknown> m_pDXR;
  LPDIRECT3DDEVICE9 m_pD3DDeviceMadVR;
  Com::SmartPtr<ISubRenderCallback2> m_pSRCB;
  Com::SmartSize m_ScreenSize;
  bool  m_bIsFullscreen;
  bool m_isDeviceSet;
  bool m_firstBoot;
  int m_shaderStage;

public:

  CmadVRAllocatorPresenter(HWND hWnd, HRESULT& hr, CStdString &_Error);
  virtual ~CmadVRAllocatorPresenter();

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
  LPDIRECT3DDEVICE9 GetDevice();
  virtual bool IsDeviceSet(){ return m_isDeviceSet; }
  virtual void SetIsDevice(bool b){ m_isDeviceSet = b; };
  virtual void OsdRedrawFrame();
  virtual void SetMadvrPixelShader();
  virtual void RestoreMadvrSettings();
  virtual void SetMadvrPoisition(CRect wndRect, CRect videoRect);
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

