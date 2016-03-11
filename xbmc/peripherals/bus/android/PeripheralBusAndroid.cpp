/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <android/input.h>

#include "PeripheralBusAndroid.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/addons/PeripheralAddonTranslator.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "platform/android/activity/XBMCApp.h"
#include "platform/android/jni/View.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PERIPHERALS;

static const std::string DeviceLocationPrefix = "android/inputdevice/";

CPeripheralBusAndroid::CPeripheralBusAndroid(CPeripherals *manager) :
    CPeripheralBus("PeripBusAndroid", manager, PERIPHERAL_BUS_ANDROID)
{
  // we don't need polling as we get notified through the IInputDeviceCallbacks interface
  m_bNeedsPolling = false;

  // register for input device callbacks
  CXBMCApp::RegisterInputDeviceCallbacks(this);

  // register for input device events
  CXBMCApp::RegisterInputDeviceEventHandler(this);

  // get all currently connected input devices
  m_scanResults = GetInputDevices();
}

CPeripheralBusAndroid::~CPeripheralBusAndroid()
{
  // unregister from input device events
  CXBMCApp::UnregisterInputDeviceEventHandler();

  // unregister from input device callbacks
  CXBMCApp::UnregisterInputDeviceCallbacks();
}

bool CPeripheralBusAndroid::InitializeProperties(CPeripheral* peripheral) const
{
  if (peripheral == nullptr || peripheral->Type() != PERIPHERAL_JOYSTICK)
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: unknown peripheral");
    return false;
  }

  int deviceId;
  if (!GetDeviceId(peripheral->Location(), deviceId))
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to initialize properties for peripheral \"%s\"", peripheral->Location().c_str());
    return false;
  }

  const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
  if (!device)
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to get input device with ID %d", deviceId);
    return false;
  }

  CPeripheralJoystick* joystick = static_cast<CPeripheralJoystick*>(peripheral);
  joystick->SetRequestedPort(device.getControllerNumber());
  joystick->SetProvider("android");

  // prepare the joystick state
  CAndroidJoystickState state;
  if (!state.Initialize(device))
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to initialize the state for input device \"%s\" with ID %d",
              joystick->DeviceName().c_str(), deviceId);
    return false;
  }

  // fill in the number of buttons, hats and axes
  joystick->SetButtonCount(state.GetButtonCount());
  joystick->SetHatCount(state.GetHatCount());
  joystick->SetAxisCount(state.GetAxisCount());

  // remember the joystick state
  m_joystickStates.insert(std::make_pair(deviceId, std::move(state)));

  CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: input device \"%s\" with ID %d has %u buttons, %u hats and %u axes",
            joystick->DeviceName().c_str(), deviceId, joystick->ButtonCount(), joystick->HatCount(), joystick->AxisCount());
  return true;
}

void CPeripheralBusAndroid::ProcessEvents()
{
  std::vector<ADDON::PeripheralEvent> events;
  {
    CSingleLock lock(m_critSectionStates);
    for (const auto& joystickState : m_joystickStates)
      joystickState.second.GetEvents(events);
  }

  for (const auto& event : events)
  {
    CPeripheral* device = GetPeripheral(GetDeviceLocation(event.PeripheralIndex()));
    if (device == nullptr || device->Type() != PERIPHERAL_JOYSTICK)
      continue;

    CPeripheralJoystick* joystick = static_cast<CPeripheralJoystick*>(device);
    switch (event.Type())
    {
      case PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON:
      {
        const bool bPressed = (event.ButtonState() == JOYSTICK_STATE_BUTTON_PRESSED);
        CLog::Log(LOGDEBUG, "Button [ %u ] on %s %s", event.DriverIndex(),
                  joystick->DeviceName().c_str(), bPressed ? "pressed" : "released");
        if (joystick->OnButtonMotion(event.DriverIndex(), bPressed))
          CLog::Log(LOGDEBUG, "Joystick button event handled");
        break;
      }
      case PERIPHERAL_EVENT_TYPE_DRIVER_HAT:
      {
        const JOYSTICK::HAT_STATE state = CPeripheralAddonTranslator::TranslateHatState(event.HatState());
        CLog::Log(LOGDEBUG, "Hat [ %u ] on %s %s", event.DriverIndex(),
                  joystick->DeviceName().c_str(), JOYSTICK::CJoystickTranslator::HatStateToString(state));
        if (joystick->OnHatMotion(event.DriverIndex(), state))
          CLog::Log(LOGDEBUG, "Joystick hat event handled");
        break;
      }
      case PERIPHERAL_EVENT_TYPE_DRIVER_AXIS:
      {
        joystick->OnAxisMotion(event.DriverIndex(), event.AxisState());
        break;
      }
      default:
        break;
    }
  }

  {
    CSingleLock lock(m_critSectionStates);
    for (const auto& joystickState : m_joystickStates)
    {
      CPeripheral* device = GetPeripheral(GetDeviceLocation(joystickState.second.GetDeviceId()));
      if (device == nullptr || device->Type() != PERIPHERAL_JOYSTICK)
        continue;

      static_cast<CPeripheralJoystick*>(device)->ProcessAxisMotions();
    }
  }
}

void CPeripheralBusAndroid::OnInputDeviceAdded(int deviceId)
{
  const std::string deviceLocation = GetDeviceLocation(deviceId);
  {
    CSingleLock lock(m_critSectionResults);
    // add the device to the cached result list
    const auto& it = std::find_if(m_scanResults.m_results.cbegin(), m_scanResults.m_results.cend(),
      [&deviceLocation](const PeripheralScanResult& scanResult) { return scanResult.m_strLocation == deviceLocation; });
    if (it != m_scanResults.m_results.cend())
    {
        CLog::Log(LOGINFO, "CPeripheralBusAndroid: ignoring added input device with ID %d because we already know it", deviceId);
        return;
    }

    const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
    if (!device)
    {
      CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to add input device with ID %d because it couldn't be found", deviceId);
      return;
    }

    PeripheralScanResult result;
    if (!ConvertToPeripheralScanResult(device, result))
      return;
    m_scanResults.m_results.push_back(result);
  }

  CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: input device with ID %d added", deviceId);
  OnDeviceAdded(deviceLocation);
}

void CPeripheralBusAndroid::OnInputDeviceChanged(int deviceId)
{
  bool changed = false;
  const std::string deviceLocation = GetDeviceLocation(deviceId);
  {
    CSingleLock lock(m_critSectionResults);
    // change the device in the cached result list
    for (auto result = m_scanResults.m_results.begin(); result != m_scanResults.m_results.end(); ++result)
    {
      if (result->m_strLocation == deviceLocation)
      {
        const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
        if (!device)
        {
          CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to update input device \"%s\" with ID %d because it couldn't be found", result->m_strDeviceName.c_str(), deviceId);
          return;
        }

        if (!ConvertToPeripheralScanResult(device, *result))
          return;

        CLog::Log(LOGINFO, "CPeripheralBusAndroid: input device \"%s\" with ID %d updated", result->m_strDeviceName.c_str(), deviceId);
        changed = true;
        break;
      }
    }
  }

  if (changed)
    OnDeviceChanged(deviceLocation);
  else
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to update input device with ID %d because it couldn't be found", deviceId);
}

void CPeripheralBusAndroid::OnInputDeviceRemoved(int deviceId)
{
  bool removed = false;
  const std::string deviceLocation = GetDeviceLocation(deviceId);
  {
    CSingleLock lock(m_critSectionResults);
    // remove the device from the cached result list
    for (auto result = m_scanResults.m_results.begin(); result != m_scanResults.m_results.end(); ++result)
    {
      if (result->m_strLocation == deviceLocation)
      {
        CLog::Log(LOGINFO, "CPeripheralBusAndroid: input device \"%s\" with ID %d removed", result->m_strDeviceName.c_str(), deviceId);
        m_scanResults.m_results.erase(result);
        removed = true;
        break;
      }
    }
  }

  if (removed)
  {
    m_joystickStates.erase(deviceId);

    OnDeviceRemoved(deviceLocation);
  }
  else
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to remove input device with ID %d because it couldn't be found", deviceId);
}

bool CPeripheralBusAndroid::OnInputDeviceEvent(const AInputEvent* event)
{
  if (event == nullptr)
    return false;

  CSingleLock lock(m_critSectionStates);
  // get the id of the input device which generated the event
  int32_t deviceId = AInputEvent_getDeviceId(event);

  // find the matching joystick state
  auto joystickState = m_joystickStates.find(deviceId);
  if (joystickState == m_joystickStates.end())
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: ignoring input event for unknown input device with ID %d", deviceId);
    return false;
  }

  return joystickState->second.ProcessEvent(event);
}

bool CPeripheralBusAndroid::PerformDeviceScan(PeripheralScanResults &results)
{
  CSingleLock lock(m_critSectionResults);
  results = m_scanResults;

  return true;
}

PeripheralScanResults CPeripheralBusAndroid::GetInputDevices()
{
  CLog::Log(LOGINFO, "CPeripheralBusAndroid: scanning for input devices...");

  PeripheralScanResults results;
  std::vector<int> deviceIds = CXBMCApp::GetInputDeviceIds();

  for (const auto& deviceId : deviceIds)
  {
    const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
    if (!device)
    {
      CLog::Log(LOGWARNING, "CPeripheralBusAndroid: no input device with ID %d found", deviceId);
      continue;
    }

    PeripheralScanResult result;
    if (!ConvertToPeripheralScanResult(device, result))
      continue;

    CLog::Log(LOGINFO, "CPeripheralBusAndroid: input device \"%s\" with ID %d detected", result.m_strDeviceName.c_str(), deviceId);
    results.m_results.push_back(result);
  }

  return results;
}

std::string CPeripheralBusAndroid::GetDeviceLocation(int deviceId)
{
  return StringUtils::Format("%s%d", DeviceLocationPrefix.c_str(), deviceId);
}

bool CPeripheralBusAndroid::GetDeviceId(const std::string& deviceLocation, int& deviceId)
{
  if (deviceLocation.empty() ||
      !StringUtils::StartsWith(deviceLocation, DeviceLocationPrefix) ||
      deviceLocation.size() <= DeviceLocationPrefix.size())
    return false;

  std::string strDeviceId = deviceLocation.substr(DeviceLocationPrefix.size());
  if (!StringUtils::IsNaturalNumber(strDeviceId))
    return false;

  deviceId = static_cast<int>(strtol(strDeviceId.c_str(), nullptr, 10));
  return true;
}

bool CPeripheralBusAndroid::ConvertToPeripheralScanResult(const CJNIViewInputDevice& inputDevice, PeripheralScanResult& peripheralScanResult)
{
  int deviceId = inputDevice.getId();
  std::string deviceName = inputDevice.getName();
  if (inputDevice.isVirtual())
  {
    CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: ignoring virtual input device \"%s\" with ID %d", deviceName.c_str(), deviceId);
    return false;
  }
  if (!inputDevice.supportsSource(CJNIViewInputDevice::SOURCE_JOYSTICK) && !inputDevice.supportsSource(CJNIViewInputDevice::SOURCE_GAMEPAD))
  {
    CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: ignoring unknown input device \"%s\" with ID %d", deviceName.c_str(), deviceId);
    return false;
  }

  peripheralScanResult.m_type = PERIPHERAL_JOYSTICK;
  peripheralScanResult.m_strLocation = GetDeviceLocation(deviceId);
  peripheralScanResult.m_iVendorId = inputDevice.getVendorId();
  peripheralScanResult.m_iProductId = inputDevice.getProductId();
  peripheralScanResult.m_mappedType = PERIPHERAL_JOYSTICK;
  peripheralScanResult.m_strDeviceName = deviceName;
  peripheralScanResult.m_busType = PERIPHERAL_BUS_ANDROID;
  peripheralScanResult.m_mappedBusType = PERIPHERAL_BUS_ANDROID;
  peripheralScanResult.m_iSequence = 0;

  return true;
}
