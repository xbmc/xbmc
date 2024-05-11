/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AESinkFactoryWin.h"

#include "platform/win32/CharsetConverter.h"

#include <mmdeviceapi.h>
#include <mmreg.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Media.Devices.h>

using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Media::Devices;
using namespace Microsoft::WRL;

std::vector<RendererDetail> CAESinkFactoryWin::GetRendererDetails()
{
  return GetRendererDetailsWinRT();
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
  HRESULT Activate(IAudioClient** ppAudioClient)
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

  int Release() override
  {
    delete this;
    return 0;
  };

  bool IsUSBDevice() override
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