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
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"
#include "utils/log.h"

#include <mmdeviceapi.h>
#include <mmreg.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Media.Devices.h>
#include <winrt/Windows.Media.Devices.Core.h>

using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Media::Devices;
using namespace winrt::Windows::Media::Devices::Core;
using namespace Microsoft::WRL;

static winrt::hstring PKEY_Device_FriendlyName = L"System.ItemNameDisplay";
static winrt::hstring PKEY_AudioEndpoint_FormFactor = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 0";
static winrt::hstring PKEY_AudioEndpoint_ControlPanelPageProvider = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 1";
static winrt::hstring PKEY_AudioEndpoint_Association = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 2";
static winrt::hstring PKEY_AudioEndpoint_PhysicalSpeakers = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 3";
static winrt::hstring PKEY_AudioEndpoint_GUID = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 4";
static winrt::hstring PKEY_AudioEndpoint_Disable_SysFx = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 5";
static winrt::hstring PKEY_AudioEndpoint_FullRangeSpeakers = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 6";
static winrt::hstring PKEY_AudioEndpoint_Supports_EventDriven_Mode = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 7";
static winrt::hstring PKEY_AudioEndpoint_JackSubType = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 8";
static winrt::hstring PKEY_AudioEndpoint_Default_VolumeInDb = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e} 9";
static winrt::hstring PKEY_AudioEngine_DeviceFormat = L"{f19f064d-082c-4e27-bc73-6882a1bb8e4c} 0";
static winrt::hstring PKEY_Device_EnumeratorName = L"{a45c254e-df1c-4efd-8020-67d146a850e0} 24";

std::vector<RendererDetail> CAESinkFactoryWin::GetRendererDetails()
{
  std::vector<RendererDetail> list;
  try
  {
    // Get the string identifier of the audio renderer
    auto defaultId = MediaDevice::GetDefaultAudioRenderId(AudioDeviceRole::Default);
    auto audioSelector = MediaDevice::GetAudioRenderSelector();

    // Add custom properties to the query
    DeviceInformationCollection devInfocollection = Wait(DeviceInformation::FindAllAsync(audioSelector,
      {
          PKEY_AudioEndpoint_FormFactor,
          PKEY_AudioEndpoint_GUID,
          PKEY_AudioEndpoint_PhysicalSpeakers,
          PKEY_AudioEngine_DeviceFormat,
          PKEY_Device_EnumeratorName
      }));
    if (devInfocollection == nullptr || devInfocollection.Size() == 0)
      goto failed;

    for (unsigned int i = 0; i < devInfocollection.Size(); i++)
    {
      RendererDetail details;

      DeviceInformation devInfo = devInfocollection.GetAt(i);
      if (devInfo.Properties().Size() == 0)
        goto failed;

      winrt::IInspectable propObj = nullptr;

      propObj = devInfo.Properties().Lookup(PKEY_AudioEndpoint_FormFactor);
      if (!propObj)
        goto failed;

      details.strWinDevType = winEndpoints[propObj.as<winrt::IPropertyValue>().GetUInt32()].winEndpointType;
      details.eDeviceType = winEndpoints[propObj.as<winrt::IPropertyValue>().GetUInt32()].aeDeviceType;

      unsigned long ulChannelMask = 0;
      unsigned int nChannels = 0;

      propObj = devInfo.Properties().Lookup(PKEY_AudioEngine_DeviceFormat);
      if (propObj)
      {
        winrt::com_array<uint8_t> com_arr;
        propObj.as<winrt::IPropertyValue>().GetUInt8Array(com_arr);

        WAVEFORMATEXTENSIBLE* smpwfxex = (WAVEFORMATEXTENSIBLE*)com_arr.data();
        nChannels = std::max(std::min(smpwfxex->Format.nChannels, (WORD)8), (WORD)2);
        ulChannelMask = smpwfxex->dwChannelMask;
      }
      else
      {
        // suppose stereo
        nChannels = 2;
        ulChannelMask = 3;
      }

      propObj = devInfo.Properties().Lookup(PKEY_AudioEndpoint_PhysicalSpeakers);
      details.uiChannelMask = propObj ? propObj.as<winrt::IPropertyValue>().GetUInt32() : ulChannelMask;
      details.nChannels = nChannels;

      details.strDescription = KODI::PLATFORM::WINDOWS::FromW(devInfo.Name().c_str());
      details.strDeviceId = KODI::PLATFORM::WINDOWS::FromW(devInfo.Id().c_str());

      details.bDefault = (devInfo.Id() == defaultId);

      list.push_back(details);
    }
    return list;
  }
  catch (...)
  {
  }

failed:
  CLog::Log(LOGERROR, __FUNCTION__": Failed to enumerate audio renderer devices.");
  return list;
}

class CAudioInterfaceActivator : public winrt::implements<CAudioInterfaceActivator, IActivateAudioInterfaceCompletionHandler>
{
  Concurrency::task_completion_event<IAudioClient*> m_ActivateCompleted;

  STDMETHODIMP ActivateCompleted(IActivateAudioInterfaceAsyncOperation *pAsyncOp)
  {
    HRESULT hr = S_OK, hr2 = S_OK;
    ComPtr<IUnknown> clientUnk;
    IAudioClient* audioClient2;

    // Get the audio activation result as IUnknown pointer
    hr2 = pAsyncOp->GetActivateResult(&hr, &clientUnk);

    // Report activation failure
    if (FAILED(hr))
    {
      m_ActivateCompleted.set_exception(winrt::hresult_error(hr));
      goto exit;
    }

    // Report failure to get activate result
    if (FAILED(hr2))
    {
      m_ActivateCompleted.set_exception(winrt::hresult_error(hr2));
      goto exit;
    }

    // Query for the activated IAudioClient2 interface
    hr = clientUnk->QueryInterface(IID_PPV_ARGS(&audioClient2));

    if (FAILED(hr))
    {
      m_ActivateCompleted.set_exception(winrt::hresult_error(hr));
      goto exit;
    }

    // Set the completed event and return success
    m_ActivateCompleted.set(audioClient2);

  exit:
    return hr;

  }
public:
  static Concurrency::task<IAudioClient*> ActivateAsync(LPCWCHAR pszDeviceId)
  {
    winrt::com_ptr<CAudioInterfaceActivator> activator = winrt::make_self<CAudioInterfaceActivator>();
    ComPtr<IActivateAudioInterfaceAsyncOperation> asyncOp;

    HRESULT hr = ActivateAudioInterfaceAsync(
      pszDeviceId,
      __uuidof(IAudioClient2),
      nullptr,
      activator.get(),
      &asyncOp);

    if (FAILED(hr))
      throw winrt::hresult_error(hr);

    // Wait for the activate completed event and return result
    return Concurrency::create_task(activator->m_ActivateCompleted);

  }
};

struct AEWASAPIDeviceWin10 : public IAEWASAPIDevice
{
  HRESULT AEWASAPIDeviceWin10::Activate(IAudioClient** ppAudioClient)
  {
    if (!ppAudioClient)
      return E_POINTER;

    HRESULT hr;
    std::wstring deviceIdW = KODI::PLATFORM::WINDOWS::ToW(deviceId);
    IAudioClient* pClient = CAudioInterfaceActivator::ActivateAsync(deviceIdW.c_str())
      .then([&hr](Concurrency::task<IAudioClient*> task) -> IAudioClient*
    {
      try
      {
        return task.get();
      }
      catch (const winrt::hresult_error& ex)
      {
        hr = ex.code();
        return nullptr;
      }
    }).get();

    if (pClient)
    {
      *ppAudioClient = pClient;
      return S_OK;
    }
    return E_UNEXPECTED;
  };

  int AEWASAPIDeviceWin10::Release() override
  {
    delete this;
    return 0;
  };

  bool AEWASAPIDeviceWin10::IsUSBDevice() override
  {
    // TODO
    return false;
  }
};

std::string CAESinkFactoryWin::GetDefaultDeviceId()
{
  // Get the string identifier of the audio renderer
  auto defaultId = MediaDevice::GetDefaultAudioRenderId(AudioDeviceRole::Default);
  return KODI::PLATFORM::WINDOWS::FromW(defaultId.c_str());
}

HRESULT CAESinkFactoryWin::ActivateWASAPIDevice(std::string &device, IAEWASAPIDevice** ppDevice)
{
  if (!ppDevice)
    return E_POINTER;

  AEWASAPIDeviceWin10* pDevice = new AEWASAPIDeviceWin10;
  pDevice->deviceId = device;

  *ppDevice = pDevice;
  return S_OK;
}