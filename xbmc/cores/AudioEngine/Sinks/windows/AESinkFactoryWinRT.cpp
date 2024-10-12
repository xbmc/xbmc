/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkFactoryWin.h"
#include "utils/log.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"

#include <winrt/windows.devices.enumeration.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.media.devices.core.h>
#include <winrt/windows.media.devices.h>

namespace winrt
{
using namespace Windows::Foundation;
}

using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Media::Devices;
using namespace winrt::Windows::Media::Devices::Core;

// clang-format off
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
// clang-format on

std::vector<RendererDetail> CAESinkFactoryWin::GetRendererDetailsWinRT()
{
  std::vector<RendererDetail> list;
  try
  {
    // Get the string identifier of the audio renderer
    auto defaultId = MediaDevice::GetDefaultAudioRenderId(AudioDeviceRole::Default);
    auto audioSelector = MediaDevice::GetAudioRenderSelector();

    // Add custom properties to the query
    DeviceInformationCollection devInfoCollection = Wait(DeviceInformation::FindAllAsync(
        audioSelector, {PKEY_AudioEndpoint_FormFactor, PKEY_AudioEndpoint_GUID,
                        PKEY_AudioEndpoint_PhysicalSpeakers, PKEY_AudioEngine_DeviceFormat,
                        PKEY_Device_EnumeratorName}));

    if (devInfoCollection == nullptr || devInfoCollection.Size() == 0)
      goto failed;

    for (const DeviceInformation& devInfo : devInfoCollection)
    {
      RendererDetail details;

      if (devInfo.Properties().Size() == 0)
        goto failed;

      winrt::IInspectable propObj = nullptr;

      propObj = devInfo.Properties().Lookup(PKEY_AudioEndpoint_FormFactor);
      if (!propObj)
        goto failed;

      const uint32_t indexFF{propObj.as<winrt::IPropertyValue>().GetUInt32()};
      details.strWinDevType = winEndpoints[indexFF].winEndpointType;
      details.eDeviceType = winEndpoints[indexFF].aeDeviceType;

      DWORD ulChannelMask = 0;
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

      details.uiChannelMask = propObj ? propObj.as<winrt::IPropertyValue>().GetUInt32()
                                      : static_cast<unsigned int>(ulChannelMask);

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
  CLog::LogF(LOGERROR, "Failed to enumerate audio renderer devices.");
  return list;
}
