#include "EVRPresentEngine.h"
#include <evr.h>
#include <d3dx9tex.h>
#include "utils/log.h"
#include "DShowUtil/dshowutil.h"
#include "WindowingFactory.h" //d3d device and d3d interface
#include "cores/VideoRenderers/RenderManager.h"

#include "application.h"
#include "IPinHook.h"
HRESULT FindAdapter(IDirect3D9 *pD3D9, HMONITOR hMonitor, UINT *puAdapterID);
const MFRatio g_DefaultFps = { 30, 1 };
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
static const GUID GUID_SURFACE_INDEX = { 0x30c8e9f6, 0x415, 0x4b81, { 0xa3, 0x15, 0x1, 0xa, 0xc6, 0xa9, 0xda, 0x19 } };
D3DPresentEngine::D3DPresentEngine(HRESULT& hr) : 
    m_DeviceResetToken(0),
    m_pD3D9(NULL),
    m_pDevice(NULL),
    m_pDeviceManager(NULL),
    m_pSurfaceRepaint(NULL),
	m_pCallback(NULL),
	m_bufferCount(4)
{
	SetRectEmpty(&m_rcDestRect);
  HMODULE    hLib;
  hLib = LoadLibrary ("dxva2.dll");
  pfDXVA2CreateDirect3DDeviceManager9  = hLib ? (PTR_DXVA2CreateDirect3DDeviceManager9) GetProcAddress (hLib, "DXVA2CreateDirect3DDeviceManager9") : NULL;
  hLib = LoadLibrary ("evr.dll");
  pfMFCreateVideoSampleFromSurface  = hLib ? (PTR_MFCreateVideoSampleFromSurface)  GetProcAddress (hLib, "MFCreateVideoSampleFromSurface") : NULL;
  if (!pfMFCreateVideoSampleFromSurface)
    CLog::Log(LOGERROR,"Could not find MFCreateVideoSampleFromSurface (evr.dll)");

    ZeroMemory(&m_DisplayMode, sizeof(m_DisplayMode));

    hr = InitializeD3D();
    
    //if (SUCCEEDED(hr))
    //   hr = CreateD3DDevice();
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

D3DPresentEngine::~D3DPresentEngine()
{
  m_pDevice = NULL;
  m_pSurfaceRepaint = NULL;
  m_pDeviceManager = NULL;
	m_pCallback = NULL;
}


//-----------------------------------------------------------------------------
// GetService
//
// Returns a service interface from the presenter engine.
// The presenter calls this method from inside it's implementation of 
// IMFGetService::GetService.
//
// Classes that derive from D3DPresentEngine can override this method to return 
// other interfaces. If you override this method, call the base method from the 
// derived class.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::GetService(REFGUID guidService, REFIID riid, void** ppv)
{
  assert(ppv != NULL);

  HRESULT hr = S_OK;

  if (riid == __uuidof(IDirect3DDeviceManager9))
  {
    if (!m_pDeviceManager)
      return MF_E_UNSUPPORTED_SERVICE;
    else
    {
      *ppv = m_pDeviceManager;
      m_pDeviceManager->AddRef();
    }
  }
  else
    return MF_E_UNSUPPORTED_SERVICE;

  return hr;
}


//-----------------------------------------------------------------------------
// CheckFormat
//
// Queries whether the D3DPresentEngine can use a specified Direct3D format.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CheckFormat(D3DFORMAT format)
{
    HRESULT hr = S_OK;

    UINT uAdapter = D3DADAPTER_DEFAULT;
    D3DDEVTYPE type = D3DDEVTYPE_HAL;

    D3DDISPLAYMODE mode;
    D3DDEVICE_CREATION_PARAMETERS params;

    if (m_pDevice)
    {
        hr = m_pDevice->GetCreationParameters(&params);
        if(FAILED(hr))
          CLog::Log(LOGERROR,"%s",__FUNCTION__);

        uAdapter = params.AdapterOrdinal;
        type = params.DeviceType;

    }
    hr = m_pD3D9->GetAdapterDisplayMode(uAdapter, &mode);
        if(FAILED(hr))
          CLog::Log(LOGERROR,"%s",__FUNCTION__);

    hr = m_pD3D9->CheckDeviceType(uAdapter, type, mode.Format, format, TRUE); 
    if(FAILED(hr))
      CLog::Log(LOGERROR,"%s",__FUNCTION__);

done:
    return hr;
}

//-----------------------------------------------------------------------------
// SetDestinationRect
// 
// Sets the region within the video window where the video is drawn.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::SetDestinationRect(const RECT& rcDest)
{
    if (EqualRect(&rcDest, &m_rcDestRect))
    {
        return S_OK; // No change.
    }

    HRESULT hr = S_OK;

    CAutoLock lock(&m_ObjectLock);

    m_rcDestRect = rcDest;

    return hr;
}

//-----------------------------------------------------------------------------
// CreateVideoSamples
// 
// Creates video samples based on a specified media type.
// 
// pFormat: Media type that describes the video format.
// videoSampleQueue: List that will contain the video samples.
//
// Note: For each video sample, the method creates a swap chain with a
// single back buffer. The video sample object holds a pointer to the swap
// chain's back buffer surface. The mixer renders to this surface, and the
// D3DPresentEngine renders the video frame by presenting the swap chain.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateVideoSamples(
    IMFMediaType *pFormat, 
    VideoSampleList& videoSampleQueue
    )
{

    if (pFormat == NULL)
    {
        return MF_E_UNEXPECTED;
    }

	HRESULT hr = S_OK;
	D3DPRESENT_PARAMETERS pp;

    IDirect3DSwapChain9 *pSwapChain = NULL;    // Swap chain
	IMFSample *pVideoSample = NULL;            // Sampl
	
    CAutoLock lock(&m_ObjectLock);

    ReleaseResources();

    // Get the swap chain parameters from the media type.
    hr = GetSwapChainPresentParameters(pFormat, &pp);
    if (FAILED(hr))
      CLog::Log(LOGERROR,"%s",__FUNCTION__);
  
  
	
  if (m_pVideoSurface)
    m_pVideoSurface = NULL;
	if (m_pVideoTexture)
    m_pVideoTexture = new CD3DTexture();
     
		
  D3DFORMAT Format;
  Format =  pp.BackBufferFormat;
  m_iVideoHeight = pp.BackBufferHeight;
  m_iVideoWidth = pp.BackBufferWidth;
  bool cbtext;
  cbtext = m_pVideoTexture->Create(m_iVideoWidth ,m_iVideoHeight ,
                          1,               /* Levels */
                          D3DUSAGE_RENDERTARGET, 
                          Format,        /* D3D_FORMAT */
                          D3DPOOL_DEFAULT);
  
  hr = m_pVideoTexture->GetSurfaceLevel(0, &m_pVideoSurface);
  
  
	// Create the video samples.
    for (int i = 0; i < m_bufferCount; i++)
    {
      hr = m_pDevice->CreateTexture(m_iVideoWidth,m_iVideoHeight,
                                            1,
                                            D3DUSAGE_RENDERTARGET,
                                            Format/*D3DFMT_X8R8G8B8*/,
                                            D3DPOOL_DEFAULT,&m_pInternalVideoTexture[i],NULL);
        

      hr  = m_pInternalVideoTexture[i]->GetSurfaceLevel(0, &m_pInternalVideoSurface[i]);
      hr = CreateD3DSample(m_pInternalVideoSurface[i], &pVideoSample,i);
      //pVideoSample->SetUINT32 (GUID_SURFACE_INDEX, i);
      //hr = pfMFCreateVideoSampleFromSurface (m_pInternalVideoSurface[i], &pVideoSample);
      
    
        // Add it to the list.
		CHECK_HR(hr = videoSampleQueue.InsertBack(pVideoSample));
    
        // Set the swap chain pointer as a custom attribute on the sample. This keeps
        // a reference count on the swap chain, so that the swap chain is kept alive
        // for the duration of the sample's lifetime.
    
        //CHECK_HR(hr = pVideoSample->SetUnknown(MFSamplePresenter_SampleSwapChain, pSwapChain));
      //CHECK_HR(hr = pMFSample->SetUINT32 (GUID_SURFACE_INDEX, i));

    	S_RELEASE(pVideoSample);
      //S_RELEASE(pSwapChain);
    }

	// Let the derived class create any additional D3D resources that it needs.
    CHECK_HR(hr = OnCreateVideoSamples(pp));

done:
    if (FAILED(hr))
    {
        ReleaseResources();
    }
		
	S_RELEASE(pSwapChain);
	S_RELEASE(pVideoSample);
    return hr;
}



//-----------------------------------------------------------------------------
// ReleaseResources
// 
// Released Direct3D resources used by this object. 
//-----------------------------------------------------------------------------

void D3DPresentEngine::ReleaseResources()
{
    // Let the derived class release any resources it created.
	OnReleaseResources();

    S_RELEASE(m_pSurfaceRepaint);
}

//-----------------------------------------------------------------------------
// PresentSample
//
// Presents a video frame.
//
// pSample:  Pointer to the sample that contains the surface to present. If 
//           this parameter is NULL, the method paints a black rectangle.
// llTarget: Target presentation time.
//
// This method is called by the scheduler and/or the presenter.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::PresentSample(IMFSample* pSample, LONGLONG llTarget)
{
    HRESULT hr = S_OK;

    IMFMediaBuffer* pBuffer = NULL;
    IDirect3DSurface9* pSurface = NULL;
    
  if (!g_renderManager.IsConfigured())
    return S_OK;
  hr = g_Windowing.GetDeviceStatus();
  CHECK_HR(hr);
  if (pSample)
  {
    CHECK_HR(hr = pSample->GetBufferByIndex(0, &pBuffer));
    IMFGetService* pServ;
    CHECK_HR(hr = pBuffer->QueryInterface(__uuidof(IMFGetService),(void**)&pServ));
    CHECK_HR(hr = pServ->GetService(MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pSurface))
    SAFE_RELEASE(pServ);
  }

  if (pSurface)
  {
    hr = m_pDevice->StretchRect(pSurface,NULL, m_pVideoSurface, NULL, D3DTEXF_NONE);    
    g_renderManager.PaintVideoTexture(m_pVideoTexture,m_pVideoSurface);
    g_application.NewFrame();
    g_application.WaitFrame(100);
  }

done:
    //S_RELEASE(pSwapChain);
    S_RELEASE(pSurface);
    S_RELEASE(pBuffer);

    if (FAILED(hr))
    {
        if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET)
        {
          CLog::Log(LOGDEBUG,"D3DDevice was lost!");
            // Ignore. We need to reset or re-create the device, but this method
            // is probably being called from the scheduler thread, which is not the
            // same thread that created the device. The Reset(Ex) method must be
            // called from the thread that created the device.

            // The presenter will detect the state when it calls CheckDeviceState() 
            // on the next sample.
            hr = S_OK;
        }
    }
    return hr;
}

//-----------------------------------------------------------------------------
// private/protected methods
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// InitializeD3D
// 
// Initializes Direct3D and the Direct3D device manager.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::InitializeD3D()
{
    HRESULT hr = S_OK;

    //assert(m_pD3D9 == NULL);
    //assert(m_pDeviceManager == NULL);

    // Create Direct3D
    //CHECK_HR(hr = Direct3DCreate9(D3D_SDK_VERSION, &m_pD3D9));
    
    // Create the device manager
    //CHECK_HR(hr = DXVA2CreateDirect3DDeviceManager9(&m_DeviceResetToken, &m_pDeviceManager));
    m_pD3D9 = g_Windowing.Get3DObject();
    m_pDevice = g_Windowing.Get3DDevice();
    
    hr = pfDXVA2CreateDirect3DDeviceManager9(&m_DeviceResetToken,&m_pDeviceManager);

  if (SUCCEEDED(hr))
  {
  CLog::Log(LOGNOTICE,"Sucess to create DXVA2CreateDirect3DDeviceManager9");
    hr = m_pDeviceManager->ResetDevice(m_pDevice, m_DeviceResetToken);
  }
  CComPtr<IDirectXVideoDecoderService>  pDecoderService;
  HANDLE              hDevice;
  hr = m_pDeviceManager->OpenDeviceHandle(&hDevice);
  hr = m_pDeviceManager->GetVideoService(hDevice,__uuidof(IDirectXVideoDecoderService), (void**)&pDecoderService);
  if (SUCCEEDED(hr))
  {
    HookDirectXVideoDecoderService (pDecoderService);
    m_pDeviceManager->CloseDeviceHandle (hDevice);
  }
done:
    return hr;
}

//-----------------------------------------------------------------------------
// CreateD3DDevice
// 
// Creates the Direct3D device.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateD3DDevice()
{
    HRESULT     hr = S_OK;
    HWND        hwnd = NULL;
    HMONITOR    hMonitor = NULL;
    UINT        uAdapterID = D3DADAPTER_DEFAULT;
    DWORD       vp = 0;

    D3DCAPS9    ddCaps;
    ZeroMemory(&ddCaps, sizeof(ddCaps));

    IDirect3DDevice9* pDevice = NULL;

    // Hold the lock because we might be discarding an exisiting device.
    CAutoLock lock(&m_ObjectLock);    

    if (!m_pD3D9 || !m_pDeviceManager)
    {
        return MF_E_NOT_INITIALIZED;
    }

    hwnd = GetDesktopWindow();

    // Note: The presenter creates additional swap chains to present the
    // video frames. Therefore, it does not use the device's implicit 
    // swap chain, so the size of the back buffer here is 1 x 1.

	D3DPRESENT_PARAMETERS pp;
	ZeroMemory(&pp, sizeof(pp));

    pp.BackBufferWidth = 1;
    pp.BackBufferHeight = 1;
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_COPY;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.hDeviceWindow = hwnd;
    pp.Flags = D3DPRESENTFLAG_VIDEO;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    // Find the monitor for this window.
    /*if (m_hwnd)
    {
        hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

        
    	CHECK_HR(hr = FindAdapter(m_pD3D9, hMonitor, &uAdapterID));
    }*/

    // Get the device caps for this adapter.
    CHECK_HR(hr = m_pD3D9->GetDeviceCaps(uAdapterID, D3DDEVTYPE_HAL, &ddCaps));

    if(ddCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    }
    else
    {
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

  if(DShowUtil::IsVistaOrAbove())
	{
		IDirect3DDevice9Ex * pDeviceEx;

		// Create the device.
		/*CHECK_HR(hr = m_pD3D9->CreateDeviceEx(uAdapterID,
											  D3DDEVTYPE_HAL,
											  pp.hDeviceWindow,
											  vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
										      &pp, 
										  	  NULL,
											  &pDeviceEx));
		pDevice = pDeviceEx;*/
	}
	else
	{
		CHECK_HR(hr = m_pD3D9->CreateDevice(uAdapterID,
											  D3DDEVTYPE_HAL,
											  pp.hDeviceWindow,
											  vp | D3DCREATE_NOWINDOWCHANGES | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
										      &pp, 
											  &pDevice));
	}

    // Get the adapter display mode.
    CHECK_HR(hr = m_pD3D9->GetAdapterDisplayMode(uAdapterID, &m_DisplayMode));

    // Reset the D3DDeviceManager with the new device 
    CHECK_HR(hr = m_pDeviceManager->ResetDevice(pDevice, m_DeviceResetToken));

    //S_RELEASE(m_pDevice);

    m_pDevice = pDevice;
    //m_pDevice->AddRef();

done:
	S_RELEASE(pDevice);
	return hr;
}


//-----------------------------------------------------------------------------
// CreateD3DSample
//
// Creates an sample object (IMFSample) to hold a Direct3D swap chain.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample,int surfaceIndex)
{
    // Caller holds the object lock.

	HRESULT hr = S_OK;
    D3DCOLOR clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);

    IMFSample* pSample = NULL;
    // Create the sample.
    CHECK_HR(hr = pfMFCreateVideoSampleFromSurface(pSurface, &pSample));
    CHECK_HR(hr = pSample->SetUINT32(GUID_SURFACE_INDEX,surfaceIndex));

    // Return the pointer to the caller.
	*ppVideoSample = pSample;
	(*ppVideoSample)->AddRef();

done:
    S_RELEASE(pSurface);
    S_RELEASE(pSample);
	return hr;
}

//-----------------------------------------------------------------------------
// PresentSwapChain
//
// Presents a swap chain that contains a video frame by doing a callback
// via the sink.
//
// pSwapChain: Pointer to the swap chain.
// pSurface: Pointer to the swap chain's back buffer surface.
//-----------------------------------------------------------------------------
HRESULT D3DPresentEngine::PresentSwapChain(IDirect3DSwapChain9* pSwapChain, IDirect3DSurface9* pSurface)
{
    HRESULT hr = S_OK;
	IDirect3DSurface9* m_pRenderSurface;
	if(!m_pRenderSurface)
	{
		D3DSURFACE_DESC desc;
		
		// Get the surface description
		pSurface->GetDesc(&desc);

		// Create a surface the same size as our sample
		hr = this->m_pDevice->CreateRenderTarget(desc.Width, 
												 desc.Height, 
												 desc.Format, 
												 desc.MultiSampleType, 
												 desc.MultiSampleQuality, 
												 true, 
												 &m_pRenderSurface, 
												 NULL);
		if(hr != S_OK)
			goto bottom;
	}

	if(m_pRenderSurface)
	{
		D3DSURFACE_DESC originalDesc;
		// Get the surface description of this sample
		pSurface->GetDesc(&originalDesc);

		D3DSURFACE_DESC renderDesc;
		// Get the surface description of the render surface
		m_pRenderSurface->GetDesc(&renderDesc);

		// Compare the descriptions to make sure they match
		if(originalDesc.Width != renderDesc.Width || 
		   originalDesc.Height != renderDesc.Height ||
		   originalDesc.Format != renderDesc.Format)
		{
			// Release the old render surface
			m_pRenderSurface = NULL;
			
			// Create a new render surface that matches the size of this surface 
			hr = this->m_pDevice->CreateRenderTarget(originalDesc.Width, 
													 originalDesc.Height, 
													 originalDesc.Format, 
													 originalDesc.MultiSampleType, 
													 originalDesc.MultiSampleQuality, 
													 true, 
													 &m_pRenderSurface, 
													 NULL);
		if(hr != S_OK)
			goto bottom;
		}
	}

	if(m_pRenderSurface)
	{
		// Copy the passed surface to our rendered surface
		hr = D3DXLoadSurfaceFromSurface(m_pRenderSurface,
										NULL,
										NULL,
										pSurface,
										NULL,
										NULL,
										D3DX_FILTER_NONE,
										0);
	}

	if(hr != S_OK)
			goto bottom;

	// Do the callback, passing the rendered surface
	if(m_pCallback)
		hr = m_pCallback->PresentSurfaceCB(m_pRenderSurface);

    if (FAILED(hr))
      CLog::Log(LOGERROR,"D3DPresentEngine::PresentSwapChain failed.");

bottom:

    return hr;
}

//-----------------------------------------------------------------------------
// GetSwapChainPresentParameters
//
// Given a media type that describes the video format, fills in the
// D3DPRESENT_PARAMETERS for creating a swap chain.
//-----------------------------------------------------------------------------

HRESULT D3DPresentEngine::GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP)
{
    // Caller holds the object lock.

    HRESULT hr = S_OK; 

    UINT32 width = 0, height = 0;
    DWORD d3dFormat = 0;

	ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));

    // Create the helper object for reading the proposed type.
    //CHECK_HR(hr = MediaTypeBuilder::Create(pType, &pTypeHelper));

    // Get some information about the video format.
    hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
    //CHECK_HR(hr = pTypeHelper->GetFrameDimensions(&width, &height));
    //CHECK_HR(hr = pTypeHelper->GetFourCC(&d3dFormat));
pPP->BackBufferWidth = width;
pPP->BackBufferHeight = height;
pPP->BackBufferFormat = D3DFMT_X8R8G8B8;

    //pPP->Windowed = TRUE;
    //pPP->SwapEffect = D3DSWAPEFFECT_COPY;
    //pPP->BackBufferFormat = (D3DFORMAT)d3dFormat;
    
    //pPP->Flags = D3DPRESENTFLAG_VIDEO;
    //pPP->PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    /*D3DDEVICE_CREATION_PARAMETERS params;
    CHECK_HR(hr = m_pDevice->GetCreationParameters(&params));
    
    if (params.DeviceType != D3DDEVTYPE_HAL)
    {
        pPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    }

done:
    S_RELEASE(pTypeHelper);*/
    return S_OK;
}

//-----------------------------------------------------------------------------
// RegisterCallback
//
// Registers a callback sink for getting the D3D surface and
// is called for every video frame that needs to be rendered
//-----------------------------------------------------------------------------
HRESULT D3DPresentEngine::RegisterCallback(IEVRPresenterCallback *pCallback)
{
	if(m_pCallback)
	{
		S_RELEASE(m_pCallback);
	}

	m_pCallback = pCallback;

	if(m_pCallback)
		m_pCallback->AddRef();

	return S_OK;
}

//-----------------------------------------------------------------------------
// SetBufferCount
//
// Sets the total number of buffers to use when the EVR
// custom presenter is running.
//-----------------------------------------------------------------------------
HRESULT D3DPresentEngine::SetBufferCount(int bufferCount)
{
	m_bufferCount = bufferCount;
	return S_OK;
}


//-----------------------------------------------------------------------------
// FindAdapter
//
// Given a handle to a monitor, returns the ordinal number that D3D uses to 
// identify the adapter.
//-----------------------------------------------------------------------------

HRESULT FindAdapter(IDirect3D9 *pD3D9, HMONITOR hMonitor, UINT *puAdapterID)
{
	HRESULT hr = E_FAIL;
	UINT cAdapters = 0;
	UINT uAdapterID = (UINT)-1;

	cAdapters = pD3D9->GetAdapterCount();
	for (UINT i = 0; i < cAdapters; i++)
	{
        HMONITOR hMonitorTmp = pD3D9->GetAdapterMonitor(i);

        if (hMonitorTmp == NULL)
        {
            break;
        }
        if (hMonitorTmp == hMonitor)
        {
            uAdapterID = i;
            break;
        }
	}

	if (uAdapterID != (UINT)-1)
	{
		*puAdapterID = uAdapterID;
		hr = S_OK;
	}
	return hr;
}
