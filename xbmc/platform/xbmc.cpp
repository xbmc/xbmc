/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "Application.h"
#include "settings/AdvancedSettings.h"

#ifdef TARGET_RASPBERRY_PI
#include "linux/RBP.h"
#endif

#if defined(TARGET_WINDOWS) || defined(TARGET_WIN10)
#include <mmdeviceapi.h>
#ifndef TARGET_WIN10
#include "platform/win32/IMMNotificationClient.h"
#endif
#endif

#if defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif

#include "platform/MessagePrinter.h"
#include "utils/log.h"

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

#ifdef TARGET_WINDOWS
  IMMDeviceEnumerator *pEnumerator = nullptr;
  CMMNotificationClient cMMNC;
  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                                reinterpret_cast<void**>(&pEnumerator));
  if (SUCCEEDED(hr))
  {
    pEnumerator->RegisterEndpointNotificationCallback(&cMMNC);
    SAFE_RELEASE(pEnumerator);
  }
#endif

  try
  {
    status = g_application.Run(params);
  }
#if defined(TARGET_WINDOWS) || defined(TARGET_WIN10)
  catch (const XbmcCommons::UncheckedException &e)
  {
    e.LogThrowMessage("CApplication::Create()");
    CMessagePrinter::DisplayError("ERROR: Exception caught on main loop. Exiting");
    status = -1;
  }
#endif
  catch(...)
  {
    CMessagePrinter::DisplayError("ERROR: Exception caught on main loop. Exiting");
    status = -1;
  }

#ifdef TARGET_WINDOWS
  // the end
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                        reinterpret_cast<void**>(&pEnumerator));
  if (SUCCEEDED(hr))
  {
    pEnumerator->UnregisterEndpointNotificationCallback(&cMMNC);
    SAFE_RELEASE(pEnumerator);
  }
#endif

#ifdef TARGET_RASPBERRY_PI
  g_RBP.Deinitialize();
#elif defined(TARGET_ANDROID)
  CXBMCApp::get()->Deinitialize();
#endif

  return status;
}
