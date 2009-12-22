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
class D3DPresentEngine : public SchedulerCallback
{
public:

    // State of the Direct3D device.
    enum DeviceState
    {
        DeviceOK,
        DeviceReset,    // The device was reset OR re-created.
        DeviceRemoved,  // The device was removed.
    };

    D3DPresentEngine(HRESULT& hr);
    virtual ~D3DPresentEngine();

    // GetService: Returns the IDirect3DDeviceManager9 interface.
    // (The signature is identical to IMFGetService::GetService but 
    // this object does not derive from IUnknown.)
    virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv);
    virtual HRESULT CheckFormat(D3DFORMAT format);
  

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
protected:
	HRESULT InitializeD3D();
    HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);
	HRESULT CreateD3DDevice();
	HRESULT CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample,int surfaceIndex);

	// A derived class can override these handlers to allocate any additional D3D resources.
	virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS& pp) { return S_OK; }
	virtual void	OnReleaseResources() { }

    virtual HRESULT PresentSwapChain(IDirect3DSwapChain9* pSwapChain, IDirect3DSurface9* pSurface);

protected:
  
	IEVRPresenterCallback		*m_pCallback;
  UINT                     m_DeviceResetToken;     // Reset token for the D3D device manager.
	int							         m_bufferCount;
  int                      m_iVideoHeight;
  int                      m_iVideoWidth;
	RECT						         m_rcDestRect;           // Destination rectangle.
  D3DDISPLAYMODE              m_DisplayMode;          // Adapter's display mode.

  CCritSec                     m_ObjectLock;           // Thread lock for the D3D device.

    // COM interfaces
  CComPtr<IDirect3D9>                     m_pD3D9;
  CComPtr<IDirect3DDevice9>               m_pDevice;
  IDirect3DDeviceManager9     *m_pDeviceManager;        // Direct3D device manager.
	
  CD3DTexture                 *m_pVideoTexture;
  IDirect3DSurface9           *m_pVideoSurface;
  IDirect3DTexture9           *m_pInternalVideoTexture[7];
  IDirect3DSurface9           *m_pInternalVideoSurface[7];

};