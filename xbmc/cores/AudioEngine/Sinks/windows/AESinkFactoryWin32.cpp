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
#include "platform/win32/WIN32Util.h"

#include <algorithm>

#include <mmdeviceapi.h>
#include <wrl/client.h>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);

DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);
DEFINE_PROPERTYKEY(PKEY_Device_EnumeratorName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 24);

#define EXIT_ON_FAILURE(hr, reason) \
  if (FAILED(hr)) \
  { \
    CLog::LogF(LOGERROR, reason " - {}", hr, CWIN32Util::FormatHRESULT(hr)); \
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

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                        reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate MMDevice enumerator.")

  // get the default audio endpoint
  if (S_OK ==
      (hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, pDefaultDevice.GetAddressOf())))
  {
    if (S_OK == (hr = pDefaultDevice->GetId(&pwszID)))
    {
      wstrDDID = pwszID;
      CoTaskMemFree(pwszID);
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to retrieve default endpoint identifier ({})",
                 CWIN32Util::FormatHRESULT(hr));
    }
    pDefaultDevice.Reset();
  }
  else
  {
    CLog::LogF(LOGERROR, "Unable to retrieve default endpoint ({})", CWIN32Util::FormatHRESULT(hr));
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
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint failed.")

    hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.ReleaseAndGetAddressOf());
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_Device_FriendlyName, &varName);
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint device name failed.")

    details.strDescription = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint GUID failed.")

    details.strDeviceId = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_FormFactor, &varName);
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint form factor failed.")

    details.strWinDevType = winEndpoints[(EndpointFormFactor)varName.uiVal].winEndpointType;
    details.eDeviceType = winEndpoints[(EndpointFormFactor)varName.uiVal].aeDeviceType;
    PropVariantClear(&varName);

    /* In shared mode Windows tells us what format the audio must be in. */
    hr = pProperty->GetValue(PKEY_AudioEngine_DeviceFormat, &varName);
    if (SUCCEEDED(hr) && varName.blob.cbSize >= sizeof(WAVEFORMATEX))
    {
      // This may be a WAVEFORMATEXTENSIBLE but the extra data is not needed.
      WAVEFORMATEX* pwfx = reinterpret_cast<WAVEFORMATEX*>(varName.blob.pBlobData);
      details.nChannels = pwfx->nChannels;
      details.m_samplesPerSec = pwfx->nSamplesPerSec;
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Getting DeviceFormat failed ({})", CWIN32Util::FormatHRESULT(hr));
    }
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &varName);
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint speaker layout failed.")

    details.uiChannelMask = std::max(varName.uintVal, (unsigned int)KSAUDIO_SPEAKER_STEREO);
    PropVariantClear(&varName);

    hr = pProperty->GetValue(PKEY_Device_EnumeratorName, &varName);
    if (SUCCEEDED(hr) && varName.vt != VT_EMPTY)
    {
      details.strDeviceEnumerator = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
      StringUtils::ToUpper(details.strDeviceEnumerator);
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Retrieval of endpoint enumerator name failed: {}.",
                 (FAILED(hr)) ? "'GetValue' has failed" : "'varName.pwszVal' is NULL");
    }
    PropVariantClear(&varName);

    if (S_OK == (hr = pDevice->GetId(&pwszID)))
    {
      if (wstrDDID.compare(pwszID) == 0)
        details.bDefault = true;

      CoTaskMemFree(pwszID);
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to retrieve device id ({})", CWIN32Util::FormatHRESULT(hr));
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

  HRESULT Activate(IAudioClient** ppAudioClient)
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
      else
      {
        CLog::LogF(LOGERROR, "unable to activate IAudioClient ({})", CWIN32Util::FormatHRESULT(hr));
      }
    }
    catch (...) {}
    return hr;
  };

  int Release() override
  {
    delete this;
    return 0;
  };

  bool IsUSBDevice() override
  {
    bool ret = false;
    ComPtr<IPropertyStore> pProperty = nullptr;
    PROPVARIANT varName;
    PropVariantInit(&varName);

    HRESULT hr = m_pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
    if (!SUCCEEDED(hr))
      return ret;
    hr = pProperty->GetValue(PKEY_Device_EnumeratorName, &varName);

    if (SUCCEEDED(hr) && varName.vt != VT_EMPTY)
    {
      std::string str = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
      StringUtils::ToUpper(str);
      ret = (str == "USB");
    }
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
  std::string strDeviceId;
  ComPtr<IMMDevice> pDevice;
  ComPtr<IMMDeviceEnumerator> pEnumerator;
  ComPtr<IPropertyStore> pProperty;
  PROPVARIANT varName;

  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                                reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  EXIT_ON_FAILURE(hr, "Could not allocate MMDevice device enumerator.")

  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, pDevice.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of default audio endpoint failed.")

  hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
  EXIT_ON_FAILURE(hr, "Retrieval of endpoint properties failed.")

  PropVariantInit(&varName);
  hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
  EXIT_ON_FAILURE(hr, "Retrieval of endpoint GUID failed.")

  strDeviceId = KODI::PLATFORM::WINDOWS::FromW(varName.pwszVal);
  PropVariantClear(&varName);

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
  EXIT_ON_FAILURE(hr, "Could not allocate MMDevice enumerator.")

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
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint failed.")

    hr = pDevice->OpenPropertyStore(STGM_READ, pProperty.GetAddressOf());
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint properties failed.")

    hr = pProperty->GetValue(PKEY_AudioEndpoint_GUID, &varName);
    EXIT_ON_FAILURE(hr, "Retrieval of endpoint GUID failed.")

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
