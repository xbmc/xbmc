/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AESinkFactoryWin.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <algorithm>

#include <mmdeviceapi.h>
#include <wrl/client.h>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
DEFINE_PROPERTYKEY(PKEY_Device_EnumeratorName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 24);

extern const char *WASAPIErrToStr(HRESULT err);
#define EXIT_ON_FAILURE(hr, reason) \
  if (FAILED(hr)) \
  { \
    CLog::LogF(LOGERROR, reason " - HRESULT = {} ErrorMessage = {}", hr, WASAPIErrToStr(hr)); \
    goto failed; \
  }

using namespace Microsoft::WRL;

std::vector<RendererDetail> CAESinkFactoryWin::GetRendererDetails()
{
  std::vector<RendererDetail> list;
  ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  ComPtr<IMMDeviceCollection> pEnumDevices = nullptr;
  ComPtr<IMMDevice> pDefaultDevice = nullptr;
  LPWSTR pwszID = nullptr;
  std::wstring wstrDDID;
  HRESULT hr;
  UINT uiCount = 0;

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate WASAPI device enumerator.")


  // get the default audio endpoint
  if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, pDefaultDevice.GetAddressOf()) == S_OK)
  {
    if (pDefaultDevice->GetId(&pwszID) == S_OK)
    {
      wstrDDID = pwszID;
      CoTaskMemFree(pwszID);
    }
    pDefaultDevice.Reset();
  }

  // enumerate over all audio endpoints
  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, pEnumDevices.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    RendererDetail details{};
    ComPtr<IMMDevice> pDevice = nullptr;
    ComPtr<IPropertyStore> pProperty = nullptr;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    hr = pEnumDevices->Item(i, pDevice.GetAddressOf());
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "Retrieval of WASAPI endpoint failed.");
      goto failed;
    }

    hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "Retrieval of WASAPI endpoint properties failed.");
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "Retrieval of WASAPI endpoint device name failed.");
      goto failed;
    }

    details.strDescription = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "Retrieval of WASAPI endpoint GUID failed.");
      goto failed;
    }

    details.strDeviceId = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "Retrieval of WASAPI endpoint form factor failed.");
      goto failed;
    }
    details.strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    details.eDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;

    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &varName);
    if (FAILED(hr))
    {
      CLog::Log(LOGERROR, "Retrieval of WASAPI endpoint speaker layout failed.");
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
  }

  return list;

failed:

  CLog::Log(LOGERROR, "Failed to enumerate audio renderer devices.");
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
      ComPtr<IAudioClient> pClient = nullptr;
      hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, reinterpret_cast<void**>(pClient.GetAddressOf()));
      if (SUCCEEDED(hr) && pClient)
      {
        *ppAudioClient = pClient.Detach();
        return hr;
      }
    }
    catch (...) {}
    return hr;
  };

  int AEWASAPIDeviceWin32::Release() override
  {
    delete this;
    return 0;
  };

  bool AEWASAPIDeviceWin32::IsUSBDevice() override
  {
    bool ret = false;
    ComPtr<IPropertyStore> pProperty = nullptr;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    HRESULT hr = m_pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
    if (!SUCCEEDED(hr))
      return ret;
    hr = pProperty->GetValue(PKEY_Device_EnumeratorName, &varName);

    std::string str = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    StringUtils::ToUpper(str);
    ret = (str == "USB");
    PropVariantClear(&varName);
    return ret;
  }

protected:
  AEWASAPIDeviceWin32(IMMDevice* pDevice)
    : m_pDevice(pDevice)
  {
  }

private:
  ComPtr<IMMDevice> m_pDevice{ nullptr };
};

std::string CAESinkFactoryWin::GetDefaultDeviceId()
{
  std::string strDeviceId = "";
  ComPtr<IMMDevice> pDevice = nullptr;
  ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  std::wstring wstrDDID;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate WASAPI device enumerator.")

    // get the default audio endpoint
  if (pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, pDevice.GetAddressOf()) == S_OK)
  {
    ComPtr<IPropertyStore> pProperty = nullptr;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of WASAPI endpoint properties failed.");
      goto failed;
    }

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of WASAPI endpoint GUID failed.");
      goto failed;
    }
    strDeviceId = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

  }

failed:
  return strDeviceId;
}

HRESULT CAESinkFactoryWin::ActivateWASAPIDevice(std::string &device, IAEWASAPIDevice **ppDevice)
{
  ComPtr<IMMDevice> pDevice = nullptr;
  ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  ComPtr<IMMDeviceCollection> pEnumDevices = nullptr;
  UINT uiCount = 0;

  if (!ppDevice)
    return E_POINTER;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate WASAPI device enumerator.")

  /* Get our device. First try to find the named device. */

  hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, pEnumDevices.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint enumeration failed.")

  hr = pEnumDevices->GetCount(&uiCount);
  EXIT_ON_FAILURE(hr, "Retrieval of audio endpoint count failed.")

  for (UINT i = 0; i < uiCount; i++)
  {
    ComPtr<IPropertyStore> pProperty = nullptr;
    PROPVARIANT varName;

    hr = pEnumDevices->Item(i, pDevice.GetAddressOf());
    EXIT_ON_FAILURE(hr, "Retrieval of WASAPI endpoint failed.")

    hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
    EXIT_ON_FAILURE(hr, "Retrieval of WASAPI endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    if (FAILED(hr))
    {
      CLog::LogF(LOGERROR, "Retrieval of WASAPI endpoint GUID failed.");
      goto failed;
    }

    std::string strDevName = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);

    if (device == strDevName)
      i = uiCount;
    else
      pDevice.Reset();

    PropVariantClear(&varName);
  }

  if (pDevice)
  {
    AEWASAPIDeviceWin32* pAEDevice = new AEWASAPIDeviceWin32(pDevice.Get());
    pAEDevice->deviceId = device;
    *ppDevice = pAEDevice;
    return S_OK;
  }

  return E_FAIL;

failed:
  CLog::LogF(LOGERROR, "WASAPI initialization failed.");
  return hr;
}
