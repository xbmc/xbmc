/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace concurrency;

#ifdef _DEBUG
#define breakOnDebug __debugbreak()
#else
#define breakOnDebug 
#endif
#define LOG_HR(hr) CLog::LogF(LOGERROR, "function call at line %d ends with error: %s", __LINE__, DX::GetErrorDescription(hr).c_str());
#define CHECK_ERR() if (FAILED(hr)) { LOG_HR(hr); breakOnDebug; return; }
#define RETURN_ERR(ret) if (FAILED(hr)) { LOG_HR(hr); breakOnDebug; return (##ret); }

namespace DisplayMetrics
{
  // High resolution displays can require a lot of GPU and battery power to render.
  // High resolution phones, for example, may suffer from poor battery life if
  // games attempt to render at 60 frames per second at full fidelity.
  // The decision to render at full fidelity across all platforms and form factors
  // should be deliberate.
  static const bool SupportHighResolutions = false;

  // The default thresholds that define a "high resolution" display. If the thresholds
  // are exceeded and SupportHighResolutions is false, the dimensions will be scaled
  // by 50%.
  static const float Dpi100 = 96.0f;    // 100% of standard desktop display.
  static const float DpiThreshold = 192.0f;    // 200% of standard desktop display.
  static const float WidthThreshold = 1920.0f;  // 1080p width.
  static const float HeightThreshold = 1080.0f;  // 1080p height.
};

bool DX::DeviceResources::CBackBuffer::Acquire(ID3D11Texture2D* pTexture)
{
  if (!pTexture)
    return false;

  D3D11_TEXTURE2D_DESC desc;
  pTexture->GetDesc(&desc);

  m_width = desc.Width;
  m_height = desc.Height;
  m_format = desc.Format;
  m_usage = desc.Usage;

  m_texture = pTexture;
  m_texture->AddRef();
  return true;
}

std::shared_ptr<DX::DeviceResources> DX::DeviceResources::Get()
{
  static std::shared_ptr<DeviceResources> sDeviceResources(new DeviceResources);
  return sDeviceResources;
}

// Constructor for DeviceResources.
DX::DeviceResources::DeviceResources()
  : m_screenViewport()
  , m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1)
  , m_outputSize()
  , m_logicalSize()
  , m_dpi(DisplayMetrics::Dpi100)
  , m_effectiveDpi(DisplayMetrics::Dpi100)
  , m_deviceNotify(nullptr)
  , m_stereoEnabled(false)
  , m_bDeviceCreated(false)
{
}

DX::DeviceResources::~DeviceResources()
{
  if (m_bDeviceCreated)
    Release();
}

void DX::DeviceResources::Release()
{
  ReleaseBackBuffer();
  OnDeviceLost(true);

  // leave fullscreen before destroying 
  BOOL bFullScreen;
  m_swapChain->GetFullscreenState(&bFullScreen, nullptr);
  if (!!bFullScreen)
    m_swapChain->SetFullscreenState(false, nullptr);

  m_swapChain = nullptr;
  m_adapter = nullptr;
  m_dxgiFactory = nullptr;
  m_output = nullptr;
  m_deferrContext = nullptr;
  m_d3dContext = nullptr;
  m_d3dDevice = nullptr;
  m_bDeviceCreated = false;
}

void DX::DeviceResources::GetOutput(IDXGIOutput** pOutput) const
{
  m_swapChain->GetContainingOutput(pOutput);
}

void DX::DeviceResources::GetAdapterDesc(DXGI_ADAPTER_DESC* desc) const
{
  if (m_adapter)
    m_adapter->GetDesc(desc);
}

void DX::DeviceResources::GetDisplayMode(DXGI_MODE_DESC* mode) const
{
  DXGI_OUTPUT_DESC outDesc;
  ComPtr<IDXGIOutput> pOutput;

  m_swapChain->GetContainingOutput(&pOutput);
  pOutput->GetDesc(&outDesc);

  DXGI_SWAP_CHAIN_DESC scDesc;
  m_swapChain->GetDesc(&scDesc);

  memset(mode, 0, sizeof(DXGI_MODE_DESC));
  // desktop coords depend on DPI
  mode->Width = DX::ConvertDipsToPixels(outDesc.DesktopCoordinates.right - outDesc.DesktopCoordinates.left, m_dpi);
  mode->Height = DX::ConvertDipsToPixels(outDesc.DesktopCoordinates.bottom - outDesc.DesktopCoordinates.top, m_dpi);
  mode->Format = scDesc.BufferDesc.Format;
  mode->Scaling = scDesc.BufferDesc.Scaling;
  mode->ScanlineOrdering = scDesc.BufferDesc.ScanlineOrdering;

#ifdef TARGET_WINDOWS_DESKTOP
  DEVMODEW sDevMode;
  memset(&sDevMode, 0, sizeof(sDevMode));
  sDevMode.dmSize = sizeof(sDevMode);

  // EnumDisplaySettingsW is only one way to detect current refresh rate
  if (EnumDisplaySettingsW(outDesc.DeviceName, ENUM_CURRENT_SETTINGS, &sDevMode))
  {
    int i = (((sDevMode.dmDisplayFrequency + 1) % 24) == 0 || ((sDevMode.dmDisplayFrequency + 1) % 30) == 0) ? 1 : 0;
    mode->RefreshRate.Numerator = (sDevMode.dmDisplayFrequency + i) * 1000;
    mode->RefreshRate.Denominator = 1000 + i;
  }
#endif
}

ID3D11RenderTargetView* DX::DeviceResources::GetBackBufferRTV()
{
  return m_backBufferTex.GetRenderTarget();
}

void DX::DeviceResources::SetViewPort(D3D11_VIEWPORT& viewPort) const
{
  // convert logical viewport to real
  D3D11_VIEWPORT realViewPort =
  {
    viewPort.TopLeftX,
    viewPort.TopLeftY,
    ConvertDipsToPixels(viewPort.Width, m_dpi),
    ConvertDipsToPixels(viewPort.Height, m_dpi),
    viewPort.MinDepth,
    viewPort.MinDepth
  };

  m_deferrContext->RSSetViewports(1, &realViewPort);
}

bool DX::DeviceResources::SetFullScreen(bool fullscreen, RESOLUTION_INFO& res)
{
  if (!m_bDeviceCreated)
    return false;

  critical_section::scoped_lock lock(m_criticalSection);

  CLog::Log(LOGDEBUG, __FUNCTION__": switching to/from fullscreen (%f x %f)", m_outputSize.Width, m_outputSize.Height);

  BOOL bFullScreen;
  bool recreate = m_stereoEnabled != (g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED);

  m_swapChain->GetFullscreenState(&bFullScreen, nullptr);
  if (!!bFullScreen && !fullscreen)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": switching to windowed");
    recreate |= SUCCEEDED(m_swapChain->SetFullscreenState(false, nullptr));
  }
  else if (fullscreen)
  {
    DXGI_MODE_DESC currentMode;
    GetDisplayMode(&currentMode);

    if ( currentMode.Width != res.iWidth
      || currentMode.Height != res.iHeight
      || DX::RationalToFloat(currentMode.RefreshRate) != res.fRefreshRate
      // force resolution change for stereo mode
      // some drivers unable to create stereo swapchain if mode does not match @23.976
      || g_graphicsContext.GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__": changing display mode to %dx%d@%0.3f", res.iWidth, res.iHeight, res.fRefreshRate);

      int refresh = static_cast<int>(res.fRefreshRate);
      int i = (refresh + 1) % 24 == 0 || (refresh + 1) % 30 == 0 ? 1 : 0;

      currentMode.Width = res.iWidth;
      currentMode.Height = res.iHeight;
      currentMode.RefreshRate.Numerator = (refresh + i) * 1000;
      currentMode.RefreshRate.Denominator = 1000 + i;

      recreate |= SUCCEEDED(m_swapChain->ResizeTarget(&currentMode));
    }

    if (!bFullScreen)
    {
      ComPtr<IDXGIOutput> pOutput;
      m_swapChain->GetContainingOutput(&pOutput);

      CLog::Log(LOGDEBUG, __FUNCTION__": switching to fullscreen");
      recreate |= SUCCEEDED(m_swapChain->SetFullscreenState(true, pOutput.Get()));
    }
  }

  // resize backbuffer to proper hanlde fullscreen/stereo transition
  if (recreate)
    ResizeBuffers();

  CLog::Log(LOGDEBUG, __FUNCTION__": switching done.");

  return true;
}

// Configures resources that don't depend on the Direct3D device.
void DX::DeviceResources::CreateDeviceIndependentResources()
{
}

// Configures the Direct3D device, and stores handles to it and the device context.
void DX::DeviceResources::CreateDeviceResources() 
{
  CLog::LogF(LOGDEBUG, "creating DirectX 11 device.");

  UINT creationFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
#if defined(_DEBUG)
  if (DX::SdkLayersAvailable())
  {
    // If the project is in a debug build, enable debugging via SDK Layers with this flag.
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
  }
#endif

  // This array defines the set of DirectX hardware feature levels this app will support.
  // Note the ordering should be preserved.
  // Don't forget to declare your application's minimum required feature level in its
  // description.  All applications are assumed to support 9.1 unless otherwise stated.
  D3D_FEATURE_LEVEL featureLevels[] =
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1
  };

  // Create the Direct3D 11 API device object and a corresponding context.
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;

  HRESULT hr = D3D11CreateDevice(
      nullptr,                   // Specify nullptr to use the default adapter.
      D3D_DRIVER_TYPE_HARDWARE,  // Create a device using the hardware graphics driver.
      nullptr,                   // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
      creationFlags,             // Set debug and Direct2D compatibility flags.
      featureLevels,             // List of feature levels this app can support.
      ARRAYSIZE(featureLevels),  // Size of the list above.
      D3D11_SDK_VERSION,         // Always set this to D3D11_SDK_VERSION for Windows Store apps.
      &device,                   // Returns the Direct3D device created.
      &m_d3dFeatureLevel,        // Returns feature level of device created.
      &context                   // Returns the device immediate context.
    );

  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "unable to create hardware device, trying to create WARP devices then.");
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
        nullptr,
        creationFlags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &device,
        &m_d3dFeatureLevel,
        &context
    );
    if (FAILED(hr))
    {
      CLog::LogF(LOGFATAL, "unable to create WARP device. Rendering in not possible.");
      CHECK_ERR();
    }
  }

  // Store pointers to the Direct3D 11.1 API device and immediate context.
  hr = device.As(&m_d3dDevice); CHECK_ERR();

  // To enable multi-threaded access (optional)
  ComPtr<ID3D10Multithread> d3dMultiThread;
  hr = m_d3dDevice.As(&d3dMultiThread); CHECK_ERR();
  d3dMultiThread->SetMultithreadProtected(1);

  hr = context.As(&m_d3dContext); CHECK_ERR();
  hr = m_d3dDevice->CreateDeferredContext1(0, &m_deferrContext); CHECK_ERR();

  ComPtr<IDXGIDevice1> dxgiDevice;
  ComPtr<IDXGIAdapter> adapter;
  hr = m_d3dDevice.As(&dxgiDevice); CHECK_ERR();
  hr = dxgiDevice->GetAdapter(&adapter); CHECK_ERR();
  hr = adapter.As(&m_adapter); CHECK_ERR();
  hr = m_adapter->GetParent(IID_PPV_ARGS(&m_dxgiFactory)); CHECK_ERR();

  DXGI_ADAPTER_DESC aDesc;
  m_adapter->GetDesc(&aDesc);

  CLog::LogF(LOGDEBUG, "device is created on adapter '%S' with feature level %04x.", aDesc.Description, m_d3dFeatureLevel);

  m_bDeviceCreated = true;
}

void DX::DeviceResources::ReleaseBackBuffer()
{
  CLog::LogF(LOGDEBUG, "release buffers.");

  // Clear the previous window size specific context.
  ID3D11RenderTargetView* nullViews[] = { nullptr };
  m_deferrContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
  FinishCommandList(false);

  m_backBufferTex.Release();
  m_d3dDepthStencilView = nullptr;
  m_deferrContext->Flush();
  m_d3dContext->Flush();
}

void DX::DeviceResources::CreateBackBuffer()
{
  if (!m_bDeviceCreated)
    return;

  CLog::LogF(LOGDEBUG, "create buffers.");

  // Get swap chain back buffer.
  ComPtr<ID3D11Texture2D> backBuffer;
  HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)); CHECK_ERR();

  // Create back buffer texture from swap chain texture
  if (!m_backBufferTex.Acquire(backBuffer.Get()))
  {
    CLog::LogF(LOGERROR, "failed to create render target.");
    return;
  }

  // Create a depth stencil view for use with 3D rendering if needed.
  CD3D11_TEXTURE2D_DESC depthStencilDesc(
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    lround(m_outputSize.Width),
    lround(m_outputSize.Height),
    1, // This depth stencil view has only one texture.
    1, // Use a single mipmap level.
    D3D11_BIND_DEPTH_STENCIL
  );

  ComPtr<ID3D11Texture2D> depthStencil;
  hr = m_d3dDevice->CreateTexture2D(
    &depthStencilDesc,
    nullptr,
    &depthStencil
  ); CHECK_ERR();

  CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
  hr = m_d3dDevice->CreateDepthStencilView(
    depthStencil.Get(),
    &depthStencilViewDesc,
    &m_d3dDepthStencilView
  ); CHECK_ERR();

  // Set the 3D rendering viewport to target the entire window.
  m_screenViewport = CD3D11_VIEWPORT(
    0.0f,
    0.0f,
    m_outputSize.Width,
    m_outputSize.Height
  );

  m_deferrContext->RSSetViewports(1, &m_screenViewport);
}

HRESULT DX::DeviceResources::CreateSwapChain(DXGI_SWAP_CHAIN_DESC1& desc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC& fsDesc, IDXGISwapChain1** ppSwapChain) const
{
  HRESULT hr;
#ifdef TARGET_WINDOWS_DESKTOP
  hr = m_dxgiFactory->CreateSwapChainForHwnd(
    m_d3dDevice.Get(),
    m_window,
    &desc,
    &fsDesc,
    nullptr,
    ppSwapChain
  ); RETURN_ERR(hr);
  hr = m_dxgiFactory->MakeWindowAssociation(m_window, /*DXGI_MWA_NO_WINDOW_CHANGES |*/ DXGI_MWA_NO_ALT_ENTER); 
#else
  hr = m_dxgiFactory->CreateSwapChainForCoreWindow(
    m_d3dDevice.Get(),
    reinterpret_cast<IUnknown*>(m_coreWindow.Get()),
    &desc,
    nullptr,
    ppSwapChain
  ); RETURN_ERR(hr);
#endif
  return hr;
}

void DX::DeviceResources::ResizeBuffers()
{
  if (!m_bDeviceCreated)
    return;

  CLog::LogF(LOGDEBUG, "resize buffers.");

  bool bHWStereoEnabled = RENDER_STEREO_MODE_HARDWAREBASED == g_graphicsContext.GetStereoMode();
  bool windowed = true;

  if (m_swapChain)
  {
    // check if swapchain needs to be recreated
    DXGI_SWAP_CHAIN_DESC1 scDesc = { 0 };
    m_swapChain->GetDesc1(&scDesc);
    if ((scDesc.Stereo == TRUE) != bHWStereoEnabled)
    {
      // check fullscreen state and go to windowing if necessary
      BOOL bFullcreen;
      m_swapChain->GetFullscreenState(&bFullcreen, nullptr);
      if (!!bFullcreen)
      {
        windowed = false; // will create fullscreen swapchain
        m_swapChain->SetFullscreenState(false, nullptr); // mandatory before releasing swapchain
      }

      m_swapChain = nullptr;
      m_deferrContext->Flush();
      m_d3dContext->Flush();
    }
  }

  if (m_swapChain != nullptr)
  {
    // If the swap chain already exists, resize it.
    HRESULT hr = m_swapChain->ResizeBuffers(
      2, // Double-buffered swap chain.
      lround(m_outputSize.Width),
      lround(m_outputSize.Height),
      DXGI_FORMAT_B8G8R8A8_UNORM,
      0
    );

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
      // If the device was removed for any reason, a new device and swap chain will need to be created.
      HandleDeviceLost(hr == DXGI_ERROR_DEVICE_REMOVED);

      // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
      // and correctly set up the new device.
      return;
    }
    CHECK_ERR();
  }
  else
  {
    // Otherwise, create a new one using the same adapter as the existing Direct3D device.
    DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;

    swapChainDesc.Width = lround(m_outputSize.Width);
    swapChainDesc.Height = lround(m_outputSize.Height);
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = bHWStereoEnabled;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 3 * (1 + bHWStereoEnabled);
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    swapChainDesc.Scaling = scaling;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC scFSDesc = { 0 }; // unused for uwp
    scFSDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    scFSDesc.Windowed = windowed;

    ComPtr<IDXGISwapChain1> swapChain;
    HRESULT hr = CreateSwapChain(swapChainDesc, scFSDesc, &swapChain);

    if (FAILED(hr) && bHWStereoEnabled)
    {
      // switch to stereo mode failed, create mono swapchain
      CLog::LogF(LOGERROR, "creating stereo swap chain failed with error.");
      CLog::LogF(LOGNOTICE, "fallback to monoscopic mode.");

      swapChainDesc.Stereo = false;
      bHWStereoEnabled = false;

      hr = CreateSwapChain(swapChainDesc, scFSDesc, &swapChain); CHECK_ERR();

      // fallback to split_horizontal mode.
      g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_SPLIT_HORIZONTAL);
    }

    hr = swapChain.As(&m_swapChain); CHECK_ERR();
    m_stereoEnabled = bHWStereoEnabled;

    // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
    // ensures that the application will only render after each VSync, minimizing power consumption.
    ComPtr<IDXGIDevice1> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice); CHECK_ERR();
    dxgiDevice->SetMaximumFrameLatency(1);
  }
}

// These resources need to be recreated every time the window size is changed.
void DX::DeviceResources::CreateWindowSizeDependentResources() 
{
  ReleaseBackBuffer();

  UpdateRenderTargetSize();
  ResizeBuffers();

  CreateBackBuffer();
}

// Determine the dimensions of the render target and whether it will be scaled down.
void DX::DeviceResources::UpdateRenderTargetSize()
{
  m_effectiveDpi = m_dpi;

  // To improve battery life on high resolution devices, render to a smaller render target
  // and allow the GPU to scale the output when it is presented.
  if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
  {
    float width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
    float height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

    // When the device is in portrait orientation, height > width. Compare the
    // larger dimension against the width threshold and the smaller dimension
    // against the height threshold.
    if (std::max(width, height) > DisplayMetrics::WidthThreshold && std::min(width, height) > DisplayMetrics::HeightThreshold)
    {
      // To scale the app we change the effective DPI. Logical size does not change.
      m_effectiveDpi /= 2.0f;
    }
  }

  // Calculate the necessary render target size in pixels.
  m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
  m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

  // Prevent zero size DirectX content from being created.
  m_outputSize.Width = std::max(m_outputSize.Width, 1.f);
  m_outputSize.Height = std::max(m_outputSize.Height, 1.f);
}

void DX::DeviceResources::Register(ID3DResource* resource)
{
  critical_section::scoped_lock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void DX::DeviceResources::Unregister(ID3DResource* resource)
{
  critical_section::scoped_lock lock(m_resourceSection);
  std::vector<ID3DResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

void DX::DeviceResources::FinishCommandList(bool bExecute) const
{
  if (m_d3dContext == m_deferrContext)
    return;

  ComPtr<ID3D11CommandList> pCommandList;
  if (FAILED(m_deferrContext->FinishCommandList(true, &pCommandList)))
  {
    CLog::LogF(LOGERROR, "failed to finish command queue.");
    return;
  }

  if (bExecute)
    m_d3dContext->ExecuteCommandList(pCommandList.Get(), false);
}

// This method is called in the event handler for the SizeChanged event.
void DX::DeviceResources::SetLogicalSize(float width, float height)
{
  if
#if defined(TARGET_WINDOWS_DESKTOP)
  (!m_window)
#else
  (!m_coreWindow.Get())
#endif
    return;

  CLog::LogF(LOGDEBUG, "receive changing logical size to %f x %f", width, height);

  if (m_logicalSize.Width != width || m_logicalSize.Height != height)
  {
    CLog::LogF(LOGDEBUG, "change logical size to %f x %f", width, height);

    m_logicalSize = Size(width, height);

    UpdateRenderTargetSize();
    ResizeBuffers();
  }
}

// This method is called in the event handler for the DpiChanged event.
void DX::DeviceResources::SetDpi(float dpi)
{
  if (dpi != m_dpi)
  {
    m_dpi = dpi;
    CreateWindowSizeDependentResources();
  }
}

// This method is called in the event handler for the DisplayContentsInvalidated event.
void DX::DeviceResources::ValidateDevice()
{
  // The D3D Device is no longer valid if the default adapter changed since the device
  // was created or if the device has been removed.

  // First, get the information for the default adapter from when the device was created.
  ComPtr<IDXGIDevice1> dxgiDevice;
  m_d3dDevice.As(&dxgiDevice);

  ComPtr<IDXGIAdapter> deviceAdapter;
  dxgiDevice->GetAdapter(&deviceAdapter);

  ComPtr<IDXGIFactory2> dxgiFactory;
  deviceAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

  DXGI_ADAPTER_DESC1 previousDesc;
  {
    ComPtr<IDXGIAdapter1> previousDefaultAdapter;
    dxgiFactory->EnumAdapters1(0, &previousDefaultAdapter);

    previousDefaultAdapter->GetDesc1(&previousDesc);
  }

  // Next, get the information for the current default adapter.
  DXGI_ADAPTER_DESC1 currentDesc;
  {
    ComPtr<IDXGIFactory1> currentFactory;
    CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory));

    ComPtr<IDXGIAdapter1> currentDefaultAdapter;
    currentFactory->EnumAdapters1(0, &currentDefaultAdapter);

    currentDefaultAdapter->GetDesc1(&currentDesc);
  }
  // If the adapter LUIDs don't match, or if the device reports that it has been removed,
  // a new D3D device must be created.
  HRESULT hr = m_d3dDevice->GetDeviceRemovedReason();
  if ( previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart 
    || previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart 
    || FAILED(hr))
  {
    // Release references to resources related to the old device.
    dxgiDevice = nullptr;
    deviceAdapter = nullptr;
    dxgiFactory = nullptr;

    // Create a new device and swap chain.
    HandleDeviceLost(hr == DXGI_ERROR_DEVICE_REMOVED);
  }
}

void DX::DeviceResources::OnDeviceLost(bool removed)
{
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_LOST);

  // tell any shared resources
  for (auto res : m_resources)
  {
    // the most of resources like textures and buffers try to 
    // receive and save their status from current device.
    // `removed` means that we have no possibility
    // to use the device anymore, tell all resouces about this.
    res->OnDestroyDevice(removed);
  }
}

void DX::DeviceResources::OnDeviceRestored()
{
  // tell any shared resources
  for (auto res : m_resources)
    res->OnCreateDevice();

  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
}

// Recreate all device resources and set them back to the current state.
void DX::DeviceResources::HandleDeviceLost(bool removed)
{
  OnDeviceLost(removed);
  if (m_deviceNotify != nullptr)
    m_deviceNotify->OnDXDeviceLost();

  ReleaseBackBuffer();
  m_swapChain = nullptr;

  CreateDeviceResources();
  CreateWindowSizeDependentResources();

  if (m_deviceNotify != nullptr)
    m_deviceNotify->OnDXDeviceRestored();
  OnDeviceRestored();

  KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "ReloadSkin");
}

bool DX::DeviceResources::Begin()
{
  HRESULT hr = m_swapChain->Present(0, DXGI_PRESENT_TEST);

  // If the device was removed either by a disconnection or a driver upgrade, we 
  // must recreate all device resources.
  if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
  {
    HandleDeviceLost(hr == DXGI_ERROR_DEVICE_REMOVED);
  }
  else
  {
    // not fatal errors
    if (hr == DXGI_ERROR_INVALID_CALL)
    {
      CreateWindowSizeDependentResources();
    }
  }

  m_deferrContext->OMSetRenderTargets(1, m_backBufferTex.GetAddressOfRTV(), m_d3dDepthStencilView.Get());

  return true;
}

// Present the contents of the swap chain to the screen.
void DX::DeviceResources::Present() 
{
  FinishCommandList();
  m_d3dContext->Flush();

  // The first argument instructs DXGI to block until VSync, putting the application
  // to sleep until the next VSync. This ensures we don't waste any cycles rendering
  // frames that will never be displayed to the screen.
  DXGI_PRESENT_PARAMETERS parameters = { 0 };
  HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

  // If the device was removed either by a disconnection or a driver upgrade, we 
  // must recreate all device resources.
  if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
  {
    HandleDeviceLost(hr == DXGI_ERROR_DEVICE_REMOVED);
  }
  else
  {
    // not fatal errors
    if (hr == DXGI_ERROR_INVALID_CALL)
    {
      CreateWindowSizeDependentResources();
    }
  }

  if (m_d3dContext == m_deferrContext)
  {
    m_deferrContext->OMSetRenderTargets(1, m_backBufferTex.GetAddressOfRTV(), m_d3dDepthStencilView.Get());
  }
}

void DX::DeviceResources::ClearDepthStencil() const
{
  m_deferrContext->ClearDepthStencilView(m_d3dDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
}

void DX::DeviceResources::ClearRenderTarget(ID3D11RenderTargetView* pRTView, float color[4]) const
{
  m_deferrContext->ClearRenderTargetView(pRTView, color);
}

void DX::DeviceResources::SetMonitor(HMONITOR monitor) const
{
  HRESULT hr;
  DXGI_ADAPTER_DESC currentDesc = { 0 };
  DXGI_ADAPTER_DESC foundDesc = { 0 };

  ComPtr<IDXGIFactory1> dxgiFactory;
  ComPtr<IDXGIAdapter1> adapter;

  if (m_d3dDevice)
  {
    ComPtr<IDXGIDevice1> dxgiDevice;
    ComPtr<IDXGIAdapter> deviceAdapter;

    hr = m_d3dDevice.As(&dxgiDevice); CHECK_ERR();
    dxgiDevice->GetAdapter(&deviceAdapter);
    deviceAdapter->GetDesc(&currentDesc);
  }

  CreateDXGIFactory1(IID_IDXGIFactory1, &dxgiFactory);

  int index = 0;
  while (true)
  {
    hr = dxgiFactory->EnumAdapters1(index, &adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
      break;

    ComPtr<IDXGIOutput> output;
    adapter->GetDesc(&foundDesc);

    for (int j = 0; adapter->EnumOutputs(j, &output) != DXGI_ERROR_NOT_FOUND; j++)
    {
      DXGI_OUTPUT_DESC outputDesc;
      output->GetDesc(&outputDesc);
      if (outputDesc.Monitor == monitor)
      {
        // check if adapter is changed
        if (currentDesc.AdapterLuid.HighPart != foundDesc.AdapterLuid.HighPart
          || currentDesc.AdapterLuid.LowPart != foundDesc.AdapterLuid.LowPart)
        {
          CLog::LogF(LOGDEBUG, "selected %S adapter. ", foundDesc.Description);

          // adapter is changed, (re)init hooks into new driver
          g_Windowing.InitHooks(output.Get());
        }
        return;
      }
    }
  }
}

void DX::DeviceResources::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
{
  m_deviceNotify = deviceNotify;
}

HMONITOR DX::DeviceResources::GetMonitor() const
{
  if (m_swapChain)
  {
    ComPtr<IDXGIOutput> output;
    HRESULT hr = m_swapChain->GetContainingOutput(&output); RETURN_ERR(nullptr);
    if (output)
    {
      DXGI_OUTPUT_DESC desc;
      output->GetDesc(&desc);
      return desc.Monitor;
    }
  }
  return nullptr;
}

bool DX::DeviceResources::IsStereoAvailable() const
{
  if (m_dxgiFactory)
    return m_dxgiFactory->IsWindowedStereoEnabled();

  return false;
}

#if defined(TARGET_WINDOWS_DESKTOP)
// This method is called when the window (WND) is created (or re-created).
void DX::DeviceResources::SetWindow(HWND window)
{
  m_window = window;

  CreateDeviceIndependentResources();
  CreateDeviceResources();
}
#elif defined(TARGET_WINDOWS_STORE)
// This method is called when the CoreWindow is created (or re-created).
void DX::DeviceResources::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
  m_coreWindow = window;
  auto dispatcher = m_coreWindow->Dispatcher;
  auto handler = ref new Windows::UI::Core::DispatchedHandler([&]()
  {
    auto coreWindow = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    m_logicalSize = Windows::Foundation::Size(coreWindow->Bounds.Width, coreWindow->Bounds.Height);
    m_dpi = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
  });
  if (dispatcher->HasThreadAccess)
    handler->Invoke();
  else
    Concurrency::create_task(dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, handler)).wait();

  CreateDeviceIndependentResources();
  CreateDeviceResources();
  // we have to call this because we will not get initial WM_SIZE
  CreateWindowSizeDependentResources();
}

// Call this method when the app suspends. It provides a hint to the driver that the app 
// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
void DX::DeviceResources::Trim() const
{
  ComPtr<IDXGIDevice3> dxgiDevice;
  m_d3dDevice.As(&dxgiDevice);

  dxgiDevice->Trim();
}

#endif