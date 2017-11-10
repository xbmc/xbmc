/*
 *      Copyright (C) 2010-2017 Team Kodi
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
#include "AESinkFactoryWin.h"
#include "platform/win32/CharsetConverter.h"
#include "system.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <mmdeviceapi.h>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
DEFINE_PROPERTYKEY(PKEY_Device_EnumeratorName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 24);

extern const char *WASAPIErrToStr(HRESULT err);
#define EXIT_ON_FAILURE(hr, reason, ...) if(FAILED(hr)) {CLog::Log(LOGERROR, reason " - %s", __VA_ARGS__, WASAPIErrToStr(hr)); goto failed;}

std::vector<RendererDetail> CAESinkFactoryWin::GetRendererDetails()
{
  std::vector<RendererDetail> list;
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;
  IMMDevice*           pDefaultDevice = NULL;
  LPWSTR               pwszID = NULL;
  std::wstring         wstrDDID;
  HRESULT              hr;

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)
  
  UINT uiCount = 0;

  // get the default audio endpoint
  if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice) == S_OK)
  {
    if (pDefaultDevice->GetId(&pwszID) == S_OK)
    {
      wstrDDID = pwszID;
      CoTaskMemFree(pwszID);
    }
    SAFE_RELEASE(pDefaultDevice);
  }

  // enumerate over all audio endpoints
  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    RendererDetail details;
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    hr = pEnumDevices->Item(i, &pDevice);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint failed.");
      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.");
      SAFE_RELEASE(pDevice);
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint device name failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    details.strDescription = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint GUID failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    details.strDeviceId = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint form factor failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }
    details.strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    details.eDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;

    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint speaker layout failed.");
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    details.uiChannelMask = std::max(varName.uintVal, (unsigned int)KSAUDIO_SPEAKER_STEREO);
    PropVariantClear(&varName);

    if (pDevice->GetId(&pwszID) == S_OK)
    {
      if (wstrDDID.compare(pwszID) == 0)
        details.bDefault = true;

      CoTaskMemFree(pwszID);
    }

    list.push_back(details);

    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pProperty);
  }

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);

  return list;

failed:

  CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate audio renderer devices.");
  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);

  return list;
}

struct AEWASAPIDeviceWin32 : public IAEWASAPIDevice
{
  friend CAESinkFactoryWin;

  HRESULT AEWASAPIDeviceWin32::Activate(IAudioClient** ppAudioClient)
  {
    HRESULT hr = S_FALSE;

    if (!ppAudioClient)
      return E_POINTER;

    try
    {
      IAudioClient* pClient = nullptr;

      hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pClient);
      if (SUCCEEDED(hr) && pClient)
      {
        *ppAudioClient = pClient;
        return hr;
      }
    }
    catch (...) {}
    return hr;
  };

  int AEWASAPIDeviceWin32::Release() override
  {
    SAFE_RELEASE(m_pDevice);
    delete this;
    return 0;
  };

  bool AEWASAPIDeviceWin32::IsUSBDevice() override
  {
    bool ret = false;
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    HRESULT hr = m_pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    if (!SUCCEEDED(hr))
      return ret;
    hr = pProperty->GetValue(PKEY_Device_EnumeratorName, &varName);

    std::string str = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    StringUtils::ToUpper(str);
    ret = (str == "USB");
    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty);
    return ret;
  }

protected:
  AEWASAPIDeviceWin32(IMMDevice* pDevice) 
    : m_pDevice(pDevice)
  {
  }

private:
  IMMDevice*           m_pDevice{ nullptr };
};

std::string CAESinkFactoryWin::GetDefaultDeviceId()
{
  IMMDevice* pDevice;
  IMMDeviceEnumerator* pEnumerator = NULL;
  LPWSTR pwszID = NULL;
  std::wstring wstrDDID;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)

    // get the default audio endpoint
  if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice) == S_OK)
  {
    if (pDevice->GetId(&pwszID) == S_OK)
    {
      wstrDDID = pwszID;
      CoTaskMemFree(pwszID);
    }
    SAFE_RELEASE(pDevice);
  }
  SAFE_RELEASE(pEnumerator);

  return KODI::PLATFORM::WINDOWS::FromW(wstrDDID);

failed:
  return std::string();
}

HRESULT CAESinkFactoryWin::ActivateWASAPIDevice(std::string &device, IAEWASAPIDevice **ppDevice)
{
  IMMDevice* pDevice;
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDeviceCollection* pEnumDevices = NULL;

  if (!ppDevice)
    return E_POINTER;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Could not allocate WASAPI device enumerator. CoCreateInstance error code: %li", hr)

  /* Get our device. First try to find the named device. */
  UINT uiCount = 0;

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEnumDevices);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    IPropertyStore *pProperty = NULL;
    PROPVARIANT varName;

    hr = pEnumDevices->Item(i, &pDevice);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint failed.")

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProperty);
    EXIT_ON_FAILURE(hr, __FUNCTION__": Retrieval of WASAPI endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, __FUNCTION__": Retrieval of WASAPI endpoint GUID failed.");
      SAFE_RELEASE(pProperty);
      goto failed;
    }

    std::string strDevName = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);

    if (device == strDevName)
      i = uiCount;
    else
      SAFE_RELEASE(pDevice);

    PropVariantClear(&varName);
    SAFE_RELEASE(pProperty);
  }

  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);

  if (pDevice)
  {
    AEWASAPIDeviceWin32* pAEDevice = new AEWASAPIDeviceWin32(pDevice);
    pAEDevice->deviceId = device;
    return S_OK;
  }

  return S_FALSE;

failed:
  CLog::Log(LOGERROR, __FUNCTION__": WASAPI initialization failed.");
  SAFE_RELEASE(pDevice);
  SAFE_RELEASE(pEnumDevices);
  SAFE_RELEASE(pEnumerator);

  return hr;
}
