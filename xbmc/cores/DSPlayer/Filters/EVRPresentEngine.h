/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#pragma once

//-----------------------------------------------------------------------------
// D3DPresentEngine class
//
// This class creates the Direct3D device, allocates Direct3D surfaces for
// rendering, and presents the surfaces. This class also owns the Direct3D
// device manager and provides the IDirect3DDeviceManager9 interface via
// GetService.
//
// The goal of this class is to isolate the EVRCustomPresenter class from
// the details of Direct3D as much as possible.
//-----------------------------------------------------------------------------
#include "ievrpresenter.h"
#include "EvrScheduler.h"
#include "EvrHelper.h"
#include "dxva2api.h"
#include "D3DResource.h"
typedef HRESULT (__stdcall *PTR_MFCreateVideoSampleFromSurface)(IUnknown* pUnkSurface, IMFSample** ppSample);
typedef HRESULT (__stdcall *PTR_DXVA2CreateDirect3DDeviceManager9)(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);

class CEVRAllocatorPresenter;

class D3DPresentEngine :
  public SchedulerCallback,
  public ID3DResource
{
public:
  Com::SmartPtr<IDirect3DDeviceManager9> m_pDeviceManager;        // Direct3D device manager.

    // State of the Direct3D device.
  enum DeviceState
  {
    DeviceOK,
    DeviceReset,    // The device was reset OR re-created.
    DeviceRemoved,  // The device was removed.
  };

  D3DPresentEngine(CEVRAllocatorPresenter *presenter, HRESULT& hr);
  virtual ~D3DPresentEngine();

  // GetService: Returns the IDirect3DDeviceManager9 interface.
  // (The signature is identical to IMFGetService::GetService but 
  // this object does not derive from IUnknown.)
  virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv);
  virtual HRESULT CheckFormat(D3DFORMAT format);

  // ID3DResource
  void OnDestroyDevice();
  void OnCreateDevice();
  void OnLostDevice();
  void OnResetDevice();


  // Video window / destination rectangle:
  // This object implements a sub-set of the functions defined by the 
  // IMFVideoDisplayControl interface. However, some of the method signatures 
  // are different. The presenter's implementation of IMFVideoDisplayControl 
  // calls these methods.
  HRESULT SetVideoWindow(HWND hwnd);
  //HWND    GetVideoWindow() const { return m_hwnd; }
  HRESULT SetDestinationRect(const RECT& rcDest);
  RECT    GetDestinationRect() const { return m_rcDestRect; };

  HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue);
  void    ReleaseResources();

    
  HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget); 

  UINT    RefreshRate() const { return m_DisplayMode.RefreshRate; }

  HRESULT RegisterCallback(IEVRPresenterCallback *pCallback);

  HRESULT SetBufferCount(int bufferCount);
  int GetVideoWidth(){return m_iVideoWidth;};
  int GetVideoHeight(){return m_iVideoHeight;};

private:
  PTR_MFCreateVideoSampleFromSurface    pfMFCreateVideoSampleFromSurface;
  PTR_DXVA2CreateDirect3DDeviceManager9  pfDXVA2CreateDirect3DDeviceManager9;
  HMODULE  pDXVA2HLib;
  HMODULE  pEVRHLib;
protected:
  HRESULT InitializeDXVA();
  HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);
  HRESULT CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample,int surfaceIndex);

  // A derived class can override these handlers to allocate any additional D3D resources.
  virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS& pp) { return S_OK; }
  virtual void  OnReleaseResources() { }

protected:
  
  Com::SmartPtr<IEVRPresenterCallback>  m_pCallback;
  UINT                     m_DeviceResetToken;     // Reset token for the D3D device manager.
  int                      m_bufferCount;
  UINT32                   m_iVideoHeight;
  UINT32                   m_iVideoWidth;
  RECT                     m_rcDestRect;           // Destination rectangle.
  D3DDISPLAYMODE           m_DisplayMode;          // Adapter's display mode.

  CEVRAllocatorPresenter     * m_pAllocatorPresenter;
  bool                         m_bExiting;
  CCritSec                     m_ObjectLock;           // Thread lock for the D3D device.

    // COM interfaces
  bool                         m_bNeedNewDevice;
  
  Com::SmartPtr<IDirect3DTexture9>  m_pEVRVideoTexture;
  Com::SmartPtr<IDirect3DSurface9>  m_pEVRVideoSurface;
  Com::SmartPtr<IDirect3DTexture9>  m_pEVRInternalVideoTexture[7];
  Com::SmartPtr<IDirect3DSurface9>  m_pEVRInternalVideoSurface[7];

};