/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceBroker.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/powermanagement/Win32PowerSyscall.h"

#include <mmdeviceapi.h>
#include <wrl/client.h>

using KODI::PLATFORM::WINDOWS::FromW;

namespace KODI::PLATFORM::WINDOWS::INTERNAL
{
inline bool DeviceSettingMatchesEndpoint(std::string setting, std::string endpointId)
{
  if (setting.empty() || endpointId.empty())
    return false;

  StringUtils::ToLower(setting);
  StringUtils::ToLower(endpointId);
  return setting.find(endpointId) != std::string::npos;
}

inline bool ConfiguredDeviceMatchesEndpoint(const std::string& audioDevice,
                                            const std::string& passthroughDevice,
                                            bool passthroughEnabled,
                                            const std::string& endpointId)
{
  return DeviceSettingMatchesEndpoint(audioDevice, endpointId) ||
         (passthroughEnabled && DeviceSettingMatchesEndpoint(passthroughDevice, endpointId));
}
} // namespace KODI::PLATFORM::WINDOWS::INTERNAL

class CMMNotificationClient : public IMMNotificationClient
{
  LONG _cRef;
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> _pEnumerator;


public:
  CMMNotificationClient() : _cRef(1), _pEnumerator(nullptr)
  {
    CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                     reinterpret_cast<void**>(_pEnumerator.GetAddressOf()));
  }

  ~CMMNotificationClient() = default;

  // IUnknown methods -- AddRef, Release, and QueryInterface

  ULONG STDMETHODCALLTYPE AddRef()
  {
    return InterlockedIncrement(&_cRef);
  }

  ULONG STDMETHODCALLTYPE Release()
  {
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
      delete this;
    }
    return ulRef;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID & riid, void **ppvInterface)
  {
    if (IID_IUnknown == riid)
    {
      AddRef();
      *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
      AddRef();
      *ppvInterface = (IMMNotificationClient*)this;
    }
    else
    {
      *ppvInterface = nullptr;
      return E_NOINTERFACE;
    }
    return S_OK;
  }

  // Callback methods for device-event notifications.

  HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
  {
    // if the default device changes this function is called four times.
    // therefore we call CServiceBroker::GetActiveAE()->DeviceChange() only for one role.
    std::string pszFlow = "?????";
    std::string pszRole = "?????";

    switch (flow)
    {
    case eRender:
      pszFlow = "eRender";
      break;
    case eCapture:
      pszFlow = "eCapture";
      break;
    }

    switch (role)
    {
    case eConsole:
      pszRole = "eConsole";
      break;
    case eMultimedia:
      pszRole = "eMultimedia";
      break;
    case eCommunications:
      pszRole = "eCommunications";
      NotifyAE();
      break;
    }

    CLog::Log(LOGDEBUG, "{}: New default device: flow = {}, role = {}", __FUNCTION__, pszFlow,
              pszRole);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId)
  {
    CLog::Log(LOGDEBUG, "{}: Added device: {}", __FUNCTION__, FromW(pwstrDeviceId));
    NotifyAEForDevice(pwstrDeviceId);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId)
  {
    CLog::Log(LOGDEBUG, "{}: Removed device: {}", __FUNCTION__, FromW(pwstrDeviceId));
    NotifyAEDeviceCountChange();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
  {
    std::string pszState = "?????";

    switch (dwNewState)
    {
    case DEVICE_STATE_ACTIVE:
      pszState = "ACTIVE";
      break;
    case DEVICE_STATE_DISABLED:
      pszState = "DISABLED";
      break;
    case DEVICE_STATE_NOTPRESENT:
      pszState = "NOTPRESENT";
      break;
    case DEVICE_STATE_UNPLUGGED:
      pszState = "UNPLUGGED";
      break;
    }
    CLog::Log(LOGDEBUG, "{}: New device state is DEVICE_STATE_{}", __FUNCTION__, pszState);
    if (dwNewState == DEVICE_STATE_ACTIVE)
      NotifyAEForDevice(pwstrDeviceId);
    else
      NotifyAEDeviceCountChange();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
  {
    CLog::Log(LOGDEBUG,
              "{}: Changed device property of {} is "
              "({:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x})#{}",
              __FUNCTION__, FromW(pwstrDeviceId), key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
              key.fmtid.Data4[0], key.fmtid.Data4[1], key.fmtid.Data4[2], key.fmtid.Data4[3],
              key.fmtid.Data4[4], key.fmtid.Data4[5], key.fmtid.Data4[6], key.fmtid.Data4[7],
              key.pid);
    return S_OK;
  }

  void STDMETHODCALLTYPE NotifyAE()
  {
    if(!CWin32PowerSyscall::IsSuspending())
      CServiceBroker::GetActiveAE()->DeviceChange();
  }

  void STDMETHODCALLTYPE NotifyAEForDevice(LPCWSTR pwstrDeviceId)
  {
    if (CWin32PowerSyscall::IsSuspending())
      return;

    const std::string endpointId = GetAudioEndpointId(pwstrDeviceId);
    const auto settingsComponent = CServiceBroker::GetSettingsComponent();
    const auto settings = settingsComponent ? settingsComponent->GetSettings() : nullptr;

    // Preserve the existing full reconfiguration when endpoint resolution
    // fails, or when the configured PCM/enabled passthrough endpoint returns. Default-
    // device changes retain their dedicated OnDefaultDeviceChanged path above.
    if (endpointId.empty() || !settings ||
        KODI::PLATFORM::WINDOWS::INTERNAL::ConfiguredDeviceMatchesEndpoint(
            settings->GetString(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE),
            settings->GetString(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE),
            settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH), endpointId))
    {
      CServiceBroker::GetActiveAE()->DeviceChange();
      return;
    }

    // An unrelated endpoint (for example Bluetooth headphones) still changes
    // the selectable device list, but must not tear down a healthy active sink.
    // DeviceCountChange refreshes enumeration and only reconfigures when the
    // current sink has actually disappeared.
    CLog::Log(LOGDEBUG,
              "{}: Refreshing audio devices without reconfiguring the active sink for unrelated "
              "endpoint {}",
              __FUNCTION__, endpointId);
    CServiceBroker::GetActiveAE()->DeviceCountChange("");
  }

  void STDMETHODCALLTYPE NotifyAEDeviceCountChange()
  {
    if (!CWin32PowerSyscall::IsSuspending())
      CServiceBroker::GetActiveAE()->DeviceCountChange("");
  }

  std::string GetAudioEndpointId(LPCWSTR pwstrDeviceId)
  {
    if (!pwstrDeviceId)
      return {};

    if (!_pEnumerator)
      return {};

    Microsoft::WRL::ComPtr<IMMDevice> device;
    HRESULT hr = _pEnumerator->GetDevice(pwstrDeviceId, device.GetAddressOf());
    if (FAILED(hr))
      return {};

    Microsoft::WRL::ComPtr<IPropertyStore> properties;
    hr = device->OpenPropertyStore(STGM_READ, properties.GetAddressOf());
    if (FAILED(hr))
      return {};

    PROPVARIANT endpointId;
    PropVariantInit(&endpointId);
    hr = properties->GetValue(PKEY_AudioEndpoint_GUID, &endpointId);
    std::string result;
    if (SUCCEEDED(hr) && endpointId.vt == VT_LPWSTR && endpointId.pwszVal)
      result = FromW(endpointId.pwszVal);
    PropVariantClear(&endpointId);

    return result;
  }
};
