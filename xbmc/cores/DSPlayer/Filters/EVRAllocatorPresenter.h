#pragma once
#include "dshowutil/dshowutil.h"
#include "dxva2api.h"
#include "evr.h"
#include "subpic/isubpic.h"

DEFINE_GUID(CLSID_EVRAllocatorPresenter, 
0x7612b889, 0xe070, 0x4bcc, 0xb8, 0x8, 0x91, 0xcb, 0x79, 0x41, 0x74, 0xab);
class CEVRAllocatorPresenter : public ISubPicAllocatorPresenterImpl, 
                               public IDirect3DDeviceManager9,
                               public IMFVideoPresenter,
                               public IMFGetService,
                               public IMFTopologyServiceLookupClient,
                               public IMFAsyncCallback,
                               public IMFVideoDisplayControl,
                               public IEVRTrustedVideoPlugin

{
public:
  CEVRAllocatorPresenter(HRESULT& hr, HWND wnd, CStdString &_Error,IDirect3D9* d3d = NULL, IDirect3DDevice9* d3dd = NULL);
  virtual ~CEVRAllocatorPresenter();

  STDMETHODIMP CreateRenderer(IUnknown** ppRenderer);
// IMFVideoPresenter
  STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam);
  STDMETHODIMP GetCurrentMediaType(__deref_out  IMFVideoMediaType **ppMediaType);





private:
  HANDLE m_hEvtQuit;			// Stop rendering thread event
  bool m_bEvtQuit;
  HANDLE m_hEvtFlush;		// Discard all buffers


  bool m_bEvtFlush;

  bool m_bPendingRenegotiate;
  bool m_bPendingMediaFinished;
  void CompleteFrameStep(bool bCancel);
  void ResetStats();
};

