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
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"
#include "platform/win32/powermanagement/Win32PowerSyscall.h"

#include <mmdeviceapi.h>
#include <wrl/client.h>

using KODI::PLATFORM::WINDOWS::FromW;

class CMMNotificationClient : public IMMNotificationClient
{
  LONG _cRef;
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> _pEnumerator;


public:
  CMMNotificationClient() : _cRef(1), _pEnumerator(nullptr)
  {
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
    char  *pszFlow = "?????";
    char  *pszRole = "?????";

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
    NotifyAE();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId)
  {
    CLog::Log(LOGDEBUG, "{}: Removed device: {}", __FUNCTION__, FromW(pwstrDeviceId));
    NotifyAE();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
  {
    char  *pszState = "?????";

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
    NotifyAE();
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
};
