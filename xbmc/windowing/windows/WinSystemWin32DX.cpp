/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWin32DX.h"
#include "commons/ilog.h"
#include "platform/win32/CharsetConverter.h"
#include "rendering/dx/RenderContext.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "windowing/GraphicContext.h"

#include "system.h"

#ifndef _M_X64
#pragma comment(lib, "EasyHook32.lib")
#include "utils/SystemInfo.h"
#else
#pragma comment(lib, "EasyHook64.lib")
#endif
#pragma comment(lib, "dxgi.lib")
#pragma warning(disable: 4091)
#include <d3d10umddi.h>
#pragma warning(default: 4091)

using KODI::PLATFORM::WINDOWS::FromW;

// User Mode Driver hooks definitions
void APIENTRY HookCreateResource(D3D10DDI_HDEVICE hDevice, const D3D10DDIARG_CREATERESOURCE* pResource, D3D10DDI_HRESOURCE hResource, D3D10DDI_HRTRESOURCE hRtResource);
HRESULT APIENTRY HookCreateDevice(D3D10DDI_HADAPTER hAdapter, D3D10DDIARG_CREATEDEVICE* pCreateData);
HRESULT APIENTRY HookOpenAdapter10_2(D3D10DDIARG_OPENADAPTER *pOpenData);
static PFND3D10DDI_OPENADAPTER s_fnOpenAdapter10_2{ nullptr };
static PFND3D10DDI_CREATEDEVICE s_fnCreateDeviceOrig{ nullptr };
static PFND3D10DDI_CREATERESOURCE s_fnCreateResourceOrig{ nullptr };

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemWin32DX());
  return winSystem;
}

CWinSystemWin32DX::CWinSystemWin32DX() : CRenderSystemDX()
  , m_hDriverModule(nullptr)
  , m_hHook(nullptr)
{
}

CWinSystemWin32DX::~CWinSystemWin32DX()
{
}

void CWinSystemWin32DX::PresentRenderImpl(bool rendered)
{
  if (rendered)
    m_deviceResources->Present();

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    OnDisplayReset();
  }

  if (!rendered)
    Sleep(40);
}

bool CWinSystemWin32DX::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res)
{
  const MONITOR_DETAILS* monitor = GetDisplayDetails(CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
  if (!monitor)
    return false;

  m_hMonitor = monitor->hMonitor;
  m_deviceResources = DX::DeviceResources::Get();
  // setting monitor before creating window for proper hooking into a driver
  m_deviceResources->SetMonitor(m_hMonitor);

  return CWinSystemWin32::CreateNewWindow(name, fullScreen, res) && m_deviceResources->HasValidDevice();
}

void CWinSystemWin32DX::SetWindow(HWND hWnd) const
{
  m_deviceResources->SetWindow(hWnd);
}

bool CWinSystemWin32DX::DestroyRenderSystem()
{
  CRenderSystemDX::DestroyRenderSystem();

  m_deviceResources->Release();
  m_deviceResources.reset();
  return true;
}

void CWinSystemWin32DX::SetDeviceFullScreen(bool fullScreen, RESOLUTION_INFO& res)
{
  if (m_deviceResources->SetFullScreen(fullScreen, res))
  {
    ResolutionChanged();
  }
}

bool CWinSystemWin32DX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemWin32::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemDX::OnResize();

  return true;
}

void CWinSystemWin32DX::OnMove(int x, int y)
{
  // do not handle moving at window creation because MonitorFromWindow
  // returns default system monitor in case of m_hWnd is null
  if (!m_hWnd)
    return;

  HMONITOR newMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
  if (newMonitor != m_hMonitor)
  {
    MONITOR_DETAILS* details = GetDisplayDetails(newMonitor);
    CDisplaySettings::GetInstance().SetMonitor(KODI::PLATFORM::WINDOWS::FromW(details->MonitorNameW));
    m_deviceResources->SetMonitor(newMonitor);
    m_hMonitor = newMonitor;
  }
}

bool CWinSystemWin32DX::DPIChanged(WORD dpi, RECT windowRect) const
{
  // on Win10 FCU the OS keeps window size exactly the same size as it was
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin10_FCU))
    return true;

  m_deviceResources->SetDpi(dpi);
  if (!IsAlteringWindow())
    return __super::DPIChanged(dpi, windowRect);

  return true;
}

void CWinSystemWin32DX::ReleaseBackBuffer()
{
  m_deviceResources->ReleaseBackBuffer();
}

void CWinSystemWin32DX::CreateBackBuffer()
{
  m_deviceResources->CreateBackBuffer();
}

void CWinSystemWin32DX::ResizeDeviceBuffers()
{
  m_deviceResources->ResizeBuffers();
}

bool CWinSystemWin32DX::IsStereoEnabled()
{
  return m_deviceResources->IsStereoEnabled();
}

void CWinSystemWin32DX::OnScreenChange(HMONITOR monitor)
{
  m_deviceResources->SetMonitor(monitor);
}

bool CWinSystemWin32DX::ChangeResolution(const RESOLUTION_INFO &res, bool forceChange)
{
  bool changed = CWinSystemWin32::ChangeResolution(res, forceChange);
  // this is a try to fix FCU issue after changing resolution
  if (m_deviceResources && changed)
    m_deviceResources->ResizeBuffers();
  return changed;
}

void CWinSystemWin32DX::OnResize(int width, int height)
{
  if (!m_IsAlteringWindow)
    ReleaseBackBuffer();

  m_deviceResources->SetLogicalSize(static_cast<float>(width), static_cast<float>(height));

  if (!m_IsAlteringWindow)
    CreateBackBuffer();
}

bool CWinSystemWin32DX::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  bool const result = CWinSystemWin32::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemDX::OnResize();
  return result;
}

void CWinSystemWin32DX::UninitHooks()
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

void CWinSystemWin32DX::InitHooks(IDXGIOutput* pOutput)
{
  DXGI_OUTPUT_DESC outputDesc;
  if (!pOutput || FAILED(pOutput->GetDesc(&outputDesc)))
    return;

  DISPLAY_DEVICEW displayDevice;
  displayDevice.cb = sizeof(DISPLAY_DEVICEW);
  DWORD adapter = 0;
  bool deviceFound = false;

  // delete exiting hooks.
  UninitHooks();

  // enum devices to find matched
  while (EnumDisplayDevicesW(nullptr, adapter, &displayDevice, 0))
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

  CLog::LogF(LOGDEBUG, "Hooking into UserModeDriver on device %s. ", FromW(displayDevice.DeviceKey));
  wchar_t* keyName =
#ifndef _M_X64
  // on x64 system and x32 build use UserModeDriverNameWow key
  CSysInfo::GetKernelBitness() == 64 ? keyName = L"UserModeDriverNameWow" :
#endif // !_WIN64
    L"UserModeDriverName";

  DWORD dwType = REG_MULTI_SZ;
  HKEY hKey = nullptr;
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
        s_fnOpenAdapter10_2 = reinterpret_cast<PFND3D10DDI_OPENADAPTER>(GetProcAddress(m_hDriverModule, "OpenAdapter10_2"));
        if (s_fnOpenAdapter10_2 != nullptr)
        {
          ULONG ACLEntries[1] = { 0 };
          m_hHook = new HOOK_TRACE_INFO();
          // install and activate hook into a driver
          if (SUCCEEDED(LhInstallHook(s_fnOpenAdapter10_2, HookOpenAdapter10_2, nullptr, m_hHook))
            && SUCCEEDED(LhSetInclusiveACL(ACLEntries, 1, m_hHook)))
          {
            CLog::LogF(LOGDEBUG, "D3D11 hook installed and activated.");
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
    CLog::LogF(LOGDEBUG, "error open registry key with error %ld.", lstat);

  if (hKey != nullptr)
    RegCloseKey(hKey);
}

void CWinSystemWin32DX::FixRefreshRateIfNecessary(const D3D10DDIARG_CREATERESOURCE* pResource) const
{
  if (pResource && pResource->pPrimaryDesc)
  {
    float refreshRate = RATIONAL_TO_FLOAT(pResource->pPrimaryDesc->ModeDesc.RefreshRate);
    if (refreshRate > 10.0f && refreshRate < 300.0f)
    {
      // interlaced
      if (pResource->pPrimaryDesc->ModeDesc.ScanlineOrdering > DXGI_DDI_MODE_SCANLINE_ORDER_PROGRESSIVE)
        refreshRate /= 2;

      uint32_t refreshNum, refreshDen;
      DX::GetRefreshRatio(static_cast<uint32_t>(floor(m_fRefreshRate)), &refreshNum, &refreshDen);
      float diff = fabs(refreshRate - static_cast<float>(refreshNum) / static_cast<float>(refreshDen)) / refreshRate;
      CLog::LogF(LOGDEBUG, "refreshRate: %0.4f, desired: %0.4f, deviation: %.5f, fixRequired: %s, %d",
        refreshRate, m_fRefreshRate, diff, (diff > 0.0005 && diff < 0.1) ? "yes" : "no", pResource->pPrimaryDesc->Flags);
      if (diff > 0.0005 && diff < 0.1)
      {
        pResource->pPrimaryDesc->ModeDesc.RefreshRate.Numerator = refreshNum;
        pResource->pPrimaryDesc->ModeDesc.RefreshRate.Denominator = refreshDen;
        if (pResource->pPrimaryDesc->ModeDesc.ScanlineOrdering > DXGI_DDI_MODE_SCANLINE_ORDER_PROGRESSIVE)
          pResource->pPrimaryDesc->ModeDesc.RefreshRate.Numerator *= 2;
        CLog::LogF(LOGDEBUG, "refreshRate fix applied -> %0.3f", RATIONAL_TO_FLOAT(pResource->pPrimaryDesc->ModeDesc.RefreshRate));
      }
    }
  }
}

void APIENTRY HookCreateResource(D3D10DDI_HDEVICE hDevice, const D3D10DDIARG_CREATERESOURCE* pResource, D3D10DDI_HRESOURCE hResource, D3D10DDI_HRTRESOURCE hRtResource)
{
  if (pResource && pResource->pPrimaryDesc)
  {
    DX::Windowing()->FixRefreshRateIfNecessary(pResource);
  }
  s_fnCreateResourceOrig(hDevice, pResource, hResource, hRtResource);
}

HRESULT APIENTRY HookCreateDevice(D3D10DDI_HADAPTER hAdapter, D3D10DDIARG_CREATEDEVICE* pCreateData)
{
  HRESULT hr = s_fnCreateDeviceOrig(hAdapter, pCreateData);
  if (pCreateData->pDeviceFuncs->pfnCreateResource)
  {
    CLog::LogF(LOGDEBUG, "hook into pCreateData->pDeviceFuncs->pfnCreateResource");
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
    CLog::LogF(LOGDEBUG, "hook into pOpenData->pAdapterFuncs->pfnCreateDevice");
    s_fnCreateDeviceOrig = pOpenData->pAdapterFuncs->pfnCreateDevice;
    pOpenData->pAdapterFuncs->pfnCreateDevice = HookCreateDevice;
  }
  return hr;
}
