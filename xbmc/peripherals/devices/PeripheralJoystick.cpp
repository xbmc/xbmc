/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralJoystick.h"

#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "application/Application.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/ControllerManager.h"
#include "input/InputManager.h"
#include "input/joysticks/DeadzoneFilter.h"
#include "input/joysticks/JoystickMonitor.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/RumbleGenerator.h"
#include "input/joysticks/interfaces/IDriverHandler.h"
#include "input/joysticks/keymaps/KeymapHandling.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <mutex>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CPeripheralJoystick::CPeripheralJoystick(CPeripherals& manager,
                                         const PeripheralScanResult& scanResult,
                                         CPeripheralBus* bus)
  : CPeripheral(manager, scanResult, bus), m_rumbleGenerator(new CRumbleGenerator)
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

  // Wait for remaining install tasks
  for (std::future<void>& task : m_installTasks)
    task.wait();
}

bool CPeripheralJoystick::InitialiseFeature(const PeripheralFeature feature)
{
  bool bSuccess = false;

  if (CPeripheral::InitialiseFeature(feature))
  {
    if (feature == FEATURE_JOYSTICK)
    {
      // Ensure an add-on is present to translate input
      PeripheralAddonPtr addon = m_manager.GetAddonWithButtonMap(this);
      if (!addon)
      {
        CLog::Log(LOGERROR, "CPeripheralJoystick: No button mapping add-on for {}", m_strLocation);
      }
      else
      {
        if (m_bus->InitializeProperties(*this))
          bSuccess = true;
        else
          CLog::Log(LOGERROR, "CPeripheralJoystick: Invalid location ({})", m_strLocation);
      }

      if (bSuccess)
      {
        m_buttonMap = std::make_unique<CAddonButtonMap>(this, addon, DEFAULT_CONTROLLER_ID);
        if (m_buttonMap->Load())
        {
          InitializeDeadzoneFiltering(*m_buttonMap);
          InitializeControllerProfile(*m_buttonMap);
        }
        else
        {
          CLog::Log(LOGERROR, "CPeripheralJoystick: Failed to load button map for {}",
                    m_strLocation);
          m_buttonMap.reset();
        }

        // Give joystick monitor priority over default controller
        m_appInput = std::make_unique<CKeymapHandling>(
            this, false, m_manager.GetInputManager().KeymapEnvironment());
        m_joystickMonitor = std::make_unique<CJoystickMonitor>();
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

void CPeripheralJoystick::InitializeDeadzoneFiltering(IButtonMap& buttonMap)
{
  m_deadzoneFilter = std::make_unique<CDeadzoneFilter>(&buttonMap, this);
}

void CPeripheralJoystick::InitializeControllerProfile(IButtonMap& buttonMap)
{
  const std::string controllerId = buttonMap.GetAppearance();
  if (controllerId.empty())
    return;

  auto controller = m_manager.GetControllerProfiles().GetController(controllerId);
  if (controller)
    CPeripheral::SetControllerProfile(controller);
  else
  {
    std::unique_lock<CCriticalSection> lock(m_controllerInstallMutex);

    // Deposit controller into queue
    m_controllersToInstall.emplace(controllerId);

    // Clean up finished install tasks
    m_installTasks.erase(std::remove_if(m_installTasks.begin(), m_installTasks.end(),
                                        [](std::future<void>& task) {
                                          return task.wait_for(std::chrono::seconds(0)) ==
                                                 std::future_status::ready;
                                        }),
                         m_installTasks.end());

    // Install controller off-thread
    std::future<void> installTask = std::async(std::launch::async, [this]() {
      // Withdraw controller from queue
      std::string controllerToInstall;
      {
        std::unique_lock<CCriticalSection> lock(m_controllerInstallMutex);
        if (!m_controllersToInstall.empty())
        {
          controllerToInstall = m_controllersToInstall.front();
          m_controllersToInstall.pop();
        }
      }

      // Do the install
      GAME::ControllerPtr controller = InstallAsync(controllerToInstall);
      if (controller)
        CPeripheral::SetControllerProfile(controller);
    });

    // Hold the task to prevent the destructor from completing during an install
    m_installTasks.emplace_back(std::move(installTask));
  }
}

void CPeripheralJoystick::OnUserNotification()
{
  IInputReceiver* inputReceiver = m_appInput->GetInputReceiver(m_rumbleGenerator->ControllerID());
  m_rumbleGenerator->NotifyUser(inputReceiver);
}

bool CPeripheralJoystick::TestFeature(PeripheralFeature feature)
{
  bool bSuccess = false;

  switch (feature)
  {
    case FEATURE_RUMBLE:
    {
      IInputReceiver* inputReceiver =
          m_appInput->GetInputReceiver(m_rumbleGenerator->ControllerID());
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
  std::unique_lock<CCriticalSection> lock(m_handlerMutex);

  DriverHandler driverHandler = {handler, bPromiscuous};
  m_driverHandlers.insert(m_driverHandlers.begin(), driverHandler);
}

void CPeripheralJoystick::UnregisterJoystickDriverHandler(IDriverHandler* handler)
{
  std::unique_lock<CCriticalSection> lock(m_handlerMutex);

  m_driverHandlers.erase(std::remove_if(m_driverHandlers.begin(), m_driverHandlers.end(),
                                        [handler](const DriverHandler& driverHandler) {
                                          return driverHandler.handler == handler;
                                        }),
                         m_driverHandlers.end());
}

IKeymap* CPeripheralJoystick::GetKeymap(const std::string& controllerId)
{
  return m_appInput->GetKeymap(controllerId);
}

GAME::ControllerPtr CPeripheralJoystick::ControllerProfile() const
{
  // Button map has the freshest state
  if (m_buttonMap)
  {
    const std::string controllerId = m_buttonMap->GetAppearance();
    if (!controllerId.empty())
    {
      auto controller = m_manager.GetControllerProfiles().GetController(controllerId);
      if (controller)
        return controller;
    }
  }

  // Fall back to last set controller profile
  if (m_controllerProfile)
    return m_controllerProfile;

  // Fall back to default controller
  return m_manager.GetControllerProfiles().GetDefaultController();
}

void CPeripheralJoystick::SetControllerProfile(const KODI::GAME::ControllerPtr& controller)
{
  CPeripheral::SetControllerProfile(controller);

  // Save preference to buttonmap
  if (m_buttonMap)
  {
    if (m_buttonMap->SetAppearance(controller->ID()))
      m_buttonMap->SaveButtonMap();
  }
}

bool CPeripheralJoystick::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  // Silence debug log if controllers are not enabled
  if (m_manager.GetInputManager().IsControllerEnabled())
  {
    CLog::Log(LOGDEBUG, "BUTTON [ {} ] on \"{}\" {}", buttonIndex, DeviceName(),
              bPressed ? "pressed" : "released");
  }

  // Avoid sending activated input if the app is in the background
  if (bPressed && !g_application.IsAppFocused())
    return false;

  m_lastActive = CDateTime::GetCurrentDateTime();

  std::unique_lock<CCriticalSection> lock(m_handlerMutex);

  // Check GUI setting and send button release if controllers are disabled
  if (!m_manager.GetInputManager().IsControllerEnabled())
  {
    for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin();
         it != m_driverHandlers.end(); ++it)
      it->handler->OnButtonMotion(buttonIndex, false);
    return true;
  }

  // Process promiscuous handlers
  for (auto& it : m_driverHandlers)
  {
    if (it.bPromiscuous)
      it.handler->OnButtonMotion(buttonIndex, bPressed);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (auto& it : m_driverHandlers)
  {
    if (!it.bPromiscuous)
    {
      bHandled |= it.handler->OnButtonMotion(buttonIndex, bPressed);

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
  // Silence debug log if controllers are not enabled
  if (m_manager.GetInputManager().IsControllerEnabled())
  {
    CLog::Log(LOGDEBUG, "HAT [ {} ] on \"{}\" {}", hatIndex, DeviceName(),
              CJoystickTranslator::HatStateToString(state));
  }

  // Avoid sending activated input if the app is in the background
  if (state != HAT_STATE::NONE && !g_application.IsAppFocused())
    return false;

  m_lastActive = CDateTime::GetCurrentDateTime();

  std::unique_lock<CCriticalSection> lock(m_handlerMutex);

  // Check GUI setting and send hat unpressed if controllers are disabled
  if (!m_manager.GetInputManager().IsControllerEnabled())
  {
    for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin();
         it != m_driverHandlers.end(); ++it)
      it->handler->OnHatMotion(hatIndex, HAT_STATE::NONE);
    return true;
  }

  // Process promiscuous handlers
  for (auto& it : m_driverHandlers)
  {
    if (it.bPromiscuous)
      it.handler->OnHatMotion(hatIndex, state);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (auto& it : m_driverHandlers)
  {
    if (!it.bPromiscuous)
    {
      bHandled |= it.handler->OnHatMotion(hatIndex, state);

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

  std::unique_lock<CCriticalSection> lock(m_handlerMutex);

  // Check GUI setting and send analog axis centered if controllers are disabled
  if (!m_manager.GetInputManager().IsControllerEnabled())
  {
    for (std::vector<DriverHandler>::iterator it = m_driverHandlers.begin();
         it != m_driverHandlers.end(); ++it)
      it->handler->OnAxisMotion(axisIndex, static_cast<float>(center), center, range);
    return true;
  }

  // Process promiscuous handlers
  for (auto& it : m_driverHandlers)
  {
    if (it.bPromiscuous)
      it.handler->OnAxisMotion(axisIndex, position, center, range);
  }

  bool bHandled = false;

  // Process regular handlers until one is handled
  for (auto& it : m_driverHandlers)
  {
    if (!it.bPromiscuous)
    {
      bHandled |= it.handler->OnAxisMotion(axisIndex, position, center, range);

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

void CPeripheralJoystick::OnInputFrame(void)
{
  std::unique_lock<CCriticalSection> lock(m_handlerMutex);

  for (auto& it : m_driverHandlers)
    it.handler->OnInputFrame();
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
    m_features.erase(std::remove(m_features.begin(), m_features.end(), FEATURE_RUMBLE),
                     m_features.end());
  else if (std::find(m_features.begin(), m_features.end(), FEATURE_RUMBLE) == m_features.end())
    m_features.push_back(FEATURE_RUMBLE);
}

void CPeripheralJoystick::SetSupportsPowerOff(bool bSupportsPowerOff)
{
  m_supportsPowerOff = bSupportsPowerOff;

  if (!m_supportsPowerOff)
    m_features.erase(std::remove(m_features.begin(), m_features.end(), FEATURE_POWER_OFF),
                     m_features.end());
  else if (std::find(m_features.begin(), m_features.end(), FEATURE_POWER_OFF) == m_features.end())
    m_features.push_back(FEATURE_POWER_OFF);
}

GAME::ControllerPtr CPeripheralJoystick::InstallAsync(const std::string& controllerId)
{
  GAME::ControllerPtr controller;

  // Only 1 install at a time. Remaining installs will wake when this one
  // is done.
  std::unique_lock<CCriticalSection> lockInstall(m_manager.GetAddonInstallMutex());

  CLog::LogF(LOGDEBUG, "Installing {}", controllerId);

  if (InstallSync(controllerId))
    controller = m_manager.GetControllerProfiles().GetController(controllerId);
  else
    CLog::LogF(LOGERROR, "Failed to install {}", controllerId);

  return controller;
}

bool CPeripheralJoystick::InstallSync(const std::string& controllerId)
{
  // If the addon isn't installed we need to install it
  bool installed = CServiceBroker::GetAddonMgr().IsAddonInstalled(controllerId);
  if (!installed)
  {
    ADDON::AddonPtr installedAddon;
    installed = ADDON::CAddonInstaller::GetInstance().InstallModal(
        controllerId, installedAddon, ADDON::InstallModalPrompt::CHOICE_NO);
  }

  if (installed)
  {
    // Make sure add-on is enabled
    if (CServiceBroker::GetAddonMgr().IsAddonDisabled(controllerId))
      CServiceBroker::GetAddonMgr().EnableAddon(controllerId);
  }

  return installed;
}
