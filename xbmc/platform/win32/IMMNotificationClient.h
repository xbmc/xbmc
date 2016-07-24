//#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include <mmdeviceapi.h>
#include "system.h" // for SAFE_RELEASE
#include "utils/log.h"
#include "cores/AudioEngine/AEFactory.h"
#include "powermanagement/windows/Win32PowerSyscall.h"

class CMMNotificationClient : public IMMNotificationClient
{
  LONG _cRef;
  IMMDeviceEnumerator *_pEnumerator;


public:
  CMMNotificationClient() : _cRef(1), _pEnumerator(NULL)
  {
  }

  ~CMMNotificationClient()
  {
    SAFE_RELEASE(_pEnumerator);
  }

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

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface)
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
      *ppvInterface = NULL;
      return E_NOINTERFACE;
    }
    return S_OK;
  }

  // Callback methods for device-event notifications.

  HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
  {
    // if the default device changes this function is called four times.
    // therefore we call CAEFactory::DeviceChange() only for one role.
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

    CLog::Log(LOGDEBUG, "%s: New default device: flow = %s, role = %s", __FUNCTION__, pszFlow, pszRole);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId)
  {
    CLog::Log(LOGDEBUG, "%s: Added device: %s", __FUNCTION__, pwstrDeviceId);
    NotifyAE();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId)
  {
    CLog::Log(LOGDEBUG, "%s: Removed device: %s", __FUNCTION__, pwstrDeviceId);
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
    CLog::Log(LOGDEBUG, "%s: New device state is DEVICE_STATE_%s", __FUNCTION__, pszState);
    NotifyAE();
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
  {
    CLog::Log(LOGDEBUG, "%s: Changed device property of %S is {%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d", 
              __FUNCTION__, pwstrDeviceId, key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
                                           key.fmtid.Data4[0], key.fmtid.Data4[1],
                                           key.fmtid.Data4[2], key.fmtid.Data4[3],
                                           key.fmtid.Data4[4], key.fmtid.Data4[5],
                                           key.fmtid.Data4[6], key.fmtid.Data4[7],
                                           key.pid);
    return S_OK;
  }

  void STDMETHODCALLTYPE NotifyAE()
  {
    if(!CWin32PowerSyscall::IsSuspending())
      CAEFactory::DeviceChange();
  }
};