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
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "rendering/dx/DirectXHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"

#ifdef TARGET_WINDOWS_STORE
#include <winrt/Windows.Graphics.Display.Core.h>

extern "C"
{
#include <libavutil/rational.h>
}
#endif

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
#define LOG_HR(hr) \
  CLog::LogF(LOGERROR, "function call at line {} ends with error: {}", __LINE__, \
             DX::GetErrorDescription(hr));
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
  , m_IsHDROutput(false)
  , m_IsTransferPQ(false)
{
}

DX::DeviceResources::~DeviceResources() = default;

void DX::DeviceResources::Release()
{
  if (!m_bDeviceCreated)
    return;

  ReleaseBackBuffer();
  OnDeviceLost(true);
  DestroySwapChain();

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
  if (!m_swapChain || FAILED(m_swapChain->GetContainingOutput(pOutput.GetAddressOf())) || !pOutput)
    m_output.As(&pOutput);
  *ppOutput = pOutput.Detach();
}

DXGI_ADAPTER_DESC DX::DeviceResources::GetAdapterDesc() const
{
  DXGI_ADAPTER_DESC desc{};

  if (m_adapter)
    m_adapter->GetDesc(&desc);

  // GetDesc() returns VendorId == 0 in Xbox however, we need to know that
  // GPU is AMD to apply workarounds in DXVA.cpp CheckCompatibility() same as desktop
  if (CSysInfo::GetWindowsDeviceFamily() == CSysInfo::Xbox)
    desc.VendorId = PCIV_AMD;

  return desc;
}

void DX::DeviceResources::GetDisplayMode(DXGI_MODE_DESC* mode) const
{
  DXGI_OUTPUT_DESC outDesc;
  ComPtr<IDXGIOutput> pOutput;
  DXGI_SWAP_CHAIN_DESC scDesc;

  if (!m_swapChain)
    return;

  m_swapChain->GetDesc(&scDesc);

  GetOutput(pOutput.GetAddressOf());
  pOutput->GetDesc(&outDesc);

  // desktop coords depend on DPI
  mode->Width = DX::ConvertDipsToPixels(outDesc.DesktopCoordinates.right - outDesc.DesktopCoordinates.left, m_dpi);
  mode->Height = DX::ConvertDipsToPixels(outDesc.DesktopCoordinates.bottom - outDesc.DesktopCoordinates.top, m_dpi);
  mode->Format = scDesc.BufferDesc.Format;
  mode->Scaling = scDesc.BufferDesc.Scaling;
  mode->ScanlineOrdering = scDesc.BufferDesc.ScanlineOrdering;

#ifdef TARGET_WINDOWS_DESKTOP
  DEVMODEW sDevMode = {};
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
#else
  using namespace winrt::Windows::Graphics::Display::Core;

  auto hdmiInfo = HdmiDisplayInformation::GetForCurrentView();
  if (hdmiInfo) // Xbox only
  {
    auto currentMode = hdmiInfo.GetCurrentDisplayMode();
    AVRational refresh = av_d2q(currentMode.RefreshRate(), 120000);
    mode->RefreshRate.Numerator = refresh.num;
    mode->RefreshRate.Denominator = refresh.den;
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
  if (!m_bDeviceCreated || !m_swapChain)
    return false;

  critical_section::scoped_lock lock(m_criticalSection);

  BOOL bFullScreen;
  m_swapChain->GetFullscreenState(&bFullScreen, nullptr);

  CLog::LogF(LOGDEBUG, "switching from {}({:.0f} x {:.0f}) to {}({} x {})",
             bFullScreen ? "fullscreen " : "", m_outputSize.Width, m_outputSize.Height,
             fullscreen ? "fullscreen " : "", res.iWidth, res.iHeight);

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
      DXGI_MODE_DESC currentMode = {};
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
        CLog::Log(LOGDEBUG, __FUNCTION__ ": changing display mode to {}x{}@{:0.3f}", res.iWidth,
                  res.iHeight, res.fRefreshRate,
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

  // resize backbuffer to proper handle fullscreen/stereo transition
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
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10))
  {
    featureLevels.push_back(D3D_FEATURE_LEVEL_12_1);
    featureLevels.push_back(D3D_FEATURE_LEVEL_12_0);
  }
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
    CLog::LogF(LOGERROR, "unable to create hardware device with video support, error {}",
               DX::GetErrorDescription(hr));
    CLog::LogF(LOGERROR, "trying to create hardware device without video support.");

    creationFlags &= ~D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

    hr = D3D11CreateDevice(m_adapter.Get(), drivertType, nullptr, creationFlags,
                           featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, &device,
                           &m_d3dFeatureLevel, &context);

    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "unable to create hardware device, error {}",
                 DX::GetErrorDescription(hr));
      CLog::LogF(LOGERROR, "trying to create WARP device.");

      hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, creationFlags,
                             featureLevels.data(), featureLevels.size(), D3D11_SDK_VERSION, &device,
                             &m_d3dFeatureLevel, &context);

      if (FAILED(hr))
      {
        CLog::LogF(LOGFATAL, "unable to create WARP device. Rendering is not possible. Error {}",
                   DX::GetErrorDescription(hr));
        CHECK_ERR();
      }
    }
  }

  // Store pointers to the Direct3D 11.1 API device and immediate context.
  hr = device.As(&m_d3dDevice); CHECK_ERR();

  // Check shared textures support
  CheckNV12SharedTexturesSupport();

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

      D3D11_INFO_QUEUE_FILTER filter = {};
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

  CLog::LogF(LOGINFO, "device is created on adapter '{}' with {}",
             KODI::PLATFORM::WINDOWS::FromW(aDesc.Description),
             GetFeatureLevelDescription(m_d3dFeatureLevel));

  CheckDXVA2SharedDecoderSurfaces();

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

void DX::DeviceResources::DestroySwapChain()
{
  if (!m_swapChain)
    return;

  BOOL bFullcreen = 0;
  m_swapChain->GetFullscreenState(&bFullcreen, nullptr);
  if (!!bFullcreen)
    m_swapChain->SetFullscreenState(false, nullptr); // mandatory before releasing swapchain
  m_swapChain = nullptr;
  m_deferrContext->Flush();
  m_d3dContext->Flush();
  m_IsTransferPQ = false;
}

void DX::DeviceResources::ResizeBuffers()
{
  if (!m_bDeviceCreated)
    return;

  CLog::LogF(LOGDEBUG, "resize buffers.");

  bool bHWStereoEnabled = RENDER_STEREO_MODE_HARDWAREBASED ==
                          CServiceBroker::GetWinSystem()->GetGfxContext().GetStereoMode();
  bool windowed = true;
  HRESULT hr = E_FAIL;
  DXGI_SWAP_CHAIN_DESC1 scDesc = {};

  if (m_swapChain)
  {
    BOOL bFullcreen = 0;
    m_swapChain->GetFullscreenState(&bFullcreen, nullptr);
    if (!!bFullcreen)
      windowed = false;

    m_swapChain->GetDesc1(&scDesc);
    if ((scDesc.Stereo == TRUE) != bHWStereoEnabled) // check if swapchain needs to be recreated
      DestroySwapChain();
  }

  if (m_swapChain) // If the swap chain already exists, resize it.
  {
    m_swapChain->GetDesc1(&scDesc);
    hr = m_swapChain->ResizeBuffers(scDesc.BufferCount, lround(m_outputSize.Width),
                                    lround(m_outputSize.Height), scDesc.Format,
                                    windowed ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
      // If the device was removed for any reason, a new device and swap chain will need to be created.
      HandleDeviceLost(hr == DXGI_ERROR_DEVICE_REMOVED);

      // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
      // and correctly set up the new device.
      return;
    }
    else if (hr == DXGI_ERROR_INVALID_CALL)
    {
      // Called when Windows HDR is toggled externally to Kodi.
      // Is forced to re-create swap chain to avoid crash.
      CreateWindowSizeDependentResources();
      return;
    }
    CHECK_ERR();
  }
  else // Otherwise, create a new one using the same adapter as the existing Direct3D device.
  {
    HDR_STATUS hdrStatus = CWIN32Util::GetWindowsHDRStatus();
    const bool isHdrEnabled = (hdrStatus == HDR_STATUS::HDR_ON);
    bool use10bit = (hdrStatus != HDR_STATUS::HDR_UNSUPPORTED);

// Xbox needs 10 bit swapchain to output true 4K resolution
#ifdef TARGET_WINDOWS_DESKTOP
    // Some AMD graphics has issues with 10 bit in SDR.
    // Enabled by default only in Intel and NVIDIA with latest drivers/hardware
    if (m_d3dFeatureLevel < D3D_FEATURE_LEVEL_12_1 || GetAdapterDesc().VendorId == PCIV_AMD)
      use10bit = false;
#endif

    // 0 = Auto | 1 = Never | 2 = Always
    int use10bitSetting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
        CSettings::SETTING_VIDEOSCREEN_10BITSURFACES);

    if (use10bitSetting == 1)
      use10bit = false;
    else if (use10bitSetting == 2)
      use10bit = true;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = lround(m_outputSize.Width);
    swapChainDesc.Height = lround(m_outputSize.Height);
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = bHWStereoEnabled;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
#ifdef TARGET_WINDOWS_DESKTOP
    swapChainDesc.BufferCount = 6; // HDR 60 fps needs 6 buffers to avoid frame drops
#else
    swapChainDesc.BufferCount = 3; // Xbox don't like 6 backbuffers (3 is fine even for 4K 60 fps)
#endif
    // FLIP_DISCARD improves performance (needed in some systems for 4K HDR 60 fps)
    swapChainDesc.SwapEffect = CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10)
                                   ? DXGI_SWAP_EFFECT_FLIP_DISCARD
                                   : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = windowed ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC scFSDesc = {}; // unused for uwp
    scFSDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    scFSDesc.Windowed = windowed;

    ComPtr<IDXGISwapChain1> swapChain;
    if (m_d3dFeatureLevel >= D3D_FEATURE_LEVEL_11_0 && !bHWStereoEnabled &&
        (isHdrEnabled || use10bit))
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
      CLog::LogF(LOGINFO, "fallback to monoscopic mode.");

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

    m_IsHDROutput = (swapChainDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM) && isHdrEnabled;

    CLog::LogF(
        LOGINFO, "{} bit swapchain is used with {} flip {} buffers and {} output (format {})",
        (swapChainDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM) ? 10 : 8, swapChainDesc.BufferCount,
        (swapChainDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD) ? "discard" : "sequential",
        m_IsHDROutput ? "HDR" : "SDR", DX::DXGIFormatToString(swapChainDesc.Format));

    hr = swapChain.As(&m_swapChain); CHECK_ERR();
    m_stereoEnabled = bHWStereoEnabled;

    if (CServiceBroker::GetLogging().IsLogLevelLogged(LOGDEBUG) &&
        CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
    {
      std::string colorSpaces;
      for (const DXGI_COLOR_SPACE_TYPE& colorSpace : GetSwapChainColorSpaces())
      {
        colorSpaces.append("\n");
        colorSpaces.append(DX::DXGIColorSpaceTypeToString(colorSpace));
      }
      CLog::LogFC(LOGDEBUG, LOGVIDEO, "Color spaces supported by the swap chain:{}", colorSpaces);
    }

    // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
    // ensures that the application will only render after each VSync, minimizing power consumption.
    ComPtr<IDXGIDevice1> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice); CHECK_ERR();
    dxgiDevice->SetMaximumFrameLatency(1);

    if (m_IsHDROutput)
      SetHdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
  }

  CLog::LogF(LOGDEBUG, "end resize buffers.");
}

// These resources need to be recreated every time the window size is changed.
void DX::DeviceResources::CreateWindowSizeDependentResources()
{
  ReleaseBackBuffer();

  DestroySwapChain();

  if (!m_dxgiFactory->IsCurrent()) // HDR toggling requires re-create factory
    CreateFactory();

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

  CLog::LogF(LOGDEBUG, "receive changing logical size to {:f} x {:f}", width, height);

  if (m_logicalSize.Width != width || m_logicalSize.Height != height)
  {
    CLog::LogF(LOGDEBUG, "change logical size to {:f} x {:f}", width, height);

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
    // to use the device anymore, tell all resources about this.
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

  DestroySwapChain();

  CreateDeviceResources();
  UpdateRenderTargetSize();
  ResizeBuffers();

  if (backbuferExists)
    CreateBackBuffer();

  if (m_deviceNotify != nullptr)
    m_deviceNotify->OnDXDeviceRestored();
  OnDeviceRestored();

  if (removed)
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                               "ReloadSkin");
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
  DXGI_PRESENT_PARAMETERS parameters = {};
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

void DX::DeviceResources::HandleOutputChange(const std::function<bool(DXGI_OUTPUT_DESC)>& cmpFunc)
{
  DXGI_ADAPTER_DESC currentDesc = {};
  DXGI_ADAPTER_DESC foundDesc = {};

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

void DX::DeviceResources::CheckNV12SharedTexturesSupport()
{
  if (m_d3dFeatureLevel < D3D_FEATURE_LEVEL_10_0 ||
      CSysInfo::GetWindowsDeviceFamily() != CSysInfo::Desktop)
    return;

  D3D11_FEATURE_DATA_D3D11_OPTIONS4 op4 = {};
  HRESULT hr = m_d3dDevice->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS4, &op4, sizeof(op4));
  m_NV12SharedTexturesSupport = SUCCEEDED(hr) && !!op4.ExtendedNV12SharedTextureSupported;
  CLog::LogF(LOGINFO, "extended NV12 shared textures is{}supported",
             m_NV12SharedTexturesSupport ? " " : " NOT ");
}

void DX::DeviceResources::CheckDXVA2SharedDecoderSurfaces()
{
  if (CSysInfo::GetWindowsDeviceFamily() != CSysInfo::Desktop)
    return;

  VideoDriverInfo driver = GetVideoDriverVersion();

  if (!m_NV12SharedTexturesSupport)
    return;

  const DXGI_ADAPTER_DESC ad = GetAdapterDesc();

  m_DXVA2SharedDecoderSurfaces =
      ad.VendorId == PCIV_Intel ||
      (ad.VendorId == PCIV_NVIDIA && driver.valid && driver.majorVersion >= 465) ||
      (ad.VendorId == PCIV_AMD && driver.valid && driver.majorVersion >= 30 &&
       m_d3dFeatureLevel >= D3D_FEATURE_LEVEL_12_1);

  CLog::LogF(LOGINFO, "DXVA2 shared decoder surfaces is{}supported",
             m_DXVA2SharedDecoderSurfaces ? " " : " NOT ");

  m_DXVA2UseFence = m_DXVA2SharedDecoderSurfaces &&
                    (ad.VendorId == PCIV_NVIDIA || ad.VendorId == PCIV_AMD) &&
                    CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10_1703);

  if (m_DXVA2SharedDecoderSurfaces)
    CLog::LogF(LOGINFO, "DXVA2 shared decoder surfaces {} fence synchronization.",
               m_DXVA2UseFence ? "WITH" : "WITHOUT");

  m_DXVASuperResolutionSupport =
      m_d3dFeatureLevel >= D3D_FEATURE_LEVEL_12_1 &&
      ((ad.VendorId == PCIV_Intel && driver.valid && driver.majorVersion >= 31) ||
       (ad.VendorId == PCIV_NVIDIA && driver.valid && driver.majorVersion >= 530));

  if (m_DXVASuperResolutionSupport)
    CLog::LogF(LOGINFO, "DXVA Video Super Resolution is potentially supported");
}

VideoDriverInfo DX::DeviceResources::GetVideoDriverVersion()
{
  const DXGI_ADAPTER_DESC ad = GetAdapterDesc();

  VideoDriverInfo driver = CWIN32Util::GetVideoDriverInfo(ad.VendorId, ad.Description);

  if (ad.VendorId == PCIV_NVIDIA)
    CLog::LogF(LOGINFO, "video driver version is {} {}.{} ({})", GetGFXProviderName(ad.VendorId),
               driver.majorVersion, driver.minorVersion, driver.version);
  else
    CLog::LogF(LOGINFO, "video driver version is {} {}", GetGFXProviderName(ad.VendorId),
               driver.version);

  return driver;
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

void DX::DeviceResources::SetHdrMetaData(DXGI_HDR_METADATA_HDR10& hdr10) const
{
  ComPtr<IDXGISwapChain4> swapChain4;

  if (!m_swapChain)
    return;

  if (SUCCEEDED(m_swapChain.As(&swapChain4)))
  {
    if (SUCCEEDED(swapChain4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(hdr10), &hdr10)))
    {
      CLog::LogF(LOGDEBUG,
                 "(raw) RP {} {} | GP {} {} | BP {} {} | WP {} {} | Max ML {} | min ML "
                 "{} | Max CLL {} | Max FALL {}",
                 hdr10.RedPrimary[0], hdr10.RedPrimary[1], hdr10.GreenPrimary[0],
                 hdr10.GreenPrimary[1], hdr10.BluePrimary[0], hdr10.BluePrimary[1],
                 hdr10.WhitePoint[0], hdr10.WhitePoint[1], hdr10.MaxMasteringLuminance,
                 hdr10.MinMasteringLuminance, hdr10.MaxContentLightLevel,
                 hdr10.MaxFrameAverageLightLevel);

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

      CLog::LogF(LOGINFO,
                 "RP {:.3f} {:.3f} | GP {:.3f} {:.3f} | BP {:.3f} {:.3f} | WP {:.3f} "
                 "{:.3f} | Max ML {:.0f} | min ML {:.4f} | Max CLL {} | Max FALL {}",
                 RP_0, RP_1, GP_0, GP_1, BP_0, BP_1, WP_0, WP_1, Max_ML, min_ML,
                 hdr10.MaxContentLightLevel, hdr10.MaxFrameAverageLightLevel);
    }
    else
    {
      CLog::LogF(LOGERROR, "DXGI SetHDRMetaData failed");
    }
  }
}

void DX::DeviceResources::SetHdrColorSpace(const DXGI_COLOR_SPACE_TYPE colorSpace)
{
  ComPtr<IDXGISwapChain3> swapChain3;

  if (!m_swapChain)
    return;

  if (SUCCEEDED(m_swapChain.As(&swapChain3)))
  {
    if (SUCCEEDED(swapChain3->SetColorSpace1(colorSpace)))
    {
      m_IsTransferPQ = (colorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);

      if (m_IsTransferPQ)
        DX::Windowing()->CacheSystemSdrPeakLuminance();

      CLog::LogF(LOGDEBUG, "DXGI SetColorSpace1 {} success",
                 DX::DXGIColorSpaceTypeToString(colorSpace));
    }
    else
    {
      CLog::LogF(LOGERROR, "DXGI SetColorSpace1 {} failed",
                 DX::DXGIColorSpaceTypeToString(colorSpace));
    }
  }
}

HDR_STATUS DX::DeviceResources::ToggleHDR()
{
  DXGI_MODE_DESC md = {};
  GetDisplayMode(&md);

  DX::Windowing()->SetTogglingHDR(true);
  DX::Windowing()->SetAlteringWindow(true);

  // Toggle display HDR
  HDR_STATUS hdrStatus = CWIN32Util::ToggleWindowsHDR(md);

  // Kill swapchain
  if (m_swapChain && hdrStatus != HDR_STATUS::HDR_TOGGLE_FAILED)
  {
    CLog::LogF(LOGDEBUG, "Re-create swapchain due HDR <-> SDR switch");
    DestroySwapChain();
  }

  DX::Windowing()->SetAlteringWindow(false);

  // Re-create swapchain
  if (hdrStatus != HDR_STATUS::HDR_TOGGLE_FAILED)
  {
    CreateWindowSizeDependentResources();

    DX::Windowing()->NotifyAppFocusChange(true);
  }

  return hdrStatus;
}

void DX::DeviceResources::ApplyDisplaySettings()
{
  CLog::LogF(LOGDEBUG, "Re-create swapchain due Display Settings changed");

  DestroySwapChain();
  CreateWindowSizeDependentResources();
}

DEBUG_INFO_RENDER DX::DeviceResources::GetDebugInfo() const
{
  if (!m_swapChain)
    return {};

  DXGI_SWAP_CHAIN_DESC1 desc = {};
  m_swapChain->GetDesc1(&desc);

  DXGI_MODE_DESC md = {};
  GetDisplayMode(&md);

  const int bits = (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM) ? 10 : 8;
  const int max = (desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM) ? 1024 : 256;
  const int range_min = DX::Windowing()->UseLimitedColor() ? (max * 16) / 256 : 0;
  const int range_max = DX::Windowing()->UseLimitedColor() ? (max * 235) / 256 : max - 1;

  DEBUG_INFO_RENDER info;

  info.renderFlags = StringUtils::Format(
      "Swapchain: {} buffers, flip {}, {}, EOTF: {} (Windows HDR {})", desc.BufferCount,
      (desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD) ? "discard" : "sequential",
      Windowing()->IsFullScreen()
          ? ((desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH) ? "fullscreen exclusive"
                                                                   : "fullscreen windowed")
          : "windowed screen",
      m_IsTransferPQ ? "PQ" : "SDR", m_IsHDROutput ? "on" : "off");

  info.videoOutput = StringUtils::Format(
      "Surfaces: {}x{}{} @ {:.3f} Hz, pixel: {} {}-bit, range: {} ({}-{})", desc.Width, desc.Height,
      (md.ScanlineOrdering > DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE) ? "i" : "p",
      static_cast<double>(md.RefreshRate.Numerator) /
          static_cast<double>(md.RefreshRate.Denominator),
      DXGIFormatToShortString(desc.Format), bits,
      DX::Windowing()->UseLimitedColor() ? "limited" : "full", range_min, range_max);

  return info;
}

std::vector<DXGI_COLOR_SPACE_TYPE> DX::DeviceResources::GetSwapChainColorSpaces() const
{
  if (!m_swapChain)
    return {};

  std::vector<DXGI_COLOR_SPACE_TYPE> result;
  HRESULT hr;

  ComPtr<IDXGISwapChain3> swapChain3;
  if (SUCCEEDED(hr = m_swapChain.As(&swapChain3)))
  {
    UINT colorSpaceSupport = 0;
    for (UINT colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
         colorSpace < DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020; colorSpace++)
    {
      DXGI_COLOR_SPACE_TYPE cs = static_cast<DXGI_COLOR_SPACE_TYPE>(colorSpace);
      if (SUCCEEDED(swapChain3->CheckColorSpaceSupport(cs, &colorSpaceSupport)) &&
          (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        result.push_back(cs);
    }
  }
  else
  {
    CLog::LogF(LOGDEBUG, "IDXGISwapChain3 is not available. Error {}", DX::GetErrorDescription(hr));
  }
  return result;
}

bool DX::DeviceResources::SetMultithreadProtected(bool enabled) const
{
  BOOL wasEnabled = FALSE;
  ComPtr<ID3D11Multithread> multithread;
  HRESULT hr = m_d3dDevice.As(&multithread);
  if (SUCCEEDED(hr))
    wasEnabled = multithread->SetMultithreadProtected(enabled ? TRUE : FALSE);

  return (wasEnabled == TRUE ? true : false);
}
