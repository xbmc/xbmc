/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralJoystick.h"
#include "games/controllers/ControllerIDs.h"
#include "input/joysticks/interfaces/IDriverHandler.h"
#include "input/joysticks/keymaps/KeymapHandling.h"
#include "input/joysticks/DeadzoneFilter.h"
#include "input/joysticks/JoystickMonitor.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/RumbleGenerator.h"
#include "input/InputManager.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "Application.h"

#include <algorithm>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CPeripheralJoystick::CPeripheralJoystick(CPeripherals& manager, const PeripheralScanResult& scanResult, CPeripheralBus* bus) :
  CPeripheral(manager, scanResult, bus),
  m_requestedPort(JOYSTICK_PORT_UNKNOWN),
  m_buttonCount(0),
  m_hatCount(0),
  m_axisCount(0),
  m_motorCount(0),
  m_supportsPowerOff(false),
  m_rumbleGenerator(new CRumbleGenerator)
{
  m_features.push_back(FEATURE_JOYSTICK);
  // FEATURE_RUMBLE conditionally added via SetMotorCount()
}

CPeripheralJoystick::~CPeripheralJoystick(void)
{
  if (m_rumbleGenerator)
  {
    m_rumbleGenerator->AbortRumble();
    m_rumbleGenerator.reset();
  }

  if (m_joystickMonitor)
  {
    UnregisterInputHandler(m_joystickMonitor.get());
    m_joystickMonitor.reset();
  }

  m_appInput.reset();
  m_deadzoneFilter.reset();
  m_buttonMap.reset();
}

bool CPeripheralJoystick::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_JOYSTICK)
    {
      // Ensure an add-on is present to translate input
      if (!m_manager.GetAddonWithButtonMap(this))
      {
        CLog::Log(LOGERROR, "CPeripheralJoystick: No button mapping add-on for %s", m_strLocation.c_str());
      }
      else
      {
        if (m_bus->InitializeProperties(*this))
          bSuccess = true;
        else
          CLog::Log(LOGERROR, "CPeripheralJoystick: Invalid location (%s)", m_strLocation.c_str());
      }

      if (bSuccess)
      {
        InitializeDeadzoneFiltering();

        // Give joystick monitor priority over default controller
        m_appInput.reset(new CKeymapHandling(this, false, m_manager.GetInputManager().KeymapEnvironment()));
        m_joystickMonitor.reset(new CJoystickMonitor);
        RegisterInputHandler(m_joystickMonitor.get(), false);
      }
    }
    else if (feature == FEATURE_RUMBLE)
    {
      bSuccess = true; // Nothing to do
    }
    else if (feature == FEATURE_POWER_OFF)
    {
      bSuccess = true; // Nothing to do
    }
  }

  return bSuccess;
}

void CPeripheralJoystick::InitializeDeadzoneFiltering()
{
  // Get a button map for deadzone filtering
  PeripheralAddonPtr addon = m_manager.GetAddonWithButtonMap(this);
  if (addon)
  {
    m_buttonMap.reset(new CAddonButtonMap(this, addon, DEFAULT_CONTROLLER_ID));
    if (m_buttonMap->Load())
    {
      m_deadzoneFilter.reset(new CDeadzoneFilter(m_buttonMap.get(), this));
    }
    else
    {
      CLog::Log(LOGERROR, "CPeripheralJoystick: Failed to load button map for deadzone filtering on %s", m_strLocation.c_str());
      m_buttonMap.reset();
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CPeripheralJoystick: Failed to create button map for deadzone filtering on %s", m_strLocation.c_str());
  }
}

void CPeripheralJoystick::OnUserNotification()
{
  IInputReceiver *inputReceiver = m_appInput->GetInputReceiver(m_rumbleGenerator->ControllerID());
  m_rumbleGenerator->NotifyUser(inputReceiver);
}

bool CPeripheralJoystick::TestFeature(PeripheralFeature feature)
{
  bool bSuccess = false;

  switch (feature)
  {
  case FEATURE_RUMBLE:
  {
    IInputReceiver *inputReceiver = m_appInput->GetInputReceiver(m_rumbleGenerator->ControllerID());
    bSuccess = m_rumbleGenerator->DoTest(inputReceiver);
    break;
  }
  case FEATURE_POWER_OFF:
    if (m_supportsPowerOff)
    {
      PowerOff();
      bSuccess = true;
    }
    break;
  default:
    break;
  }

  return bSuccess;
}

void CPeripheralJoystick::PowerOff()
{
  m_bus->PowerOff(m_strLocation);
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

IKeymap *CPeripheralJoystick::GetKeymap(const std::string &controllerId)
{
  return m_appInput->GetKeymap(controllerId);
}

bool CPeripheralJoystick::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  CLog::Log(LOGDEBUG, "BUTTON [ %u ] on \"%s\" %s", buttonIndex,
            DeviceName().c_str(), bPressed ? "pressed" : "released");

  // Avoid sending activated input if the app is in the background
  if (bPressed && !g_application.IsAppFocused())
    return false;

  m_lastActive = CDateTime::GetCurrentDateTime();

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
  CLog::Log(LOGDEBUG, "HAT [ %u ] on \"%s\" %s", hatIndex,
            DeviceName().c_str(), CJoystickTranslator::HatStateToString(state));

  // Avoid sending activated input if the app is in the background
  if (state != HAT_STATE::NONE && !g_application.IsAppFocused())
    return false;

  m_lastActive = CDateTime::GetCurrentDateTime();

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
      if (state == HAT_STATE::NONE)
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
  // Get axis properties
  int center = 0;
  unsigned int range = 1;
  if (m_buttonMap)
    m_buttonMap->GetAxisProperties(axisIndex, center, range);

  // Apply deadzone filtering
  if (center == 0 && m_deadzoneFilter)
    position = m_deadzoneFilter->FilterAxis(axisIndex, position);

  // Avoid sending activated input if the app is in the background
  if (position != static_cast<float>(center) && !g_application.IsAppFocused())
    return false;

  CSingleLock lock(m_handlerMutex);

  // Process promiscuous handlers
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (it->bPromiscuous)
      it->handler->OnAxisMotion(axisIndex, position, center, range);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin(); it != m_driverHandlers.end(); ++it)
  {
    if (!it->bPromiscuous)
    {
      bHandled |= it->handler->OnAxisMotion(axisIndex, position, center, range);

      // If axis is centered, force bHandled to false to notify all handlers.
      // This avoids "sticking".
      if (position == static_cast<float>(center))
        bHandled = false;

      // Once an axis is handled, we're done
      if (bHandled)
        break;
    }
  }

  if (bHandled)
    m_lastActive = CDateTime::GetCurrentDateTime();

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

void CPeripheralJoystick::SetSupportsPowerOff(bool bSupportsPowerOff)
{
  m_supportsPowerOff = bSupportsPowerOff;

  if (!m_supportsPowerOff)
    m_features.erase(std::remove(m_features.begin(), m_features.end(), FEATURE_POWER_OFF), m_features.end());
  else if (std::find(m_features.begin(), m_features.end(), FEATURE_POWER_OFF) == m_features.end())
    m_features.push_back(FEATURE_POWER_OFF);
}
