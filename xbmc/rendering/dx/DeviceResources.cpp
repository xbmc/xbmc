/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "RenderContext.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "windowing/GraphicContext.h"
#include "messaging/ApplicationMessenger.h"
#include "platform/win32/CharsetConverter.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxgi.lib")
#endif // _DEBUG

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Concurrency;
namespace winrt
{
  using namespace Windows::Foundation;
}

#ifdef _DEBUG
#define breakOnDebug __debugbreak()
#else
#define breakOnDebug
#endif
#define LOG_HR(hr) CLog::LogF(LOGERROR, "function call at line %d ends with error: %s", __LINE__, DX::GetErrorDescription(hr).c_str());
#define CHECK_ERR() if (FAILED(hr)) { LOG_HR(hr); breakOnDebug; return; }
#define RETURN_ERR(ret) if (FAILED(hr)) { LOG_HR(hr); breakOnDebug; return (##ret); }

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

DX::DeviceResources::~DeviceResources() = default;

void DX::DeviceResources::Release()
{
  if (!m_bDeviceCreated)
    return;

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
#ifdef _DEBUG
  if (m_d3dDebug)
  {
    m_d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
    m_d3dDebug = nullptr;
  }
#endif
}

void DX::DeviceResources::GetOutput(IDXGIOutput** ppOutput) const
{
  ComPtr<IDXGIOutput> pOutput;
  if (FAILED(m_swapChain->GetContainingOutput(pOutput.GetAddressOf())) || !pOutput)
    m_output.As(&pOutput);
  *ppOutput = pOutput.Detach();
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

  GetOutput(pOutput.GetAddressOf());
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
    if (sDevMode.dmDisplayFlags & DM_INTERLACED)
    {
      mode->RefreshRate.Numerator *= 2;
      mode->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST; // guessing
    }
    else
      mode->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
  }
#endif
}

void DX::DeviceResources::SetViewPort(D3D11_VIEWPORT& viewPort) const
{
  // convert logical viewport to real
  D3D11_VIEWPORT realViewPort =
  {
    viewPort.TopLeftX,
    viewPort.TopLeftY,
    viewPort.Width,
    viewPort.Height,
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

  BOOL bFullScreen;
  m_swapChain->GetFullscreenState(&bFullScreen, nullptr);

  CLog::LogF(LOGDEBUG, "switching from %s(%.0f x %.0f) to %s(%d x %d)",
             bFullScreen ? "fullscreen " : "", m_outputSize.Width, m_outputSize.Height,
             fullscreen  ? "fullscreen " : "", res.iWidth, res.iHeight);

  bool recreate = m_stereoEnabled != (CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED);
  if (!!bFullScreen && !fullscreen)
  {
    CLog::LogF(LOGDEBUG, "switching to windowed");
    recreate |= SUCCEEDED(m_swapChain->SetFullscreenState(false, nullptr));
  }
  else if (fullscreen)
  {
    const bool isResValid = res.iWidth > 0 && res.iHeight > 0 && res.fRefreshRate > 0.f;
    if (isResValid)
    {
      DXGI_MODE_DESC currentMode;
      GetDisplayMode(&currentMode);
      DXGI_SWAP_CHAIN_DESC scDesc;
      m_swapChain->GetDesc(&scDesc);

      bool is_interlaced = scDesc.BufferDesc.ScanlineOrdering > DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
      float refreshRate = res.fRefreshRate;
      if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
        refreshRate *= 2;

      if (currentMode.Width != res.iWidth
        || currentMode.Height != res.iHeight
        || DX::RationalToFloat(currentMode.RefreshRate) != refreshRate
        || is_interlaced != (res.dwFlags & D3DPRESENTFLAG_INTERLACED ? true : false)
        // force resolution change for stereo mode
        // some drivers unable to create stereo swapchain if mode does not match @23.976
        || CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode() == RENDER_STEREO_MODE_HARDWAREBASED)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__": changing display mode to %dx%d@%0.3f", res.iWidth, res.iHeight, res.fRefreshRate,
                  res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

        int refresh = static_cast<int>(res.fRefreshRate);
        int i = (refresh + 1) % 24 == 0 || (refresh + 1) % 30 == 0 ? 1 : 0;

        currentMode.Width = res.iWidth;
        currentMode.Height = res.iHeight;
        currentMode.RefreshRate.Numerator = (refresh + i) * 1000;
        currentMode.RefreshRate.Denominator = 1000 + i;
        if (res.dwFlags & D3DPRESENTFLAG_INTERLACED)
        {
          currentMode.RefreshRate.Numerator *= 2;
          currentMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST; // guessing;
        }
        // sometimes the OS silently brings Kodi out of full screen mode
        // in this case switching a resolution has no any effect and
        // we have to enter into full screen mode before switching
        if (!bFullScreen)
        {
          ComPtr<IDXGIOutput> pOutput;
          GetOutput(pOutput.GetAddressOf());

          CLog::LogF(LOGDEBUG, "fixup fullscreen mode before switching resolution");
          recreate |= SUCCEEDED(m_swapChain->SetFullscreenState(true, pOutput.Get()));
          m_swapChain->GetFullscreenState(&bFullScreen, nullptr);
        }
        bool resized = SUCCEEDED(m_swapChain->ResizeTarget(&currentMode));
        if (resized) 
        {
          // some system doesn't inform windowing about desktop size changes
          // so we have to change output size before resizing buffers
          m_outputSize.Width = static_cast<float>(currentMode.Width);
          m_outputSize.Height = static_cast<float>(currentMode.Height);
        }
        recreate |= resized;
      }
    }
    if (!bFullScreen)
    {
      ComPtr<IDXGIOutput> pOutput;
      GetOutput(pOutput.GetAddressOf());

      CLog::LogF(LOGDEBUG, "switching to fullscreen");
      recreate |= SUCCEEDED(m_swapChain->SetFullscreenState(true, pOutput.Get()));
    }
  }

  // resize backbuffer to proper hanlde fullscreen/stereo transition
  if (recreate)
    ResizeBuffers();

  CLog::LogF(LOGDEBUG, "switching done.");

  return recreate;
}

// Configures resources that don't depend on the Direct3D device.
void DX::DeviceResources::CreateDeviceIndependentResources()
{
}

// Configures the Direct3D device, and stores handles to it and the device context.
void DX::DeviceResources::CreateDeviceResources()
{
  CLog::LogF(LOGDEBUG, "creating DirectX 11 device.");

  CreateFactory();

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
  std::vector<D3D_FEATURE_LEVEL> featureLevels;
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8))
    featureLevels.push_back(D3D_FEATURE_LEVEL_11_1);

  featureLevels.push_back(D3D_FEATURE_LEVEL_11_0);
  featureLevels.push_back(D3D_FEATURE_LEVEL_10_1);
  featureLevels.push_back(D3D_FEATURE_LEVEL_10_0);
  featureLevels.push_back(D3D_FEATURE_LEVEL_9_3);
  featureLevels.push_back(D3D_FEATURE_LEVEL_9_2);
  featureLevels.push_back(D3D_FEATURE_LEVEL_9_1);

  // Create the Direct3D 11 API device object and a corresponding context.
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;

  D3D_DRIVER_TYPE drivertType = m_adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
  HRESULT hr = D3D11CreateDevice(
      m_adapter.Get(),           // Create a device on specified adapter.
      drivertType,               // Create a device using scepcified driver.
      nullptr,                   // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
      creationFlags,             // Set debug and Direct2D compatibility flags.
      featureLevels.data(),      // List of feature levels this app can support.
      featureLevels.size(),      // Size of the list above.
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
        featureLevels.data(),
        featureLevels.size(),
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

#ifdef _DEBUG
  if (SUCCEEDED(m_d3dDevice.As(&m_d3dDebug)))
  {
    ComPtr<ID3D11InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(m_d3dDebug.As(&d3dInfoQueue)))
    {
      std::vector<D3D11_MESSAGE_ID> hide =
      {
        D3D11_MESSAGE_ID_GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED,        // avoid GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED (dx bug)
        D3D11_MESSAGE_ID_DEVICE_RSSETSCISSORRECTS_NEGATIVESCISSOR         // avoid warning for some labels out of screen
                                                                          // Add more message IDs here as needed
      };

      D3D11_INFO_QUEUE_FILTER filter;
      ZeroMemory(&filter, sizeof(filter));
      filter.DenyList.NumIDs = hide.size();
      filter.DenyList.pIDList = hide.data();
      d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
  }
#endif

  hr = context.As(&m_d3dContext); CHECK_ERR();
  hr = m_d3dDevice->CreateDeferredContext1(0, &m_deferrContext); CHECK_ERR();

  if (!m_adapter)
  {
    ComPtr<IDXGIDevice1> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    hr = m_d3dDevice.As(&dxgiDevice); CHECK_ERR();
    hr = dxgiDevice->GetAdapter(&adapter); CHECK_ERR();
    hr = adapter.As(&m_adapter); CHECK_ERR();
  }

  DXGI_ADAPTER_DESC aDesc;
  m_adapter->GetDesc(&aDesc);

  CLog::LogF(LOGDEBUG, "device is created on adapter '%s' with feature level %04x.",
             KODI::PLATFORM::WINDOWS::FromW(aDesc.Description), m_d3dFeatureLevel);

  m_bDeviceCreated = true;
}

void DX::DeviceResources::ReleaseBackBuffer()
{
  CLog::LogF(LOGDEBUG, "release buffers.");

  m_backBufferTex.Release();
  m_d3dDepthStencilView = nullptr;
  if (m_deferrContext)
  {
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = { nullptr, nullptr, nullptr, nullptr };
    m_deferrContext->OMSetRenderTargets(4, nullViews, nullptr);
    FinishCommandList(false);

    m_deferrContext->Flush();
    m_d3dContext->Flush();
  }
}

void DX::DeviceResources::CreateBackBuffer()
{
  if (!m_bDeviceCreated || !m_swapChain)
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
    winrt::get_unknown(m_coreWindow),
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

  bool bHWStereoEnabled = RENDER_STEREO_MODE_HARDWAREBASED ==
                          CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
  bool windowed = true;
  bool isHdrEnabled = false;
  HRESULT hr = E_FAIL;
  DXGI_SWAP_CHAIN_DESC1 scDesc = { 0 };

  if (m_swapChain)
  {
    BOOL bFullcreen = 0;
    m_swapChain->GetFullscreenState(&bFullcreen, nullptr);
    if (!!bFullcreen)
    {
      windowed = false;
    }

    // check if swapchain needs to be recreated
    m_swapChain->GetDesc1(&scDesc);
    isHdrEnabled = IsDisplayHDREnabled();

    if ((scDesc.Stereo == TRUE) != bHWStereoEnabled || (m_Is10bSwapchain != isHdrEnabled))
    {
      // check fullscreen state and go to windowing if necessary
      if (!!bFullcreen)
      {
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
    m_swapChain->GetDesc1(&scDesc);
    hr = m_swapChain->ResizeBuffers(
      scDesc.BufferCount,
      lround(m_outputSize.Width),
      lround(m_outputSize.Height),
      scDesc.Format,
      windowed ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
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
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.Width = lround(m_outputSize.Width);
    swapChainDesc.Height = lround(m_outputSize.Height);
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = bHWStereoEnabled;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 3 * (1 + bHWStereoEnabled);
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = windowed ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC scFSDesc = { 0 }; // unused for uwp
    scFSDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scFSDesc.Windowed = windowed;

    ComPtr<IDXGISwapChain1> swapChain;
    if (m_d3dFeatureLevel >= D3D_FEATURE_LEVEL_11_0 && !bHWStereoEnabled &&
        (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bTry10bitOutput ||
         isHdrEnabled))
    {
      swapChainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
      hr = CreateSwapChain(swapChainDesc, scFSDesc, &swapChain);
      if (FAILED(hr))
      {
        CLog::LogF(LOGWARNING, "creating 10bit swapchain failed, fallback to 8bit.");
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      }
    }

    if (!swapChain)
      hr = CreateSwapChain(swapChainDesc, scFSDesc, &swapChain);

    if (FAILED(hr) && bHWStereoEnabled)
    {
      // switch to stereo mode failed, create mono swapchain
      CLog::LogF(LOGERROR, "creating stereo swap chain failed with error.");
      CLog::LogF(LOGNOTICE, "fallback to monoscopic mode.");

      swapChainDesc.Stereo = false;
      bHWStereoEnabled = false;

      hr = CreateSwapChain(swapChainDesc, scFSDesc, &swapChain); CHECK_ERR();

      // fallback to split_horizontal mode.
      CServiceBroker::GetWinSystem()->GetGfxContext().SetStereoMode(
          RENDER_STEREO_MODE_SPLIT_HORIZONTAL);
    }

    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "unable to create swapchain.");
      return;
    }

    if (swapChainDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
    {
      std::string txOutput;
      m_Is10bSwapchain = true;
      if (isHdrEnabled)
      {
        m_IsHDROutput = true;
        txOutput = "HDR";
      }
      else
      {
        m_IsHDROutput = false;
        txOutput = "SDR";
      }
      CLog::LogF(LOGNOTICE, "10 bit swapchain is used with {0:s} output", txOutput);
    }
    else
    {
      m_Is10bSwapchain = false;
      m_IsHDROutput = false;
      CLog::LogF(LOGNOTICE, "8 bit swapchain is used with SDR output");
    }

    hr = swapChain.As(&m_swapChain); CHECK_ERR();
    m_stereoEnabled = bHWStereoEnabled;

    // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
    // ensures that the application will only render after each VSync, minimizing power consumption.
    ComPtr<IDXGIDevice1> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice); CHECK_ERR();
    dxgiDevice->SetMaximumFrameLatency(1);
  }

  CLog::LogF(LOGDEBUG, "end resize buffers.");
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
  (!m_coreWindow)
#endif
    return;

  CLog::LogF(LOGDEBUG, "receive changing logical size to %f x %f", width, height);

  if (m_logicalSize.Width != width || m_logicalSize.Height != height)
  {
    CLog::LogF(LOGDEBUG, "change logical size to %f x %f", width, height);

    m_logicalSize = winrt::Size(width, height);

    UpdateRenderTargetSize();
    ResizeBuffers();
  }
}

// This method is called in the event handler for the DpiChanged event.
void DX::DeviceResources::SetDpi(float dpi)
{
  dpi = std::max(dpi, DisplayMetrics::Dpi100);
  if (dpi != m_dpi)
    m_dpi = dpi;
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
  auto pGUI = CServiceBroker::GetGUI();
  if (pGUI)
    pGUI->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_LOST);

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

  auto pGUI = CServiceBroker::GetGUI();
  if (pGUI)
    pGUI->GetWindowManager().SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
}

// Recreate all device resources and set them back to the current state.
void DX::DeviceResources::HandleDeviceLost(bool removed)
{
  bool backbuferExists = m_backBufferTex.Get() != nullptr;

  OnDeviceLost(removed);
  if (m_deviceNotify != nullptr)
    m_deviceNotify->OnDXDeviceLost();

  if (backbuferExists)
    ReleaseBackBuffer();

  m_swapChain = nullptr;
  CreateDeviceResources();
  UpdateRenderTargetSize();
  ResizeBuffers();

  if (backbuferExists)
    CreateBackBuffer();

  if (m_deviceNotify != nullptr)
    m_deviceNotify->OnDXDeviceRestored();
  OnDeviceRestored();

  if (removed)
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
    if (!m_dxgiFactory->IsCurrent())
      CreateFactory();
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

void DX::DeviceResources::HandleOutputChange(const std::function<bool(DXGI_OUTPUT_DESC)>& cmpFunc)
{
  DXGI_ADAPTER_DESC currentDesc = { 0 };
  DXGI_ADAPTER_DESC foundDesc = { 0 };

  ComPtr<IDXGIFactory1> factory;
  if (m_adapter)
    m_adapter->GetDesc(&currentDesc);

  CreateDXGIFactory1(IID_IDXGIFactory1, &factory);

  ComPtr<IDXGIAdapter1> adapter;
  for (int i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++)
  {
    adapter->GetDesc(&foundDesc);
    ComPtr<IDXGIOutput> output;
    for (int j = 0; adapter->EnumOutputs(j, output.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; j++)
    {
      DXGI_OUTPUT_DESC outputDesc;
      output->GetDesc(&outputDesc);
      if (cmpFunc(outputDesc))
      {
        output.As(&m_output);
        // check if adapter is changed
        if (currentDesc.AdapterLuid.HighPart != foundDesc.AdapterLuid.HighPart
          || currentDesc.AdapterLuid.LowPart != foundDesc.AdapterLuid.LowPart)
        {
          // adapter is changed
          m_adapter = adapter;
          CLog::LogF(LOGDEBUG, "selected {} adapter. ",
                     KODI::PLATFORM::WINDOWS::FromW(foundDesc.Description));
          // (re)init hooks into new driver
          Windowing()->InitHooks(output.Get());
          // recreate d3d11 device on new adapter
          if (m_d3dDevice)
            HandleDeviceLost(false);
        }
        return;
      }
    }
  }
}

bool DX::DeviceResources::CreateFactory()
{
  HRESULT hr;
#if defined(_DEBUG) && defined(TARGET_WINDOWS_STORE)
  bool debugDXGI = false;
  {
    ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
    {
      debugDXGI = true;

      hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())); RETURN_ERR(false);

      dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
      dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }
  }

  if (!debugDXGI)
#endif
  hr = CreateDXGIFactory1(IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())); RETURN_ERR(false);

  return true;
}

void DX::DeviceResources::SetMonitor(HMONITOR monitor)
{
  HandleOutputChange([monitor](DXGI_OUTPUT_DESC outputDesc) {
    return outputDesc.Monitor == monitor;
  });
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
    GetOutput(output.GetAddressOf());
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

bool DX::DeviceResources::DoesTextureSharingWork()
{
  if (m_d3dFeatureLevel < D3D_FEATURE_LEVEL_10_0 ||
    CSysInfo::GetWindowsDeviceFamily() != CSysInfo::Desktop)
    return false;

  if (!CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_allowUseSeparateDeviceForDecoding)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS options;
    if (SUCCEEDED(m_d3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(options))))
    {
      CLog::LogF(LOGDEBUG, "extended sharing resource is{}supported", !!options.ExtendedResourceSharing ? " " : " not ");
      return !!options.ExtendedResourceSharing;
    }
    return false;
  }
  return true;
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
void DX::DeviceResources::SetWindow(const winrt::Windows::UI::Core::CoreWindow& window)
{
  using namespace winrt::Windows::UI::Core;
  using namespace winrt::Windows::Graphics::Display;

  m_coreWindow = window;
  auto dispatcher = m_coreWindow.Dispatcher();
  DispatchedHandler handler([&]()
  {
    auto coreWindow = CoreWindow::GetForCurrentThread();
    m_logicalSize = winrt::Size(coreWindow.Bounds().Width, coreWindow.Bounds().Height);
    m_dpi = DisplayInformation::GetForCurrentView().LogicalDpi();
    SetWindowPos(coreWindow.Bounds());
  });
  if (dispatcher.HasThreadAccess())
    handler();
  else
    dispatcher.RunAsync(CoreDispatcherPriority::High, handler).get();

  CreateDeviceIndependentResources();
  CreateDeviceResources();
  // we have to call this because we will not get initial WM_SIZE
  CreateWindowSizeDependentResources();
}

void DX::DeviceResources::SetWindowPos(winrt::Rect rect)
{
  int centerX = rect.X + rect.Width / 2;
  int centerY = rect.Y + rect.Height / 2;

  HandleOutputChange([centerX, centerY](DXGI_OUTPUT_DESC outputDesc) {
    // DesktopCoordinates depends on the DPI of the desktop
    return outputDesc.DesktopCoordinates.left <= centerX && outputDesc.DesktopCoordinates.right  >= centerX
        && outputDesc.DesktopCoordinates.top  <= centerY && outputDesc.DesktopCoordinates.bottom >= centerY;
  });
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

DXGI_HDR_METADATA_HDR10 DX::DeviceResources::GetHdr10Display() const
{
  ComPtr<IDXGIOutput> pOutput;
  ComPtr<IDXGIOutput6> pOutput6;
  DXGI_HDR_METADATA_HDR10 hdr10 = {};
  DXGI_OUTPUT_DESC1 od = {};
  bool hdrCapable = false;

  if (m_swapChain == nullptr)
    return hdr10;

  if (SUCCEEDED(m_swapChain->GetContainingOutput(pOutput.GetAddressOf())))
  {
    if (SUCCEEDED(pOutput.As(&pOutput6)))
    {
      if (SUCCEEDED(pOutput6->GetDesc1(&od)))
      {
        constexpr float FACTOR_1 = 50000.0;
        constexpr float FACTOR_2 = 10000.0;
        hdr10.RedPrimary[0] = static_cast<uint16_t>(FACTOR_1 * od.RedPrimary[0]);
        hdr10.RedPrimary[1] = static_cast<uint16_t>(FACTOR_1 * od.RedPrimary[1]);
        hdr10.GreenPrimary[0] = static_cast<uint16_t>(FACTOR_1 * od.GreenPrimary[0]);
        hdr10.GreenPrimary[1] = static_cast<uint16_t>(FACTOR_1 * od.GreenPrimary[1]);
        hdr10.BluePrimary[0] = static_cast<uint16_t>(FACTOR_1 * od.BluePrimary[0]);
        hdr10.BluePrimary[1] = static_cast<uint16_t>(FACTOR_1 * od.BluePrimary[1]);
        hdr10.WhitePoint[0] = static_cast<uint16_t>(FACTOR_1 * od.WhitePoint[0]);
        hdr10.WhitePoint[1] = static_cast<uint16_t>(FACTOR_1 * od.WhitePoint[1]);
        hdr10.MaxMasteringLuminance = static_cast<uint32_t>(FACTOR_2 * od.MaxLuminance);
        hdr10.MinMasteringLuminance = static_cast<uint32_t>(FACTOR_2 * od.MinLuminance);
        hdr10.MaxContentLightLevel = static_cast<uint16_t>(od.MaxFullFrameLuminance);

        if (od.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        {
          CLog::LogF(LOGNOTICE, "Display HDR capable detected and HDR current state is ENABLED:");
          hdrCapable = true;
        }
        else if (od.MaxLuminance >= 400.0)
        {
          CLog::LogF(LOGNOTICE, "Display HDR capable is detected but NOT enabled:");
          hdrCapable = true;
        }
        else
        {
          CLog::LogF(LOGNOTICE, "No display HDR capable detected.");
        }
        if (hdrCapable)
        {
          std::string txColorSpace = "UNKNOWN";
          switch (od.ColorSpace)
          {
            case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
              txColorSpace = "DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
              break;
            case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
              txColorSpace = "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709";
              break;
          }
          CLog::LogF(LOGNOTICE, "ColorSpace = {0:s}", txColorSpace);
          CLog::LogF(LOGNOTICE, "RedPrimary = {0:0.3f}, {1:0.3f}", od.RedPrimary[0],
                     od.RedPrimary[1]);
          CLog::LogF(LOGNOTICE, "GreenPrimary = {0:0.3f}, {1:0.3f}", od.GreenPrimary[0],
                     od.GreenPrimary[1]);
          CLog::LogF(LOGNOTICE, "BluePrimary = {0:0.3f}, {1:0.3f}", od.BluePrimary[0],
                     od.BluePrimary[1]);
          CLog::LogF(LOGNOTICE, "WhitePoint = {0:0.3f}, {1:0.3f}", od.WhitePoint[0],
                     od.WhitePoint[1]);
          CLog::LogF(LOGNOTICE, "MinLuminance = {0:0.4f}", od.MinLuminance);
          CLog::LogF(LOGNOTICE, "MaxLuminance = {0:0.0f}", od.MaxLuminance);
          CLog::LogF(LOGNOTICE, "MaxFullFrameLuminance = {0:0.0f}", od.MaxFullFrameLuminance);
        }
      }
      else
      {
        CLog::LogF(LOGERROR, "DXGI GetDesc1 failed");
      }
    }
  }

  return hdr10;
}

void DX::DeviceResources::SetHdrMetaData(DXGI_HDR_METADATA_HDR10& hdr10) const
{
  ComPtr<IDXGISwapChain4> swapChain4;

  if (m_swapChain == nullptr)
    return;

  if (SUCCEEDED(m_swapChain.As(&swapChain4)))
  {
    if (SUCCEEDED(swapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(hdr10), &hdr10)))
    {
      constexpr double FACTOR_1 = 50000.0;
      constexpr double FACTOR_2 = 10000.0;
      const double RP_0 = static_cast<double>(hdr10.RedPrimary[0]) / FACTOR_1;
      const double RP_1 = static_cast<double>(hdr10.RedPrimary[1]) / FACTOR_1;
      const double GP_0 = static_cast<double>(hdr10.GreenPrimary[0]) / FACTOR_1;
      const double GP_1 = static_cast<double>(hdr10.GreenPrimary[1]) / FACTOR_1;
      const double BP_0 = static_cast<double>(hdr10.BluePrimary[0]) / FACTOR_1;
      const double BP_1 = static_cast<double>(hdr10.BluePrimary[1]) / FACTOR_1;
      const double WP_0 = static_cast<double>(hdr10.WhitePoint[0]) / FACTOR_1;
      const double WP_1 = static_cast<double>(hdr10.WhitePoint[1]) / FACTOR_1;
      const double Max_ML = static_cast<double>(hdr10.MaxMasteringLuminance) / FACTOR_2;
      const double min_ML = static_cast<double>(hdr10.MinMasteringLuminance) / FACTOR_2;

      CLog::LogF(LOGNOTICE,
                 "Stream HDR metadata => RP {0:0.3f} {1:0.3f} | GP {2:0.3f} {3:0.3f} | BP "
                 "{4:0.3f} {5:0.3f} | WP {6:0.3f} "
                 "{7:0.3f} | MAX ML {8:0.0f} "
                 "| min ML {9:0.3f} | Max CLL {10:d} | Max FALL {11:d}",
                 RP_0, RP_1, GP_0, GP_1, BP_0, BP_1, WP_0, WP_1, Max_ML, min_ML,
                 hdr10.MaxContentLightLevel, hdr10.MaxFrameAverageLightLevel);
    }
    else
    {
      CLog::LogF(LOGERROR, "DXGI SetHDRMetaData failed");
    }
  }
}

void DX::DeviceResources::SetHdrColorSpace(const DXGI_COLOR_SPACE_TYPE colorSpace) const
{
  ComPtr<IDXGISwapChain3> swapChain3;

  if (m_swapChain == nullptr)
    return;

  if (SUCCEEDED(m_swapChain.As(&swapChain3)))
  {
    DXGI_COLOR_SPACE_TYPE cs = colorSpace;
    if (DX::Windowing()->UseLimitedColor())
    {
      switch (cs)
      {
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
          cs = DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709;
          break;
        case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
          cs = DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020;
          break;
        case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
          cs = DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020;
          break;
      }
    }
    if (SUCCEEDED(swapChain3->SetColorSpace1(cs)))
    {
      CLog::LogF(LOGDEBUG, "DXGI SetColorSpace1 success");
    }
    else
    {
      CLog::LogF(LOGERROR, "DXGI SetColorSpace1 failed");
    }
  }
}

bool DX::DeviceResources::IsDisplayHDREnabled() const
{
  ComPtr<IDXGIOutput> pOutput;
  ComPtr<IDXGIOutput6> pOutput6;
  DXGI_OUTPUT_DESC1 od = {};

  if (m_swapChain == nullptr)
    return false;

  if (SUCCEEDED(m_swapChain->GetContainingOutput(pOutput.GetAddressOf())))
  {
    if (SUCCEEDED(pOutput.As(&pOutput6)))
    {
      if (SUCCEEDED(pOutput6->GetDesc1(&od)))
      {
        CLog::LogF(LOGDEBUG, "DXGI GetDesc1 success");
        if (od.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        {
          return true;
        }
      }
      else
      {
        CLog::LogF(LOGERROR, "DXGI GetDesc1 failed");
      }
    }
  }

  return false;
}
