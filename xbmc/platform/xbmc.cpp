/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Application.h"
#include "settings/AdvancedSettings.h"

#ifdef TARGET_RASPBERRY_PI
#include "platform/linux/RBP.h"
#endif

#ifdef TARGET_WINDOWS_DESKTOP
#include "platform/win32/IMMNotificationClient.h"
#include <mmdeviceapi.h>
#include <wrl/client.h>
#endif

#if defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif

#include "platform/MessagePrinter.h"
#include "utils/log.h"
#include "commons/Exception.h"

extern "C" int XBMC_Run(bool renderGUI, const CAppParamParser &params)
{
  int status = -1;

  if (!g_advancedSettings.Initialized())
  {
    g_advancedSettings.Initialize();
  }

  if (!g_application.Create(params))
  {
    CMessagePrinter::DisplayError("ERROR: Unable to create application. Exiting");
    return status;
  }

#ifdef TARGET_RASPBERRY_PI
  if(!g_RBP.Initialize())
    return false;
  g_RBP.LogFirmwareVersion();
#elif defined(TARGET_ANDROID)
  CXBMCApp::get()->Initialize();
#endif

  if (renderGUI && !g_application.CreateGUI())
  {
    CMessagePrinter::DisplayError("ERROR: Unable to create GUI. Exiting");
    return status;
  }
  if (!g_application.Initialize())
  {
    CMessagePrinter::DisplayError("ERROR: Unable to Initialize. Exiting");
    return status;
  }

#ifdef TARGET_WINDOWS_DESKTOP
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  CMMNotificationClient cMMNC;
  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                                reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  if (SUCCEEDED(hr))
  {
    pEnumerator->RegisterEndpointNotificationCallback(&cMMNC);
    pEnumerator = nullptr;
  }
#endif

  status = g_application.Run(params);

#ifdef TARGET_WINDOWS_DESKTOP
  // the end
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                        reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  if (SUCCEEDED(hr))
  {
    pEnumerator->UnregisterEndpointNotificationCallback(&cMMNC);
    pEnumerator = nullptr;
  }
#endif

#ifdef TARGET_RASPBERRY_PI
  g_RBP.Deinitialize();
#elif defined(TARGET_ANDROID)
  CXBMCApp::get()->Deinitialize();
#endif

  return status;
}
