/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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


#ifdef HAS_DX

#include <DirectXPackedVector.h>
#include "Application.h"
#include "RenderSystemDX.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "guilib/D3DResource.h"
#include "guilib/GUIShaderDX.h"
#include "guilib/GUITextureD3D.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "platform/win32/dxerr.h"
#include "utils/SystemInfo.h"
#pragma warning(disable: 4091)
#include <d3d10umddi.h>
#pragma warning(default: 4091)
#include <algorithm>

#ifndef _M_X64
#pragma comment(lib, "EasyHook32.lib")
#else
#pragma comment(lib, "EasyHook64.lib")
#endif
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#define RATIONAL_TO_FLOAT(rational) ((rational.Denominator != 0) ? \
 static_cast<float>(rational.Numerator) / static_cast<float>(rational.Denominator) : 0.0f)

// User Mode Driver hooks definitions
void APIENTRY HookCreateResource(D3D10DDI_HDEVICE hDevice, const D3D10DDIARG_CREATERESOURCE* pResource, D3D10DDI_HRESOURCE hResource, D3D10DDI_HRTRESOURCE hRtResource);
HRESULT APIENTRY HookCreateDevice(D3D10DDI_HADAPTER hAdapter, D3D10DDIARG_CREATEDEVICE* pCreateData);
HRESULT APIENTRY HookOpenAdapter10_2(D3D10DDIARG_OPENADAPTER *pOpenData);
static PFND3D10DDI_OPENADAPTER s_fnOpenAdapter10_2{ nullptr };
static PFND3D10DDI_CREATEDEVICE s_fnCreateDeviceOrig{ nullptr };
static PFND3D10DDI_CREATERESOURCE s_fnCreateResourceOrig{ nullptr };
CRenderSystemDX* s_windowing{ nullptr };

using namespace DirectX::PackedVector;

CRenderSystemDX::CRenderSystemDX() : CRenderSystemBase()
{
  m_enumRenderingSystem = RENDERING_SYSTEM_DIRECTX;

  m_bVSync = true;

  ZeroMemory(&m_cachedMode, sizeof(m_cachedMode));
  ZeroMemory(&m_viewPort, sizeof(m_viewPort));
  ZeroMemory(&m_scissor, sizeof(CRect));
  ZeroMemory(&m_adapterDesc, sizeof(DXGI_ADAPTER_DESC));
  s_windowing = this;
}

CRenderSystemDX::~CRenderSystemDX()
{
  s_windowing = nullptr;
}

bool CRenderSystemDX::InitRenderSystem()
{
  m_bVSync = true;
  
  CLog::Log(LOGDEBUG, __FUNCTION__" - Initializing D3D11 Factory...");

  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&m_dxgiFactory));
  if (FAILED(hr)) 
  {
    CLog::Log(LOGERROR, __FUNCTION__" - D3D11 initialization failed.");
    return false;
  }
  
  UpdateMonitor();
  return CreateDevice();
}

void CRenderSystemDX::SetRenderParams(unsigned int width, unsigned int height, bool fullScreen, float refreshRate)
{
  m_nBackBufferWidth  = width;
  m_nBackBufferHeight = height;
  m_bFullScreenDevice = fullScreen;
  m_refreshRate       = refreshRate;
}

void CRenderSystemDX::SetMonitor(HMONITOR monitor)
{
  if (!m_dxgiFactory)
    return;

  DXGI_OUTPUT_DESC outputDesc;
  if (m_pOutput && SUCCEEDED(m_pOutput->GetDesc(&outputDesc)) && outputDesc.Monitor == monitor)
    return;

  // find the appropriate screen
  IDXGIAdapter1*  pAdapter;
  for (unsigned i = 0; m_dxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    pAdapter->GetDesc1(&adapterDesc);

    IDXGIOutput* pOutput;
    for (unsigned j = 0; pAdapter->EnumOutputs(j, &pOutput) != DXGI_ERROR_NOT_FOUND; ++j)
    {
      pOutput->GetDesc(&outputDesc);
      if (outputDesc.Monitor == monitor)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Selected %S output. ", outputDesc.DeviceName);

        // update monitor info
        SAFE_RELEASE(m_pOutput);
        m_pOutput = pOutput;

        // check if adapter is changed
        if ( m_adapterDesc.AdapterLuid.HighPart != adapterDesc.AdapterLuid.HighPart 
          || m_adapterDesc.AdapterLuid.LowPart != adapterDesc.AdapterLuid.LowPart)
        {
          CLog::Log(LOGDEBUG, __FUNCTION__" - Selected %S adapter. ", adapterDesc.Description);

          pAdapter->GetDesc(&m_adapterDesc);
          SAFE_RELEASE(m_adapter);
          m_adapter = pAdapter;
          m_needNewDevice = true;

          // adapter is changed, (re)init hooks into new driver
          InitHooks();
          return;
        }

        return;
      }
      pOutput->Release();
    }
    pAdapter->Release();
  }
}

bool CRenderSystemDX::ResetRenderSystem(int width, int height, bool fullScreen, float refreshRate)
{
  if (!m_pD3DDev)
    return false;

  if (m_hDeviceWnd != nullptr)
  {
    HMONITOR hMonitor = MonitorFromWindow(m_hDeviceWnd, MONITOR_DEFAULTTONULL);
    if (hMonitor)
      SetMonitor(hMonitor);
  }

  SetRenderParams(width, height, fullScreen, refreshRate);

  CRect rc(0, 0, float(width), float(height));
  SetViewPort(rc);

  if (!m_needNewDevice)
  {
    SetFullScreenInternal();
    CreateWindowSizeDependentResources();
  }
  else 
  {
    OnDeviceLost();
    OnDeviceReset();
  }

  return true;
}

void CRenderSystemDX::OnMove()
{
  if (!m_bRenderCreated)
    return;

  DXGI_OUTPUT_DESC outputDesc;
  m_pOutput->GetDesc(&outputDesc);
  HMONITOR newMonitor = MonitorFromWindow(m_hDeviceWnd, MONITOR_DEFAULTTONULL);

  if (newMonitor != nullptr && outputDesc.Monitor != newMonitor)
  {
    SetMonitor(newMonitor);
    if (m_needNewDevice)
    {
      CLog::Log(LOGDEBUG, "%s - Adapter changed, reseting render system.", __FUNCTION__);
      ResetRenderSystem(m_nBackBufferWidth, m_nBackBufferHeight, m_bFullScreenDevice, m_refreshRate);
    }
  }
}

void CRenderSystemDX::OnResize(unsigned int width, unsigned int height)
{
  if (!m_bRenderCreated)
    return;

  m_nBackBufferWidth = width;
  m_nBackBufferHeight = height;
  CreateWindowSizeDependentResources();
}

void CRenderSystemDX::GetClosestDisplayModeToCurrent(IDXGIOutput* output, DXGI_MODE_DESC* outCurrentDisplayMode, bool useCached /*= false*/)
{
  DXGI_OUTPUT_DESC outputDesc;
  output->GetDesc(&outputDesc);
  HMONITOR hMonitor = outputDesc.Monitor;
  MONITORINFOEX monitorInfo;
  monitorInfo.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &monitorInfo);
  DEVMODE devMode;
  devMode.dmSize = sizeof(DEVMODE);
  devMode.dmDriverExtra = 0;
  EnumDisplaySettings(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode);

  bool useDefaultRefreshRate = 1 == devMode.dmDisplayFrequency || 0 == devMode.dmDisplayFrequency;
  float refreshRate = RATIONAL_TO_FLOAT(m_cachedMode.RefreshRate);

  // this needed to improve performance for VideoSync bacause FindClosestMatchingMode is very slow
  if (!useCached
    || m_cachedMode.Width  != devMode.dmPelsWidth
    || m_cachedMode.Height != devMode.dmPelsHeight
    || long(refreshRate)   != devMode.dmDisplayFrequency)
  {
    DXGI_MODE_DESC current;
    current.Width = devMode.dmPelsWidth;
    current.Height = devMode.dmPelsHeight;
    current.RefreshRate.Numerator = 0;
    current.RefreshRate.Denominator = 0;
    if (!useDefaultRefreshRate)
      GetRefreshRatio(devMode.dmDisplayFrequency, &current.RefreshRate.Numerator, &current.RefreshRate.Denominator);
    current.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    current.ScanlineOrdering = m_interlaced ? DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST : DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    current.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    output->FindClosestMatchingMode(&current, &m_cachedMode, m_pD3DDev);
  }

  ZeroMemory(outCurrentDisplayMode, sizeof(DXGI_MODE_DESC));
  outCurrentDisplayMode->Width = m_cachedMode.Width;
  outCurrentDisplayMode->Height = m_cachedMode.Height;
  outCurrentDisplayMode->RefreshRate.Numerator = m_cachedMode.RefreshRate.Numerator;
  outCurrentDisplayMode->RefreshRate.Denominator = m_cachedMode.RefreshRate.Denominator;
  outCurrentDisplayMode->Format = m_cachedMode.Format;
  outCurrentDisplayMode->ScanlineOrdering = m_cachedMode.ScanlineOrdering;
  outCurrentDisplayMode->Scaling = m_cachedMode.Scaling;
}

void CRenderSystemDX::GetDisplayMode(DXGI_MODE_DESC *mode, bool useCached /*= false*/)
{
  GetClosestDisplayModeToCurrent(m_pOutput, mode, useCached);
}

inline void DXWait(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
  ID3D11Query* wait = nullptr;
  CD3D11_QUERY_DESC qd(D3D11_QUERY_EVENT);
  if (SUCCEEDED(pDevice->CreateQuery(&qd, &wait)))
  {
    pContext->End(wait);
    while (S_FALSE == pContext->GetData(wait, nullptr, 0, 0))
      Sleep(1);
  }
  SAFE_RELEASE(wait);
}

void CRenderSystemDX::SetFullScreenInternal()
{
  if (!m_bRenderCreated)
    return;

  HRESULT hr = S_OK;
  BOOL bFullScreen;
  m_pSwapChain->GetFullscreenState(&bFullScreen, nullptr);

  // full-screen to windowed translation. Only change FS state and return
  if (!!bFullScreen && m_useWindowedDX)
  {
    CLog::Log(LOGDEBUG, "%s - Switching swap chain to windowed mode.", __FUNCTION__);

    OnDisplayLost();
    hr = m_pSwapChain->SetFullscreenState(false, nullptr);
    if (SUCCEEDED(hr))
      m_bResizeRequred = true;
    else
      CLog::Log(LOGERROR, "%s - Failed switch full screen state: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
  }
  // true full-screen
  else if (m_bFullScreenDevice && !m_useWindowedDX)
  {
    IDXGIOutput* pOutput = nullptr;
    m_pSwapChain->GetContainingOutput(&pOutput);

    DXGI_OUTPUT_DESC trgDesc, currDesc;
    m_pOutput->GetDesc(&trgDesc);
    pOutput->GetDesc(&currDesc);

    if (trgDesc.Monitor != currDesc.Monitor || !bFullScreen)
    {
      // swap chain requires to change FS mode after resize or transition from windowed to full-screen.
      CLog::Log(LOGDEBUG, "%s - Switching swap chain to fullscreen state.", __FUNCTION__);

      OnDisplayLost();
      hr = m_pSwapChain->SetFullscreenState(true, m_pOutput);
      if (SUCCEEDED(hr))
        m_bResizeRequred = true;
      else
        CLog::Log(LOGERROR, "%s - Failed switch full screen state: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
    }
    SAFE_RELEASE(pOutput);

    // do not change modes if hw stereo enabled
    if (m_bHWStereoEnabled)
      goto end;

    DXGI_SWAP_CHAIN_DESC scDesc;
    m_pSwapChain->GetDesc(&scDesc);

    DXGI_MODE_DESC currentMode, // closest to current mode
      toMatchMode,              // required mode
      matchedMode;              // closest to required mode

    // find current mode on target output
    GetClosestDisplayModeToCurrent(m_pOutput, &currentMode);

    float currentRefreshRate = RATIONAL_TO_FLOAT(currentMode.RefreshRate);
    CLog::Log(LOGDEBUG, "%s - Current display mode is: %dx%d@%0.3f", __FUNCTION__, currentMode.Width, currentMode.Height, currentRefreshRate);

    // use backbuffer dimention to find required display mode
    toMatchMode.Width = m_nBackBufferWidth;
    toMatchMode.Height = m_nBackBufferHeight;
    bool useDefaultRefreshRate = 0 == m_refreshRate;
    toMatchMode.RefreshRate.Numerator = 0;
    toMatchMode.RefreshRate.Denominator = 0;
    if (!useDefaultRefreshRate)
      GetRefreshRatio(static_cast<uint32_t>(m_refreshRate), &toMatchMode.RefreshRate.Numerator, &toMatchMode.RefreshRate.Denominator);
    toMatchMode.Format = scDesc.BufferDesc.Format;
    toMatchMode.Scaling = scDesc.BufferDesc.Scaling;
    toMatchMode.ScanlineOrdering = scDesc.BufferDesc.ScanlineOrdering;

    // find closest mode
    m_pOutput->FindClosestMatchingMode(&toMatchMode, &matchedMode, m_pD3DDev);

    float matchedRefreshRate = RATIONAL_TO_FLOAT(matchedMode.RefreshRate);
    CLog::Log(LOGDEBUG, "%s - Found matched mode: %dx%d@%0.3f", __FUNCTION__, matchedMode.Width, matchedMode.Height, matchedRefreshRate);
    // FindClosestMatchingMode doesn't return "fixed" modes, so wee need to check deviation and force switching mode
    float diff = fabs(matchedRefreshRate - m_refreshRate) / matchedRefreshRate;
    // change mode if required (current != required)
    if ( currentMode.Width != matchedMode.Width
      || currentMode.Height != matchedMode.Height
      || currentRefreshRate != matchedRefreshRate
      || diff > 0.0005)
    {
      // change monitor resolution (in fullscreen mode) to required mode
      CLog::Log(LOGDEBUG, "%s - Switching mode to %dx%d@%0.3f.", __FUNCTION__, matchedMode.Width, matchedMode.Height, matchedRefreshRate);

      if (!m_bResizeRequred)
        OnDisplayLost();

      hr = m_pSwapChain->ResizeTarget(&matchedMode);
      if (SUCCEEDED(hr))
        m_bResizeRequred = true;
      else
        CLog::Log(LOGERROR, "%s - Failed to switch output mode: %s", __FUNCTION__, GetErrorDescription(hr).c_str());
    }
  }
end:
  SetMaximumFrameLatency();
}

bool CRenderSystemDX::IsFormatSupport(DXGI_FORMAT format, unsigned int usage)
{
  UINT supported;
  m_pD3DDev->CheckFormatSupport(format, &supported);
  return (supported & usage) != 0;
}

bool CRenderSystemDX::DestroyRenderSystem()
{
  UninitHooks();
  DeleteDevice();

  // restore stereo setting on exit
  if (g_advancedSettings.m_useDisplayControlHWStereo)
    SetDisplayStereoEnabled(m_bDefaultStereoEnabled);

  SAFE_RELEASE(m_pOutput);
  SAFE_RELEASE(m_adapter);
  SAFE_RELEASE(m_dxgiFactory);
  return true;
}

void CRenderSystemDX::DeleteDevice()
{
  CSingleLock lock(m_resourceSection);

  if (m_pGUIShader)
    m_pGUIShader->End();

  // tell any shared resources
  for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnDestroyDevice();

  if (m_pSwapChain)
    m_pSwapChain->SetFullscreenState(false, nullptr);

  SAFE_DELETE(m_pGUIShader);
  SAFE_RELEASE(m_pTextureRight);
  SAFE_RELEASE(m_pRenderTargetViewRight);
  SAFE_RELEASE(m_pShaderResourceViewRight);
  SAFE_RELEASE(m_BlendEnableState);
  SAFE_RELEASE(m_BlendDisableState);
  SAFE_RELEASE(m_RSScissorDisable);
  SAFE_RELEASE(m_RSScissorEnable);
  SAFE_RELEASE(m_depthStencilState);
  SAFE_RELEASE(m_depthStencilView);
  SAFE_RELEASE(m_pRenderTargetView);
  if (m_pContext && m_pContext != m_pImdContext)
  {
    m_pContext->ClearState();
    m_pContext->Flush();
    SAFE_RELEASE(m_pContext);
  }
  if (m_pImdContext)
  {
    m_pImdContext->ClearState();
    m_pImdContext->Flush();
    SAFE_RELEASE(m_pImdContext);
  }
  SAFE_RELEASE(m_pSwapChain);
  SAFE_RELEASE(m_pSwapChain1);
  SAFE_RELEASE(m_pD3DDev);
#ifdef _DEBUG
  if (m_d3dDebug)
  {
    m_d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);
    SAFE_RELEASE(m_d3dDebug);
  }
#endif
  m_bResizeRequred = false;
  m_bHWStereoEnabled = false;
  m_bRenderCreated = false;
  m_bStereoEnabled = false;
}

void CRenderSystemDX::OnDeviceLost()
{
  CSingleLock lock(m_resourceSection);
  g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_LOST);

  OnDisplayLost();

  if (m_needNewDevice)
    DeleteDevice();
  else
  {
    // just resetting the device
    for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnLostDevice();
  }
}

void CRenderSystemDX::OnDeviceReset()
{
  CSingleLock lock(m_resourceSection);

  if (m_needNewDevice)
    CreateDevice();
  
  if (m_bRenderCreated)
  {
    // we're back
    for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDevice();

    g_windowManager.SendMessage(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_RENDERER_RESET);
  }

  OnDisplayReset();
}

bool CRenderSystemDX::CreateDevice()
{
  CSingleLock lock(m_resourceSection);

  HRESULT hr;
  SAFE_RELEASE(m_pD3DDev);

  D3D_FEATURE_LEVEL featureLevels[] =
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1,
  };

  // the VIDEO_SUPPORT flag force lowering feature level if current env not support video on 11_1
  UINT createDeviceFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT; 
#ifdef _DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_DRIVER_TYPE driverType = m_adapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN;
  // we should specify D3D_DRIVER_TYPE_UNKNOWN if create device with specified adapter.
  hr = D3D11CreateDevice(m_adapter, driverType, nullptr, createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels),
                         D3D11_SDK_VERSION, &m_pD3DDev, &m_featureLevel, &m_pImdContext);

  if (FAILED(hr))
  {
    // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
    CLog::Log(LOGDEBUG, "%s - First try to create device failed with error: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
    CLog::Log(LOGDEBUG, "%s - Trying to create device with lowest feature level: %#x.", __FUNCTION__, featureLevels[1]);

    hr = D3D11CreateDevice(m_adapter, driverType, nullptr, createDeviceFlags, &featureLevels[1], ARRAYSIZE(featureLevels) - 1,
                           D3D11_SDK_VERSION, &m_pD3DDev, &m_featureLevel, &m_pImdContext);
    if (FAILED(hr))
    {
      // still failed. seems driver doesn't support video API acceleration, try without VIDEO_SUPPORT flag
      CLog::Log(LOGDEBUG, "%s - Next try to create device failed with error: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
      CLog::Log(LOGDEBUG, "%s - Trying to create device without video API support.", __FUNCTION__);

      createDeviceFlags &= ~D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
      hr = D3D11CreateDevice(m_adapter, driverType, nullptr, createDeviceFlags, &featureLevels[1], ARRAYSIZE(featureLevels) - 1,
                             D3D11_SDK_VERSION, &m_pD3DDev, &m_featureLevel, &m_pImdContext);
      if (SUCCEEDED(hr))
        CLog::Log(LOGNOTICE, "%s - Your video driver doesn't support DirectX 11 Video Acceleration API. Application is not be able to use hardware video processing and decoding", __FUNCTION__);
    }
  }

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s - D3D11 device creation failure with error %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
    return false;
  }

  if (!m_adapter)
  {
    // get adapter from device if it was not detected previously
    IDXGIDevice1* pDXGIDevice = nullptr;
    if (SUCCEEDED(m_pD3DDev->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&pDXGIDevice))))
    {
      IDXGIAdapter *pAdapter = nullptr;
      if (SUCCEEDED(pDXGIDevice->GetAdapter(&pAdapter)))
      {
        hr = pAdapter->QueryInterface(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&m_adapter));
        SAFE_RELEASE(pAdapter);
        if (FAILED(hr))
          return false;

        m_adapter->GetDesc(&m_adapterDesc);
        CLog::Log(LOGDEBUG, __FUNCTION__" - Selected %S adapter. ", m_adapterDesc.Description);
      }
      SAFE_RELEASE(pDXGIDevice);
    }
  }

  if (!m_adapter)
  {
    CLog::Log(LOGERROR, "%s - Failed to find adapter.", __FUNCTION__);
    return false;
  }

  SAFE_RELEASE(m_dxgiFactory);
  m_adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&m_dxgiFactory));

  if (g_advancedSettings.m_useDisplayControlHWStereo)
    UpdateDisplayStereoStatus(true);

  if (!m_pOutput)
  {
    HMONITOR hMonitor = MonitorFromWindow(m_hDeviceWnd, MONITOR_DEFAULTTONULL);
    SetMonitor(hMonitor);
  }

  if ( g_advancedSettings.m_bAllowDeferredRendering 
    && FAILED(m_pD3DDev->CreateDeferredContext(0, &m_pContext)))
  {
    CLog::Log(LOGERROR, "%s - Failed to create deferred context, deferred rendering is not possible, fallback to immediate rendering.", __FUNCTION__);
  }

  // make immediate context as default context if deferred context was not created
  if (!m_pContext)
    m_pContext = m_pImdContext;

  if (m_featureLevel < D3D_FEATURE_LEVEL_9_3)
    m_maxTextureSize = D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else if (m_featureLevel < D3D_FEATURE_LEVEL_10_0)
    m_maxTextureSize = D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else if (m_featureLevel < D3D_FEATURE_LEVEL_11_0)
    m_maxTextureSize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  else 
    // 11_x and greater feature level. Limit this size to avoid memory overheads
    m_maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION >> 1;

  // use multi-thread protection on the device context to prevent deadlock issues that can sometimes happen
  // when decoder call ID3D11VideoContext::GetDecoderBuffer or ID3D11VideoContext::ReleaseDecoderBuffer.
  ID3D10Multithread *pMultiThreading = nullptr;
  if (SUCCEEDED(m_pD3DDev->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&pMultiThreading))))
  {
    pMultiThreading->SetMultithreadProtected(true);
    pMultiThreading->Release();
  }

  SetMaximumFrameLatency();

#ifdef _DEBUG
  if (SUCCEEDED(m_pD3DDev->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&m_d3dDebug))))
  {
    ID3D11InfoQueue *d3dInfoQueue = nullptr;
    if (SUCCEEDED(m_d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&d3dInfoQueue))))
    {
      D3D11_MESSAGE_ID hide[] =
      {
        D3D11_MESSAGE_ID_GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED,        // avoid GETVIDEOPROCESSORFILTERRANGE_UNSUPPORTED (dx bug)
        D3D11_MESSAGE_ID_DEVICE_RSSETSCISSORRECTS_NEGATIVESCISSOR         // avoid warning for some labels out of screen
        // Add more message IDs here as needed
      };

      D3D11_INFO_QUEUE_FILTER filter;
      ZeroMemory(&filter, sizeof(filter));
      filter.DenyList.NumIDs = _countof(hide);
      filter.DenyList.pIDList = hide;
      d3dInfoQueue->AddStorageFilterEntries(&filter);
      d3dInfoQueue->Release();
    }
  }
#endif

  m_adapterDesc = {};
  if (SUCCEEDED(m_adapter->GetDesc(&m_adapterDesc)))
  {
    CLog::Log(LOGDEBUG, "%s - on adapter %S (VendorId: %#x DeviceId: %#x) with feature level %#x.", __FUNCTION__, 
                        m_adapterDesc.Description, m_adapterDesc.VendorId, m_adapterDesc.DeviceId, m_featureLevel);

    m_RenderRenderer = StringUtils::Format("%S", m_adapterDesc.Description);
    IDXGIFactory2* dxgiFactory2 = nullptr;
    m_dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    m_RenderVersion = StringUtils::Format("DirectX %s (FL %d.%d)", 
                                          dxgiFactory2 != nullptr ? "11.1" : "11.0", 
                                          (m_featureLevel >> 12) & 0xF, 
                                          (m_featureLevel >> 8) & 0xF);
    SAFE_RELEASE(dxgiFactory2);
  }

  m_renderCaps = 0;
  unsigned int usage = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
  if ( IsFormatSupport(DXGI_FORMAT_BC1_UNORM, usage)
    && IsFormatSupport(DXGI_FORMAT_BC2_UNORM, usage)
    && IsFormatSupport(DXGI_FORMAT_BC3_UNORM, usage))
    m_renderCaps |= RENDER_CAPS_DXT;

  // MSDN: At feature levels 9_1, 9_2 and 9_3, the display device supports the use of 2D textures with dimensions that are not powers of two under two conditions.
  // First, only one MIP-map level for each texture can be created - we are using only 1 mip level)
  // Second, no wrap sampler modes for textures are allowed - we are using clamp everywhere
  // At feature levels 10_0, 10_1 and 11_0, the display device unconditionally supports the use of 2D textures with dimensions that are not powers of two.
  // so, setup caps NPOT
  m_renderCaps |= RENDER_CAPS_NPOT;
  if ((m_renderCaps & RENDER_CAPS_DXT) != 0)
    m_renderCaps |= RENDER_CAPS_DXT_NPOT;

  // Temporary - allow limiting the caps to debug a texture problem
  if (g_advancedSettings.m_RestrictCapsMask != 0)
    m_renderCaps &= ~g_advancedSettings.m_RestrictCapsMask;

  if (m_renderCaps & RENDER_CAPS_DXT)
    CLog::Log(LOGDEBUG, "%s - RENDER_CAPS_DXT", __FUNCTION__);
  if (m_renderCaps & RENDER_CAPS_NPOT)
    CLog::Log(LOGDEBUG, "%s - RENDER_CAPS_NPOT", __FUNCTION__);
  if (m_renderCaps & RENDER_CAPS_DXT_NPOT)
    CLog::Log(LOGDEBUG, "%s - RENDER_CAPS_DXT_NPOT", __FUNCTION__);

  /* All the following quirks need to be tested
  // nVidia quirk: some NPOT DXT textures of the GUI display with corruption
  // when using D3DPOOL_DEFAULT + D3DUSAGE_DYNAMIC textures (no other choice with D3D9Ex for example)
  // most likely xbmc bug, but no hw to repro & fix properly.
  // affects lots of hw generations - 6xxx, 7xxx, GT220, ION1
  // see ticket #9269
  if (m_adapterDesc.VendorId == PCIV_nVidia)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - nVidia workaround - disabling RENDER_CAPS_DXT_NPOT");
    m_renderCaps &= ~RENDER_CAPS_DXT_NPOT;
  }

  // Intel quirk: DXT texture pitch must be > 64
  // when using D3DPOOL_DEFAULT + D3DUSAGE_DYNAMIC textures (no other choice with D3D9Ex)
  // DXT1:   32 pixels wide is the largest non-working texture width
  // DXT3/5: 16 pixels wide ----------------------------------------
  // Both equal to a pitch of 64. So far no Intel has DXT NPOT (including i3/i5/i7, so just go with the next higher POT.
  // See ticket #9578
  if (m_adapterDesc.VendorId == PCIV_Intel)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__" - Intel workaround - specifying minimum pitch for compressed textures.");
    m_minDXTPitch = 128;
  }*/

  if (!CreateStates() || !InitGUIShader() || !CreateWindowSizeDependentResources())
    return false;

  m_bRenderCreated = true;
  m_needNewDevice = false;

  // tell any shared objects about our resurrection
  for (std::vector<ID3DResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
    (*i)->OnCreateDevice();

  RestoreViewPort();

  return true;
}

bool CRenderSystemDX::CreateWindowSizeDependentResources()
{
  if (m_resizeInProgress)
    return false;

  HRESULT hr;
  DXGI_SWAP_CHAIN_DESC scDesc = { 0 };

  bool bNeedRecreate = false;
  bool bNeedResize   = false;
  bool bHWStereoEnabled = RENDER_STEREO_MODE_HARDWAREBASED == g_graphicsContext.GetStereoMode();

  if (m_pSwapChain)
  {
    m_pSwapChain->GetDesc(&scDesc);
    bNeedResize = m_bResizeRequred || 
                  m_nBackBufferWidth != scDesc.BufferDesc.Width || 
                  m_nBackBufferHeight != scDesc.BufferDesc.Height;
  }
  else
    bNeedResize = true;

  if (m_pSwapChain1)
  {
    DXGI_SWAP_CHAIN_DESC1 scDesc;
    m_pSwapChain1->GetDesc1(&scDesc);
    bNeedRecreate = (scDesc.Stereo == TRUE) != bHWStereoEnabled;
  }

  if (!bNeedRecreate && !bNeedResize)
  {
    CheckInterlasedStereoView();
    return true;
  }

  m_resizeInProgress = true;
  CLog::Log(LOGDEBUG, "%s - (Re)Create window size (%dx%d) dependent resources.", __FUNCTION__, m_nBackBufferWidth, m_nBackBufferHeight);

  bool bRestoreRTView = false;
  {
    ID3D11RenderTargetView* pRTView; ID3D11DepthStencilView* pDSView;
    m_pContext->OMGetRenderTargets(1, &pRTView, &pDSView);

    bRestoreRTView = (nullptr != pRTView || nullptr != pDSView);

    SAFE_RELEASE(pRTView);
    SAFE_RELEASE(pDSView);
  }

  m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
  FinishCommandList(false);

  SAFE_RELEASE(m_pRenderTargetView);
  SAFE_RELEASE(m_depthStencilView);
  SAFE_RELEASE(m_pRenderTargetViewRight);
  SAFE_RELEASE(m_pShaderResourceViewRight);
  SAFE_RELEASE(m_pTextureRight);

  if (bNeedRecreate)
  {
    if (!m_bResizeRequred)
    {
      OnDisplayLost();
      m_bResizeRequred = true;
    }

    BOOL fullScreen;
    m_pSwapChain1->GetFullscreenState(&fullScreen, nullptr);
    if (fullScreen)
      m_pSwapChain1->SetFullscreenState(false, nullptr);

    // disable/enable stereo 3D on system level
    if (g_advancedSettings.m_useDisplayControlHWStereo)
      SetDisplayStereoEnabled(bHWStereoEnabled);

    CLog::Log(LOGDEBUG, "%s - Destroying swapchain in order to switch %s stereoscopic 3D.", __FUNCTION__, bHWStereoEnabled ? "to" : "from");

    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pSwapChain1);
    m_pImdContext->ClearState();
    m_pImdContext->Flush();

    // flush command is asynchronous, so wait until destruction is completed
    // otherwise it can cause problems with flip presentation model swap chains.
    DXWait(m_pD3DDev, m_pImdContext);
  }

  uint32_t scFlags = m_useWindowedDX ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  if (!m_pSwapChain)
  {
    CLog::Log(LOGDEBUG, "%s - Creating swapchain in %s mode.", __FUNCTION__, bHWStereoEnabled ? "Stereoscopic 3D" : "Mono");

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = m_dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (SUCCEEDED(hr) && dxgiFactory2)
    {
      // DirectX 11.1 or later
      DXGI_SWAP_CHAIN_DESC1 scDesc1 = { 0 };
      scDesc1.Width       = m_nBackBufferWidth;
      scDesc1.Height      = m_nBackBufferHeight;
      scDesc1.BufferCount = 3 * (1 + bHWStereoEnabled);
      scDesc1.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
      scDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      scDesc1.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
      scDesc1.Stereo      = bHWStereoEnabled;
      scDesc1.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      scDesc1.Flags       = scFlags;

      scDesc1.SampleDesc.Count = 1;
      scDesc1.SampleDesc.Quality = 0;

      DXGI_SWAP_CHAIN_FULLSCREEN_DESC scFSDesc = { 0 };
      scFSDesc.ScanlineOrdering = m_interlaced ? DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST : DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
      scFSDesc.Windowed         = m_useWindowedDX;

      hr = dxgiFactory2->CreateSwapChainForHwnd(m_pD3DDev, m_hFocusWnd, &scDesc1, &scFSDesc, nullptr, &m_pSwapChain1);

      // some drivers (AMD) are denied to switch in stereoscopic 3D mode, if so then fallback to mono mode
      if (FAILED(hr) && bHWStereoEnabled)
      {
        // switch to stereo mode failed, create mono swapchain
        CLog::Log(LOGERROR, "%s - Creating stereo swap chain failed with error: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
        CLog::Log(LOGNOTICE, "%s - Fallback to monoscopic mode.", __FUNCTION__);

        scDesc1.Stereo = false;
        bHWStereoEnabled = false;
        hr = dxgiFactory2->CreateSwapChainForHwnd(m_pD3DDev, m_hFocusWnd, &scDesc1, &scFSDesc, nullptr, &m_pSwapChain1);

        // fallback to split_horisontal mode.
        g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_SPLIT_HORIZONTAL);
      }

      if (SUCCEEDED(hr))
      {
        m_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&m_pSwapChain));
        // this hackish way to disable stereo in windowed mode:
        // - restart presenting, 0 in sync interval discards current frame also
        // - wait until new frame will be drawn
        // sleep value possible depends on hardware m.b. need a setting in as.xml
        if (!g_advancedSettings.m_useDisplayControlHWStereo && m_useWindowedDX && !bHWStereoEnabled && m_bHWStereoEnabled)
        {
          m_pSwapChain1->Present(0, DXGI_PRESENT_RESTART);
          Sleep(100);
        }
        m_bHWStereoEnabled = bHWStereoEnabled;
      }
      dxgiFactory2->Release();
    }
    else
    {
      // DirectX 11.0 systems
      scDesc.BufferCount  = 3;
      scDesc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      scDesc.OutputWindow = m_hFocusWnd;
      scDesc.Windowed     = m_useWindowedDX;
      scDesc.SwapEffect   = DXGI_SWAP_EFFECT_SEQUENTIAL;
      scDesc.Flags        = scFlags;

      scDesc.BufferDesc.Width   = m_nBackBufferWidth;
      scDesc.BufferDesc.Height  = m_nBackBufferHeight;
      scDesc.BufferDesc.Format  = DXGI_FORMAT_B8G8R8A8_UNORM;
      scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
      scDesc.BufferDesc.ScanlineOrdering = m_interlaced ? DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST : DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
      scDesc.SampleDesc.Count = 1;
      scDesc.SampleDesc.Quality = 0;

      hr = m_dxgiFactory->CreateSwapChain(m_pD3DDev, &scDesc, &m_pSwapChain);
    }

    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "%s - Creating swap chain failed with error: %s.", __FUNCTION__, GetErrorDescription(hr).c_str());
      m_bRenderCreated = false;
      return false;
    }

    // tell DXGI to not interfere with application's handling of window mode changes
    m_dxgiFactory->MakeWindowAssociation(m_hFocusWnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
  }
  else
  {
    // resize swap chain buffers with preserving the existing buffer count and format.
    hr = m_pSwapChain->ResizeBuffers(scDesc.BufferCount, m_nBackBufferWidth, m_nBackBufferHeight, scDesc.BufferDesc.Format, scFlags);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "%s - Failed to resize buffers (%s).", __FUNCTION__, GetErrorDescription(hr).c_str());
      if (DXGI_ERROR_DEVICE_REMOVED == hr)
      {
        OnDeviceLost();
        return false;
      }
      // wait a bit and try again
      Sleep(50);
      hr = m_pSwapChain->ResizeBuffers(scDesc.BufferCount, m_nBackBufferWidth, m_nBackBufferHeight, scDesc.BufferDesc.Format, scFlags);
    }
  }

  // Create a render target view
  ID3D11Texture2D* pBackBuffer = nullptr;
  hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s - Failed to get back buffer (%s).", __FUNCTION__, GetErrorDescription(hr).c_str());
    return false;
  }

  // Create a view interface on the rendertarget to use on bind for mono or left eye view.
  CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_UNKNOWN, 0, 0, 1);
  hr = m_pD3DDev->CreateRenderTargetView(pBackBuffer, &rtDesc, &m_pRenderTargetView);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s - Failed to create render target view (%s).", __FUNCTION__, GetErrorDescription(hr).c_str());
    pBackBuffer->Release();
    return false;
  }

  if (m_bHWStereoEnabled)
  {
    // Stereo swapchains have an arrayed resource, so create a second Render Target for the right eye buffer.
    CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_UNKNOWN, 0, 1, 1);
    hr = m_pD3DDev->CreateRenderTargetView(pBackBuffer, &rtDesc, &m_pRenderTargetViewRight);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "%s - Failed to create right eye buffer (%s).", __FUNCTION__, GetErrorDescription(hr).c_str());
      g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_OFF); // try fallback to mono
    }
  }
  pBackBuffer->Release();

  DXGI_FORMAT zFormat = DXGI_FORMAT_D16_UNORM;
  if      (IsFormatSupport(DXGI_FORMAT_D32_FLOAT, D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))          zFormat = DXGI_FORMAT_D32_FLOAT;
  else if (IsFormatSupport(DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))  zFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  else if (IsFormatSupport(DXGI_FORMAT_D16_UNORM, D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))          zFormat = DXGI_FORMAT_D16_UNORM;

  ID3D11Texture2D* depthStencilBuffer = nullptr;
  // Initialize the description of the depth buffer.
  CD3D11_TEXTURE2D_DESC depthBufferDesc(zFormat, m_nBackBufferWidth, m_nBackBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);
  // Create the texture for the depth buffer using the filled out description.
  hr = m_pD3DDev->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s - Failed to create depth stencil buffer (%s).", __FUNCTION__, GetErrorDescription(hr).c_str());
    return false;
  }

  // Create the depth stencil view.
  CD3D11_DEPTH_STENCIL_VIEW_DESC viewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
  hr = m_pD3DDev->CreateDepthStencilView(depthStencilBuffer, &viewDesc, &m_depthStencilView);
  depthStencilBuffer->Release();

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s - Failed to create depth stencil view (%s).", __FUNCTION__, GetErrorDescription(hr).c_str());
    return false;
  }

  if (m_viewPort.Height == 0 || m_viewPort.Width == 0)
  {
    CRect rect(0.0f, 0.0f,
      static_cast<float>(m_nBackBufferWidth),
      static_cast<float>(m_nBackBufferHeight));
    SetViewPort(rect);
  }

  // set camera to center of screen
  CPoint camPoint = { m_nBackBufferWidth * 0.5f, m_nBackBufferHeight * 0.5f };
  SetCameraPosition(camPoint, m_nBackBufferWidth, m_nBackBufferHeight);

  CheckInterlasedStereoView();

  if (bRestoreRTView)
    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_depthStencilView);

  // notify about resurection of display
  if (m_bResizeRequred)
    OnDisplayBack();

  m_resizeInProgress = false;
  m_bResizeRequred = false;

  return true;
}

void CRenderSystemDX::CheckInterlasedStereoView(void)
{
  RENDER_STEREO_MODE stereoMode = g_graphicsContext.GetStereoMode();

  if ( m_pRenderTargetViewRight 
    && RENDER_STEREO_MODE_INTERLACED    != stereoMode
    && RENDER_STEREO_MODE_CHECKERBOARD  != stereoMode
    && RENDER_STEREO_MODE_HARDWAREBASED != stereoMode)
  {
    // release resources
    SAFE_RELEASE(m_pRenderTargetViewRight);
    SAFE_RELEASE(m_pShaderResourceViewRight);
    SAFE_RELEASE(m_pTextureRight);
  }

  if ( !m_pRenderTargetViewRight
    && ( RENDER_STEREO_MODE_INTERLACED   == stereoMode 
      || RENDER_STEREO_MODE_CHECKERBOARD == stereoMode))
  {
    // Create a second Render Target for the right eye buffer
    HRESULT hr;
    CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_B8G8R8A8_UNORM, m_nBackBufferWidth, m_nBackBufferHeight, 1, 1,
                                  D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    hr = m_pD3DDev->CreateTexture2D(&texDesc, nullptr, &m_pTextureRight);
    if (SUCCEEDED(hr))
    {
      CD3D11_RENDER_TARGET_VIEW_DESC rtDesc(D3D11_RTV_DIMENSION_TEXTURE2D);
      hr = m_pD3DDev->CreateRenderTargetView(m_pTextureRight, &rtDesc, &m_pRenderTargetViewRight);

      if (SUCCEEDED(hr))
      {
        CD3D11_SHADER_RESOURCE_VIEW_DESC srDesc(D3D11_SRV_DIMENSION_TEXTURE2D);
        hr = m_pD3DDev->CreateShaderResourceView(m_pTextureRight, &srDesc, &m_pShaderResourceViewRight);

        if (FAILED(hr))
          CLog::Log(LOGERROR, "%s - Failed to create right view shader resource.", __FUNCTION__);
      }
      else
        CLog::Log(LOGERROR, "%s - Failed to create right view render target.", __FUNCTION__);
    }

    if (FAILED(hr))
    {
      SAFE_RELEASE(m_pShaderResourceViewRight);
      SAFE_RELEASE(m_pRenderTargetViewRight);
      SAFE_RELEASE(m_pTextureRight);

      CLog::Log(LOGERROR, "%s - Failed to create right eye buffer.", __FUNCTION__);
      g_graphicsContext.SetStereoMode(RENDER_STEREO_MODE_OFF); // try fallback to mono
    }
  }
}

bool CRenderSystemDX::CreateStates()
{
  if (!m_pD3DDev)
    return false;

  SAFE_RELEASE(m_depthStencilState);
  SAFE_RELEASE(m_BlendEnableState);
  SAFE_RELEASE(m_BlendDisableState);

  // Initialize the description of the stencil state.
  D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
  ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

  // Set up the description of the stencil state.
  depthStencilDesc.DepthEnable = false;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
  depthStencilDesc.StencilEnable = false;
  depthStencilDesc.StencilReadMask = 0xFF;
  depthStencilDesc.StencilWriteMask = 0xFF;

  // Stencil operations if pixel is front-facing.
  depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Stencil operations if pixel is back-facing.
  depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Create the depth stencil state.
  HRESULT hr = m_pD3DDev->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
  if(FAILED(hr))
    return false;

  // Set the depth stencil state.
	m_pContext->OMSetDepthStencilState(m_depthStencilState, 0);

  D3D11_RASTERIZER_DESC rasterizerState;
  rasterizerState.CullMode = D3D11_CULL_NONE; 
  rasterizerState.FillMode = D3D11_FILL_SOLID;// DEBUG - D3D11_FILL_WIREFRAME
  rasterizerState.FrontCounterClockwise = false;
  rasterizerState.DepthBias = 0;
  rasterizerState.DepthBiasClamp = 0.0f;
  rasterizerState.DepthClipEnable = true;
  rasterizerState.SlopeScaledDepthBias = 0.0f;
  rasterizerState.ScissorEnable = false;
  rasterizerState.MultisampleEnable = false;
  rasterizerState.AntialiasedLineEnable = false;

  if (FAILED(m_pD3DDev->CreateRasterizerState(&rasterizerState, &m_RSScissorDisable)))
    return false;

  rasterizerState.ScissorEnable = true;
  if (FAILED(m_pD3DDev->CreateRasterizerState(&rasterizerState, &m_RSScissorEnable)))
    return false;

  m_pContext->RSSetState(m_RSScissorDisable); // by default

  D3D11_BLEND_DESC blendState = { 0 };
  ZeroMemory(&blendState, sizeof(D3D11_BLEND_DESC));
  blendState.RenderTarget[0].BlendEnable = true;
  blendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // D3D11_BLEND_SRC_ALPHA;
  blendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // D3D11_BLEND_INV_SRC_ALPHA;
  blendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  blendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  m_pD3DDev->CreateBlendState(&blendState, &m_BlendEnableState);

  blendState.RenderTarget[0].BlendEnable = false;
  m_pD3DDev->CreateBlendState(&blendState, &m_BlendDisableState);

  // by default
  m_pContext->OMSetBlendState(m_BlendEnableState, nullptr, 0xFFFFFFFF);
  m_BlendEnabled = true;

  return true;
}

void CRenderSystemDX::PresentRenderImpl(bool rendered)
{
  HRESULT hr;

  if (!rendered)
    return;
  
  if (!m_bRenderCreated || m_resizeInProgress)
    return;

  if (m_nDeviceStatus != S_OK)
  {
    // if DXGI_STATUS_OCCLUDED occurred we just clear command queue and return
    if (m_nDeviceStatus == DXGI_STATUS_OCCLUDED)
      FinishCommandList(false);
    return;
  }

  if ( m_stereoMode == RENDER_STEREO_MODE_INTERLACED
    || m_stereoMode == RENDER_STEREO_MODE_CHECKERBOARD)
  {
    // all views prepared, let's merge them before present
    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_depthStencilView);
    CRect destRect = { 0.0f, 0.0f, float(m_nBackBufferWidth), float(m_nBackBufferHeight) };
    SHADER_METHOD method = RENDER_STEREO_MODE_INTERLACED == m_stereoMode
                           ? SHADER_METHOD_RENDER_STEREO_INTERLACED_RIGHT
                           : SHADER_METHOD_RENDER_STEREO_CHECKERBOARD_RIGHT;
    SetAlphaBlendEnable(true);
    CD3DTexture::DrawQuad(destRect, 0, 1, &m_pShaderResourceViewRight, nullptr, method);
    CD3DHelper::PSClearShaderResources(m_pContext);
  }

  FinishCommandList();
  m_pImdContext->Flush();

  hr = m_pSwapChain->Present((m_bVSync ? 1 : 0), 0);

  if (DXGI_ERROR_DEVICE_REMOVED == hr)
  {
    CLog::Log(LOGDEBUG, "%s - device removed", __FUNCTION__);
    return;
  }

  if (DXGI_ERROR_INVALID_CALL == hr)
  {
    SetFullScreenInternal();
    CreateWindowSizeDependentResources();
    hr = S_OK;
  }

  if (FAILED(hr))
  {
    CLog::Log(LOGDEBUG, "%s - Present failed. %s", __FUNCTION__, GetErrorDescription(hr).c_str());
    return;
  }

  // after present swapchain unbinds RT view from immediate context, need to restore it because it can be used by something else
  if (m_pContext == m_pImdContext)
    m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_depthStencilView);
}

bool CRenderSystemDX::BeginRender()
{
  if (!m_bRenderCreated)
    return false;

  HRESULT oldStatus = m_nDeviceStatus;
  m_nDeviceStatus = m_pSwapChain->Present(0, DXGI_PRESENT_TEST);

  // handling of return values. 
  switch (m_nDeviceStatus)
  {
  case DXGI_ERROR_DEVICE_REMOVED: // GPU has been physically removed from the system, or a driver upgrade occurred. 
    CLog::Log(LOGERROR, "DXGI_ERROR_DEVICE_REMOVED");
    m_needNewDevice = true;
    break;
  case DXGI_ERROR_DEVICE_RESET: // This is an run-time issue that should be investigated and fixed.
    CLog::Log(LOGERROR, "DXGI_ERROR_DEVICE_RESET");
    m_nDeviceStatus = DXGI_ERROR_DEVICE_REMOVED;
    m_needNewDevice = true;
    break;
  case DXGI_ERROR_INVALID_CALL: // application provided invalid parameter data. Try to return after resize buffers
    CLog::Log(LOGERROR, "DXGI_ERROR_INVALID_CALL");
    // in most cases when DXGI_ERROR_INVALID_CALL occurs it means what DXGI silently leaves from FSE mode.
    // if so, we should return for FSE mode and resize buffers
    SetFullScreenInternal();
    CreateWindowSizeDependentResources();
    m_nDeviceStatus = S_OK;
    break;
  case DXGI_STATUS_OCCLUDED: // decide what we should do when windows content is not visible
    // do not spam to log file
    if (m_nDeviceStatus != oldStatus)
      CLog::Log(LOGDEBUG, "DXGI_STATUS_OCCLUDED");
    // Status OCCLUDED is not an error and not handled by FAILED macro, 
    // but if it occurs we should not render anything, this status will be accounted on present stage
  }

  if (FAILED(m_nDeviceStatus))
  {
    if (DXGI_ERROR_DEVICE_REMOVED == m_nDeviceStatus)
    {
      OnDeviceLost();
      OnDeviceReset();
    }
    return false;
  }

  m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_depthStencilView);
  m_inScene = true;

  return true;
}

bool CRenderSystemDX::EndRender()
{
  m_inScene = false;

  if (!m_bRenderCreated)
    return false;
  
  if(m_nDeviceStatus != S_OK)
    return false;

  return true;
}

bool CRenderSystemDX::ClearBuffers(color_t color)
{
  if (!m_bRenderCreated || m_resizeInProgress)
    return false;

  float fColor[4];
  CD3DHelper::XMStoreColor(fColor, color);
  ID3D11RenderTargetView* pRTView = m_pRenderTargetView;

  if ( m_stereoMode != RENDER_STEREO_MODE_OFF
    && m_stereoMode != RENDER_STEREO_MODE_MONO)
  {
    // if stereo anaglyph/tab/sbs, data was cleared when left view was rendererd
    if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    {
      // execute command's queue
      FinishCommandList();

      // do not clear RT for anaglyph modes
      if ( m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA
        || m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN
        || m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
      {
        pRTView = nullptr;
      }
      // for interlaced/checkerboard/hw clear right view
      else if (m_pRenderTargetViewRight)
        pRTView = m_pRenderTargetViewRight;
    }
  }
 
  if (pRTView == nullptr)
    return true;

  CRect clRect(0.0f, 0.0f,
    static_cast<float>(m_nBackBufferWidth),
    static_cast<float>(m_nBackBufferHeight));

  // Unlike Direct3D 9, D3D11 ClearRenderTargetView always clears full extent of the resource view. 
  // Viewport and scissor settings are not applied. So clear RT by drawing full sized rect with clear color
  if (m_ScissorsEnabled && m_scissor != clRect)
  {
    bool alphaEnabled = m_BlendEnabled;
    if (alphaEnabled)
      SetAlphaBlendEnable(false);

    CGUITextureD3D::DrawQuad(clRect, color);

    if (alphaEnabled)
      SetAlphaBlendEnable(true);
  }
  else
    m_pContext->ClearRenderTargetView(pRTView, fColor);

  m_pContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
  return true;
}

bool CRenderSystemDX::IsExtSupported(const char* extension)
{
  return false;
}

void CRenderSystemDX::SetVSync(bool enable)
{
  m_bVSync = enable;
}

void CRenderSystemDX::CaptureStateBlock()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::ApplyStateBlock()
{
  if (!m_bRenderCreated)
    return;

  m_pContext->RSSetState(m_ScissorsEnabled ? m_RSScissorEnable : m_RSScissorDisable);
  m_pContext->OMSetDepthStencilState(m_depthStencilState, 0);
  float factors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  m_pContext->OMSetBlendState(m_BlendEnabled ? m_BlendEnableState : m_BlendDisableState, factors, 0xFFFFFFFF);

  m_pGUIShader->ApplyStateBlock();
}

void CRenderSystemDX::SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor)
{
  if (!m_bRenderCreated)
    return;

  // grab the viewport dimensions and location
  float w = m_viewPort.Width*0.5f;
  float h = m_viewPort.Height*0.5f;

  XMFLOAT2 offset = XMFLOAT2(camera.x - screenWidth*0.5f, camera.y - screenHeight*0.5f);

  // world view.  Until this is moved onto the GPU (via a vertex shader for instance), we set it to the identity here.
  m_pGUIShader->SetWorld(XMMatrixIdentity());

  // Initialize the view matrix
  // camera view.  Multiply the Y coord by -1 then translate so that everything is relative to the camera
  // position.
  XMMATRIX flipY, translate;
  flipY = XMMatrixScaling(1.0, -1.0f, 1.0f);
  translate = XMMatrixTranslation(-(w + offset.x - stereoFactor), -(h + offset.y), 2 * h);
  m_pGUIShader->SetView(XMMatrixMultiply(translate, flipY));

  // projection onto screen space
  m_pGUIShader->SetProjection(XMMatrixPerspectiveOffCenterLH((-w - offset.x)*0.5f, (w - offset.x)*0.5f, (-h + offset.y)*0.5f, (h + offset.y)*0.5f, h, 100 * h));
}

void CRenderSystemDX::Project(float &x, float &y, float &z)
{
  if (!m_bRenderCreated)
    return;

  m_pGUIShader->Project(x, y, z);
}

bool CRenderSystemDX::TestRender()
{
  /*
  static unsigned int lastTime = 0;
  static float delta = 0;

  unsigned int thisTime = XbmcThreads::SystemClockMillis();

  if(thisTime - lastTime > 10)
  {
    lastTime = thisTime;
    delta++;
  }

  CLog::Log(LOGINFO, "Delta =  %d", delta);

  if(delta > m_nBackBufferWidth)
    delta = 0;

  LPDIRECT3DVERTEXBUFFER9 pVB = NULL;

  // A structure for our custom vertex type
  struct CUSTOMVERTEX
  {
    FLOAT x, y, z, rhw; // The transformed position for the vertex
    DWORD color;        // The vertex color
  };

  // Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

  // Initialize three vertices for rendering a triangle
  CUSTOMVERTEX vertices[] =
  {
    { delta + 100.0f,  50.0f, 0.5f, 1.0f, 0xffff0000, }, // x, y, z, rhw, color
    { delta+200.0f, 250.0f, 0.5f, 1.0f, 0xff00ff00, },
    {  delta, 250.0f, 0.5f, 1.0f, 0xff00ffff, },
  };

  // Create the vertex buffer. Here we are allocating enough memory
  // (from the default pool) to hold all our 3 custom vertices. We also
  // specify the FVF, so the vertex buffer knows what data it contains.
  if( FAILED( m_pD3DDevice->CreateVertexBuffer( 3 * sizeof( CUSTOMVERTEX ),
    0, D3DFVF_CUSTOMVERTEX,
    D3DPOOL_DEFAULT, &pVB, NULL ) ) )
  {
    return false;
  }

  // Now we fill the vertex buffer. To do this, we need to Lock() the VB to
  // gain access to the vertices. This mechanism is required becuase vertex
  // buffers may be in device memory.
  VOID* pVertices;
  if( FAILED( pVB->Lock( 0, sizeof( vertices ), ( void** )&pVertices, 0 ) ) )
    return false;
  memcpy( pVertices, vertices, sizeof( vertices ) );
  pVB->Unlock();

  m_pD3DDevice->SetStreamSource( 0, pVB, 0, sizeof( CUSTOMVERTEX ) );
  m_pD3DDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
  m_pD3DDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 1 );

  pVB->Release();
  */
  return true;
}

void CRenderSystemDX::ApplyHardwareTransform(const TransformMatrix &finalMatrix)
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::RestoreHardwareTransform()
{
  if (!m_bRenderCreated)
    return;
}

void CRenderSystemDX::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  viewPort.x1 = m_viewPort.TopLeftX;
  viewPort.y1 = m_viewPort.TopLeftY;
  viewPort.x2 = m_viewPort.TopLeftX + m_viewPort.Width;
  viewPort.y2 = m_viewPort.TopLeftY + m_viewPort.Height;
}

void CRenderSystemDX::SetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  m_viewPort.MinDepth   = 0.0f;
  m_viewPort.MaxDepth   = 1.0f;
  m_viewPort.TopLeftX   = viewPort.x1;
  m_viewPort.TopLeftY   = viewPort.y1;
  m_viewPort.Width      = viewPort.x2 - viewPort.x1;
  m_viewPort.Height     = viewPort.y2 - viewPort.y1;

  m_pContext->RSSetViewports(1, &m_viewPort);
  m_pGUIShader->SetViewPort(m_viewPort);
}

void CRenderSystemDX::RestoreViewPort()
{
  if (!m_bRenderCreated)
    return;

  m_pContext->RSSetViewports(1, &m_viewPort);
  m_pGUIShader->SetViewPort(m_viewPort);
}

bool CRenderSystemDX::ScissorsCanEffectClipping()
{
  if (!m_bRenderCreated)
    return false;

  return m_pGUIShader != nullptr && m_pGUIShader->HardwareClipIsPossible();
}

CRect CRenderSystemDX::ClipRectToScissorRect(const CRect &rect)
{
  if (!m_bRenderCreated)
    return CRect();

  float xFactor = m_pGUIShader->GetClipXFactor();
  float xOffset = m_pGUIShader->GetClipXOffset();
  float yFactor = m_pGUIShader->GetClipYFactor();
  float yOffset = m_pGUIShader->GetClipYOffset();

  return CRect(rect.x1 * xFactor + xOffset,
               rect.y1 * yFactor + yOffset,
               rect.x2 * xFactor + xOffset,
               rect.y2 * yFactor + yOffset);
}

void CRenderSystemDX::SetScissors(const CRect& rect)
{
  if (!m_bRenderCreated)
    return;

  m_scissor = rect;
  CD3D11_RECT scissor(MathUtils::round_int(rect.x1)
                    , MathUtils::round_int(rect.y1)
                    , MathUtils::round_int(rect.x2)
                    , MathUtils::round_int(rect.y2));

  m_pContext->RSSetScissorRects(1, &scissor);
  m_pContext->RSSetState(m_RSScissorEnable);
  m_ScissorsEnabled = true;
}

void CRenderSystemDX::ResetScissors()
{
  if (!m_bRenderCreated)
    return;

  m_scissor.SetRect(0.0f, 0.0f, 
    static_cast<float>(m_nBackBufferWidth),
    static_cast<float>(m_nBackBufferHeight));

  m_pContext->RSSetState(m_RSScissorDisable);
  m_ScissorsEnabled = false;
}

void CRenderSystemDX::Register(ID3DResource *resource)
{
  CSingleLock lock(m_resourceSection);
  m_resources.push_back(resource);
}

void CRenderSystemDX::Unregister(ID3DResource* resource)
{
  CSingleLock lock(m_resourceSection);
  std::vector<ID3DResource*>::iterator i = find(m_resources.begin(), m_resources.end(), resource);
  if (i != m_resources.end())
    m_resources.erase(i);
}

std::string CRenderSystemDX::GetErrorDescription(HRESULT hr)
{
  WCHAR buff[1024];
  DXGetErrorDescription(hr, buff, 1024);
  std::wstring error(DXGetErrorString(hr));
  std::wstring descr(buff);
  return StringUtils::Format("%X - %ls (%ls)", hr, error.c_str(), descr.c_str());
}

void CRenderSystemDX::SetStereoMode(RENDER_STEREO_MODE mode, RENDER_STEREO_VIEW view)
{
  CRenderSystemBase::SetStereoMode(mode, view);

  if (!m_bRenderCreated)
    return;

  UINT writeMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_RED;
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_GREEN;
  }
  if(m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA)
  {
    if(m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_GREEN;
    else if(m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_RED;
  }
  if (m_stereoMode == RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE)
  {
    if (m_stereoView == RENDER_STEREO_VIEW_LEFT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN;
    else if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
      writeMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
  }
  if ( RENDER_STEREO_MODE_INTERLACED    == m_stereoMode
    || RENDER_STEREO_MODE_CHECKERBOARD  == m_stereoMode
    || RENDER_STEREO_MODE_HARDWAREBASED == m_stereoMode)
  {
    if (m_stereoView == RENDER_STEREO_VIEW_RIGHT)
    {
      // render right eye view to right render target
      m_pContext->OMSetRenderTargets(1, &m_pRenderTargetViewRight, m_depthStencilView);
    }
  }

  D3D11_BLEND_DESC desc;
  m_BlendEnableState->GetDesc(&desc);
  // update blend state
  if (desc.RenderTarget[0].RenderTargetWriteMask != writeMask)
  {
    SAFE_RELEASE(m_BlendDisableState);
    SAFE_RELEASE(m_BlendEnableState);

    desc.RenderTarget[0].RenderTargetWriteMask = writeMask;
    m_pD3DDev->CreateBlendState(&desc, &m_BlendEnableState);

    desc.RenderTarget[0].BlendEnable = false;
    m_pD3DDev->CreateBlendState(&desc, &m_BlendDisableState);

    float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pContext->OMSetBlendState(m_BlendEnabled ? m_BlendEnableState : m_BlendDisableState, blendFactors, 0xFFFFFFFF);
  }
}

bool CRenderSystemDX::GetStereoEnabled() const
{
  bool result = false;

  IDXGIFactory2* dxgiFactory2 = nullptr;
  if (SUCCEEDED(m_dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2))))
    result = dxgiFactory2->IsWindowedStereoEnabled() == TRUE;
  SAFE_RELEASE(dxgiFactory2);

  return result;
}

bool CRenderSystemDX::GetDisplayStereoEnabled() const
{
  bool result = false;

  IDXGIDisplayControl * pDXGIDisplayControl = nullptr;
  if (SUCCEEDED(m_dxgiFactory->QueryInterface(__uuidof(IDXGIDisplayControl), reinterpret_cast<void **>(&pDXGIDisplayControl))))
    result = pDXGIDisplayControl->IsStereoEnabled() == TRUE;
  SAFE_RELEASE(pDXGIDisplayControl);

  return result;
}

void CRenderSystemDX::SetDisplayStereoEnabled(bool enable) const
{
  IDXGIDisplayControl * pDXGIDisplayControl = nullptr;
  if (SUCCEEDED(m_dxgiFactory->QueryInterface(__uuidof(IDXGIDisplayControl), reinterpret_cast<void **>(&pDXGIDisplayControl))))
    pDXGIDisplayControl->SetStereoEnabled(enable);
  SAFE_RELEASE(pDXGIDisplayControl);
}

void CRenderSystemDX::UpdateDisplayStereoStatus(bool first)
{
  if (first)
    m_bDefaultStereoEnabled = GetDisplayStereoEnabled();

  if (!first || !m_bDefaultStereoEnabled)
    SetDisplayStereoEnabled(true);

  m_bStereoEnabled = GetStereoEnabled();
  SetDisplayStereoEnabled(false);
}

bool CRenderSystemDX::SupportsStereo(RENDER_STEREO_MODE mode) const
{
  switch (mode)
  {
    case RENDER_STEREO_MODE_ANAGLYPH_RED_CYAN:
    case RENDER_STEREO_MODE_ANAGLYPH_GREEN_MAGENTA:
    case RENDER_STEREO_MODE_ANAGLYPH_YELLOW_BLUE:
    case RENDER_STEREO_MODE_INTERLACED:
    case RENDER_STEREO_MODE_CHECKERBOARD:
      return true;
    case RENDER_STEREO_MODE_HARDWAREBASED:
      return m_bStereoEnabled || GetStereoEnabled();
    default:
      return CRenderSystemBase::SupportsStereo(mode);
  }
}

void CRenderSystemDX::FlushGPU() const
{
  if (!m_bRenderCreated)
    return;

  FinishCommandList();
  m_pImdContext->Flush();
}

bool CRenderSystemDX::InitGUIShader()
{
  if (!m_pD3DDev)
    return false;

  SAFE_DELETE(m_pGUIShader);
  m_pGUIShader = new CGUIShaderDX();
  if (!m_pGUIShader->Initialize())
  {
    CLog::Log(LOGERROR, __FUNCTION__ " - Failed to initialize GUI shader.");
    return false;
  }

  m_pGUIShader->ApplyStateBlock();

  return true;
}

void CRenderSystemDX::SetAlphaBlendEnable(bool enable)
{
  if (!m_bRenderCreated)
    return;

  float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
  m_pContext->OMSetBlendState(enable ? m_BlendEnableState : m_BlendDisableState, nullptr, 0xFFFFFFFF);
  m_BlendEnabled = enable;
}

void CRenderSystemDX::FinishCommandList(bool bExecute /*= true*/) const
{
  if (m_pImdContext == m_pContext)
    return;

  ID3D11CommandList* pCommandList = nullptr;
  if (FAILED(m_pContext->FinishCommandList(true, &pCommandList)))
  {
    CLog::Log(LOGERROR, "%s - Failed to finish command queue.", __FUNCTION__);
    return;
  }

  if (bExecute)
    m_pImdContext->ExecuteCommandList(pCommandList, false);

  SAFE_RELEASE(pCommandList);
}

void CRenderSystemDX::SetMaximumFrameLatency(uint8_t latency) const
{
  if (!m_pD3DDev)
    return;

  IDXGIDevice1* pDXGIDevice = nullptr;
  if (SUCCEEDED(m_pD3DDev->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&pDXGIDevice))))
  {
    // in windowed mode DWM uses triple buffering in any case. 
    // for FSEM we use same buffering to avoid possible shuttering/tearing
    if (latency == -1)
      latency = m_useWindowedDX ? 1 : 3;
    pDXGIDevice->SetMaximumFrameLatency(latency);
    SAFE_RELEASE(pDXGIDevice);
  }
}

void CRenderSystemDX::UninitHooks()
{
  // uninstall
  LhUninstallAllHooks();
  // we need to wait for memory release
  LhWaitForPendingRemovals();
  SAFE_DELETE(m_hHook);
  if (m_hDriverModule)
  {
    FreeLibrary(m_hDriverModule);
    m_hDriverModule = nullptr;
  }
}

void CRenderSystemDX::InitHooks()
{
  DXGI_OUTPUT_DESC outputDesc;
  if (!m_pOutput || FAILED(m_pOutput->GetDesc(&outputDesc)))
    return;

  DISPLAY_DEVICEW displayDevice;
  displayDevice.cb = sizeof(DISPLAY_DEVICEW);
  DWORD adapter = 0;
  bool deviceFound = false;

  // delete exiting hooks.
  UninitHooks();

  // enum devices to find matched
  while (EnumDisplayDevicesW(NULL, adapter, &displayDevice, 0))
  {
    if (wcscmp(displayDevice.DeviceName, outputDesc.DeviceName) == 0)
    {
      deviceFound = true;
      break;
    }
    adapter++;
  }
  if (!deviceFound)
    return;

  CLog::Log(LOGDEBUG, __FUNCTION__": Hookind into UserModeDriver on device %S. ", displayDevice.DeviceKey);
  wchar_t* keyName =
#ifndef _M_X64
    // on x64 system and x32 build use UserModeDriverNameWow key
    CSysInfo::GetKernelBitness() == 64 ? keyName = L"UserModeDriverNameWow" :
#endif // !_WIN64
    L"UserModeDriverName";

  DWORD dwType = REG_MULTI_SZ;
  HKEY hKey = 0;
  wchar_t value[1024];
  DWORD valueLength = sizeof(value);
  LSTATUS lstat;

  // to void \Registry\Machine at the beginning, we use shifted pointer at 18
  if (ERROR_SUCCESS == (lstat = RegOpenKeyExW(HKEY_LOCAL_MACHINE, displayDevice.DeviceKey + 18, 0, KEY_READ, &hKey))
   && ERROR_SUCCESS == (lstat = RegQueryValueExW(hKey, keyName, nullptr, &dwType, (LPBYTE)&value, &valueLength)))
  {
    // 1. registry value has a list of drivers for each API with the following format: dx9\0dx10\0dx11\0dx12\0\0
    // 2. we split the value by \0
    std::vector<std::wstring> drivers;
    const wchar_t* pValue = value;
    while (*pValue)
    {
      drivers.push_back(std::wstring(pValue));
      pValue += drivers.back().size() + 1;
    }
    // no entries in the registry
    if (drivers.empty())
      return;
    // 3. we take only first three values (dx12 driver isn't needed if it exists ofc)
    if (drivers.size() > 3)
      drivers = std::vector<std::wstring>(drivers.begin(), drivers.begin() + 3);
    // 4. and then iterate with reverse order to start iterate with the best candidate for d3d11 driver
    for (auto it = drivers.rbegin(); it != drivers.rend(); ++it)
    {
      m_hDriverModule = LoadLibraryW(it->c_str());
      if (m_hDriverModule != nullptr)
      {
        s_fnOpenAdapter10_2 = (PFND3D10DDI_OPENADAPTER)GetProcAddress(m_hDriverModule, "OpenAdapter10_2");
        if (s_fnOpenAdapter10_2 != nullptr)
        {
          ULONG ACLEntries[1] = { 0 };
          m_hHook = new HOOK_TRACE_INFO();
          // install and activate hook into a driver
          if (SUCCEEDED(LhInstallHook(s_fnOpenAdapter10_2, HookOpenAdapter10_2, nullptr, m_hHook))
           && SUCCEEDED(LhSetInclusiveACL(ACLEntries, 1, m_hHook)))
          {
            CLog::Log(LOGDEBUG, __FUNCTION__": D3D11 hook installed and activated.");
            break;
          }
          else
          {
            CLog::Log(LOGDEBUG, __FUNCTION__": Unable ot install and activate D3D11 hook.");
            SAFE_DELETE(m_hHook);
            FreeLibrary(m_hDriverModule);
            m_hDriverModule = nullptr;
          }
        }
      }
    }
  }

  if (lstat != ERROR_SUCCESS)
    CLog::Log(LOGDEBUG, __FUNCTION__": error open regystry key with error %ld.", lstat);

  if (hKey != 0)
    RegCloseKey(hKey);
}

void CRenderSystemDX::FixRefreshRateIfNecessary(const D3D10DDIARG_CREATERESOURCE* pResource)
{
  if (pResource && pResource->pPrimaryDesc)
  {
    float refreshRate = RATIONAL_TO_FLOAT(pResource->pPrimaryDesc->ModeDesc.RefreshRate);
    if (refreshRate > 10.0f && refreshRate < 300.0f)
    {
      uint32_t refreshNum, refreshDen;
      GetRefreshRatio(static_cast<uint32_t>(m_refreshRate), &refreshNum, &refreshDen);
      float diff = fabs(refreshRate - ((float)refreshNum / (float)refreshDen)) / refreshRate;
      CLog::Log(LOGDEBUG, __FUNCTION__": refreshRate: %0.4f, desired: %0.4f, deviation: %.5f, fixRequired: %s", 
                refreshRate, m_refreshRate, diff, (diff > 0.0005) ? "true" : "false");
      if (diff > 0.0005)
      {
        pResource->pPrimaryDesc->ModeDesc.RefreshRate.Numerator = refreshNum;
        pResource->pPrimaryDesc->ModeDesc.RefreshRate.Denominator = refreshDen;
        CLog::Log(LOGDEBUG, __FUNCTION__": refreshRate fix applied -> %0.3f", RATIONAL_TO_FLOAT(pResource->pPrimaryDesc->ModeDesc.RefreshRate));
      }
    }
  }
}

void CRenderSystemDX::GetRefreshRatio(uint32_t refresh, uint32_t * num, uint32_t * den)
{
  int i = (((refresh + 1) % 24) == 0 || ((refresh + 1) % 30) == 0) ? 1 : 0;
  *num = (refresh + i) * 1000;
  *den = 1000 + i;
}

void APIENTRY HookCreateResource(D3D10DDI_HDEVICE hDevice, const D3D10DDIARG_CREATERESOURCE* pResource, D3D10DDI_HRESOURCE hResource, D3D10DDI_HRTRESOURCE hRtResource)
{
  if (s_windowing && pResource && pResource->pPrimaryDesc)
  {
    s_windowing->FixRefreshRateIfNecessary(pResource);
  }
  s_fnCreateResourceOrig(hDevice, pResource, hResource, hRtResource);
}

HRESULT APIENTRY HookCreateDevice(D3D10DDI_HADAPTER hAdapter, D3D10DDIARG_CREATEDEVICE* pCreateData)
{
  HRESULT hr = s_fnCreateDeviceOrig(hAdapter, pCreateData);
  if (pCreateData->pDeviceFuncs->pfnCreateResource)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": hook into pCreateData->pDeviceFuncs->pfnCreateResource");
    s_fnCreateResourceOrig = pCreateData->pDeviceFuncs->pfnCreateResource;
    pCreateData->pDeviceFuncs->pfnCreateResource = HookCreateResource;
  }
  return hr;
}

HRESULT APIENTRY HookOpenAdapter10_2(D3D10DDIARG_OPENADAPTER *pOpenData)
{
  HRESULT hr = s_fnOpenAdapter10_2(pOpenData);
  if (pOpenData->pAdapterFuncs->pfnCreateDevice)
  {
    CLog::Log(LOGDEBUG, __FUNCTION__": hook into pOpenData->pAdapterFuncs->pfnCreateDevice");
    s_fnCreateDeviceOrig = pOpenData->pAdapterFuncs->pfnCreateDevice;
    pOpenData->pAdapterFuncs->pfnCreateDevice = HookCreateDevice;
  }
  return hr;
}

#endif
