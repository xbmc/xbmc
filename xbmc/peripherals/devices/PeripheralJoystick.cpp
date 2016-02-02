/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "PeripheralJoystick.h"
#include "peripherals/Peripherals.h"
#include "peripherals/bus/android/PeripheralBusAndroid.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <algorithm>

using namespace JOYSTICK;
using namespace PERIPHERALS;

CPeripheralJoystick::CPeripheralJoystick(const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(scanResult, bus),
  m_requestedPort(JOYSTICK_PORT_UNKNOWN),
  m_buttonCount(0),
  m_hatCount(0),
  m_axisCount(0),
  m_motorCount(0),
  m_supportsPowerOff(false)
{
  m_features.push_back(FEATURE_JOYSTICK);
  // FEATURE_RUMBLE conditionally added via SetMotorCount()
}

CPeripheralJoystick::~CPeripheralJoystick(void)
{
  m_defaultInputHandler.AbortRumble();
  UnregisterJoystickInputHandler(&m_defaultInputHandler);
  UnregisterJoystickDriverHandler(&m_joystickMonitor);
}

bool CPeripheralJoystick::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_JOYSTICK)
    {
      if (m_mappedBusType == PERIPHERAL_BUS_ADDON)
      {
        CPeripheralBusAddon* addonBus = dynamic_cast<CPeripheralBusAddon*>(m_bus);
        if (addonBus)
        {
          if (addonBus->InitializeProperties(this))
            bSuccess = true;
          else
            CLog::Log(LOGERROR, "CPeripheralJoystick: Invalid location (%s)", m_strLocation.c_str());
        }
      }
#ifdef TARGET_ANDROID
      else if (m_mappedBusType == PERIPHERAL_BUS_ANDROID)
      {
        CPeripheralBusAndroid* androidBus = dynamic_cast<CPeripheralBusAndroid*>(m_bus);
        if (androidBus)
        {
          if (androidBus->InitializeProperties(this))
            bSuccess = true;
          else
            CLog::Log(LOGERROR, "CPeripheralJoystick: Invalid location (%s)", m_strLocation.c_str());
        }
      }
#endif
    }
    else if (feature == FEATURE_RUMBLE)
    {
      bSuccess = true; // Nothing to do
    }
  }

  if (bSuccess)
  {
    // Give joystick monitor priority over default controller
    RegisterJoystickInputHandler(&m_defaultInputHandler);
    RegisterJoystickDriverHandler(&m_joystickMonitor, false);
  }

  return bSuccess;
}

void CPeripheralJoystick::OnUserNotification()
{
  m_defaultInputHandler.NotifyUser();
}

bool CPeripheralJoystick::TestFeature(PeripheralFeature feature)
{
  bool bSuccess = false;

  switch (feature)
  {
  case FEATURE_RUMBLE:
    bSuccess = m_defaultInputHandler.TestRumble();
    break;
  default:
    break;
  }

  return bSuccess;
}

void CPeripheralJoystick::RegisterJoystickDriverHandler(IDriverHandler* handler, bool bPromiscuous)
{
  CSingleLock lock(m_handlerMutex);

  DriverHandler driverHandler = { handler, bPromiscuous };
  m_driverHandlers.insert(m_driverHandlers.begin(), driverHandler);
}

void CPeripheralJoystick::UnregisterJoystickDriverHandler(IDriverHandler* handler)
{
  CSingleLock lock(m_handlerMutex);

  m_driverHandlers.erase(std::remove_if(m_driverHandlers.begin(), m_driverHandlers.end(),
    [handler](const DriverHandler& driverHandler)
    {
      return driverHandler.handler == handler;
    }), m_driverHandlers.end());
}

bool CPeripheralJoystick::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  CSingleLock lock(m_handlerMutex);

  // Process promiscuous handlers
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (it->bPromiscuous)
      it->handler->OnButtonMotion(buttonIndex, bPressed);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (!it->bPromiscuous)
    {
      bHandled |= it->handler->OnButtonMotion(buttonIndex, bPressed);

      // If button is released, force bHandled to false to notify all handlers.
      // This avoids "sticking".
      if (!bPressed)
        bHandled = false;

      // Once a button is handled, we're done
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

bool CPeripheralJoystick::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  CSingleLock lock(m_handlerMutex);

  // Process promiscuous handlers
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (it->bPromiscuous)
      it->handler->OnHatMotion(hatIndex, state);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (!it->bPromiscuous)
    {
      bHandled |= it->handler->OnHatMotion(hatIndex, state);

      // If hat is centered, force bHandled to false to notify all handlers.
      // This avoids "sticking".
      if (state == HAT_STATE::UNPRESSED)
        bHandled = false;

      // Once a hat is handled, we're done
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

bool CPeripheralJoystick::OnAxisMotion(unsigned int axisIndex, float position)
{
  CSingleLock lock(m_handlerMutex);

  // Process promiscuous handlers
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (it->bPromiscuous)
      it->handler->OnAxisMotion(axisIndex, position);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (!it->bPromiscuous)
    {
      bHandled |= it->handler->OnAxisMotion(axisIndex, position);

      // If axis is centered, force bHandled to false to notify all handlers.
      // This avoids "sticking".
      if (position == 0.0f)
        bHandled = false;

      // Once an axis is handled, we're done
      if (bHandled)
        break;
    }
  }

  return bHandled;
}

void CPeripheralJoystick::ProcessAxisMotions(void)
{
  CSingleLock lock(m_handlerMutex);

  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
    it->handler->ProcessAxisMotions();
}

bool CPeripheralJoystick::SetMotorState(unsigned int motorIndex, float magnitude)
{
  bool bHandled = false;

  if (m_mappedBusType == PERIPHERAL_BUS_ADDON)
  {
    CPeripheralBusAddon* addonBus = static_cast<CPeripheralBusAddon*>(m_bus);
    if (addonBus)
    {
      bHandled = addonBus->SendRumbleEvent(m_strLocation, motorIndex, magnitude);
    }
  }
  return bHandled;
}

void CPeripheralJoystick::SetMotorCount(unsigned int motorCount)
{
  m_motorCount = motorCount;

  if (m_motorCount == 0)
    m_features.erase(std::remove(m_features.begin(), m_features.end(), FEATURE_RUMBLE), m_features.end());
  else if (std::find(m_features.begin(), m_features.end(), FEATURE_RUMBLE) == m_features.end())
    m_features.push_back(FEATURE_RUMBLE);
}
