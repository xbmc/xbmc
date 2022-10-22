/*
 *  Copyright (C) 2017-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameAgentManager.h"

#include "ServiceBroker.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientJoystick.h"
#include "input/InputManager.h"
#include "peripherals/EventLockHandle.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/devices/PeripheralJoystick.h"

using namespace KODI;
using namespace GAME;

CGameAgentManager::CGameAgentManager(PERIPHERALS::CPeripherals& peripheralManager,
                                     CInputManager& inputManager)
  : m_peripheralManager(peripheralManager), m_inputManager(inputManager)
{
  // Register callbacks
  m_peripheralManager.RegisterObserver(this);
  m_inputManager.RegisterKeyboardDriverHandler(this);
  m_inputManager.RegisterMouseDriverHandler(this);
}

CGameAgentManager::~CGameAgentManager()
{
  // Unregister callbacks in reverse order
  m_inputManager.UnregisterMouseDriverHandler(this);
  m_inputManager.UnregisterKeyboardDriverHandler(this);
  m_peripheralManager.UnregisterObserver(this);
}

void CGameAgentManager::Start(GameClientPtr gameClient)
{
  // Initialize state
  m_gameClient = std::move(gameClient);

  // Register callbacks
  if (m_gameClient)
    m_gameClient->Input().RegisterObserver(this);
}

void CGameAgentManager::Stop()
{
  // Unregister callbacks in reverse order
  if (m_gameClient)
    m_gameClient->Input().UnregisterObserver(this);

  // Close open joysticks
  {
    PERIPHERALS::EventLockHandlePtr inputHandlingLock;

    for (const auto& [inputProvider, joystick] : m_portMap)
    {
      if (!inputHandlingLock)
        inputHandlingLock = CServiceBroker::GetPeripherals().RegisterEventLock();

      joystick->UnregisterInput(inputProvider);

      SetChanged(true);
    }

    m_portMap.clear();
  }

  // Notify observers if anything changed
  NotifyObservers(ObservableMessageGameAgentsChanged);

  // Reset state
  m_gameClient.reset();
}

void CGameAgentManager::Refresh()
{
  if (m_gameClient)
  {
    // Open keyboard
    ProcessKeyboard();

    // Open mouse
    ProcessMouse();

    // Open/close joysticks
    PERIPHERALS::EventLockHandlePtr inputHandlingLock;
    ProcessJoysticks(inputHandlingLock);
  }

  // Notify observers if anything changed
  NotifyObservers(ObservableMessageGameAgentsChanged);
}

void CGameAgentManager::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageGamePortsChanged:
    case ObservableMessagePeripheralsChanged:
    {
      Refresh();
      break;
    }
    default:
      break;
  }
}

bool CGameAgentManager::OnKeyPress(const CKey& key)
{
  m_bHasKeyboard = true;
  return false;
}

bool CGameAgentManager::OnPosition(int x, int y)
{
  m_bHasMouse = true;
  return false;
}

bool CGameAgentManager::OnButtonPress(MOUSE::BUTTON_ID button)
{
  m_bHasMouse = true;
  return false;
}

void CGameAgentManager::ProcessJoysticks(PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  // Get system joysticks.
  //
  // It's important to hold these shared pointers for the function scope
  // because we call into the input handlers in m_portMap.
  //
  // The input handlers are upcasted from their peripheral object, so the
  // peripheral object must persist through this function.
  //
  PERIPHERALS::PeripheralVector joysticks;
  m_peripheralManager.GetPeripheralsWithFeature(joysticks, PERIPHERALS::FEATURE_JOYSTICK);

  // Update expired joysticks
  UpdateExpiredJoysticks(joysticks, inputHandlingLock);

  // Perform the port mapping
  PortMap newPortMap = MapJoysticks(joysticks, m_gameClient->Input().GetJoystickMap(),
                                    m_gameClient->Input().GetPlayerLimit());

  // Update connected joysticks
  UpdateConnectedJoysticks(joysticks, newPortMap, inputHandlingLock);
}

void CGameAgentManager::ProcessKeyboard()
{
  if (m_bHasKeyboard && m_gameClient->Input().SupportsKeyboard() &&
      !m_gameClient->Input().IsKeyboardOpen())
  {
    PERIPHERALS::PeripheralVector keyboards;
    CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(keyboards,
                                                               PERIPHERALS::FEATURE_KEYBOARD);
    if (!keyboards.empty())
    {
      const CControllerTree& controllers = m_gameClient->Input().GetActiveControllerTree();

      auto it = std::find_if(
          controllers.GetPorts().begin(), controllers.GetPorts().end(),
          [](const CPortNode& port) { return port.GetPortType() == PORT_TYPE::KEYBOARD; });

      PERIPHERALS::PeripheralPtr keyboard = std::move(keyboards.at(0));
      m_gameClient->Input().OpenKeyboard(it->GetActiveController().GetController(), keyboard);

      SetChanged(true);
    }
  }
}

void CGameAgentManager::ProcessMouse()
{
  if (m_bHasMouse && m_gameClient->Input().SupportsMouse() && !m_gameClient->Input().IsMouseOpen())
  {
    PERIPHERALS::PeripheralVector mice;
    CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(mice, PERIPHERALS::FEATURE_MOUSE);
    if (!mice.empty())
    {
      const CControllerTree& controllers = m_gameClient->Input().GetActiveControllerTree();

      auto it = std::find_if(
          controllers.GetPorts().begin(), controllers.GetPorts().end(),
          [](const CPortNode& port) { return port.GetPortType() == PORT_TYPE::MOUSE; });

      PERIPHERALS::PeripheralPtr mouse = std::move(mice.at(0));
      m_gameClient->Input().OpenMouse(it->GetActiveController().GetController(), mouse);

      SetChanged(true);
    }
  }
}

void CGameAgentManager::UpdateExpiredJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                               PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  // Make a copy - expired joysticks are removed from m_portMap
  PortMap portMapCopy = m_portMap;

  for (const auto& [inputProvider, joystick] : portMapCopy)
  {
    // Structured binding cannot be captured, so make a copy
    JOYSTICK::IInputProvider* const inputProviderCopy = inputProvider;

    // Search peripheral vector for input provider
    auto it2 = std::find_if(joysticks.begin(), joysticks.end(),
                            [inputProviderCopy](const PERIPHERALS::PeripheralPtr& joystick) {
                              // Upcast peripheral to input interface
                              JOYSTICK::IInputProvider* peripheralInput = joystick.get();

                              // Compare
                              return inputProviderCopy == peripheralInput;
                            });

    // If peripheral wasn't found, then it was disconnected
    const bool bDisconnected = (it2 == joysticks.end());

    // Erase joystick from port map if its peripheral becomes disconnected
    if (bDisconnected)
    {
      // Must use nullptr because peripheral has likely fallen out of scope,
      // destroying the object
      joystick->UnregisterInput(nullptr);

      if (!inputHandlingLock)
        inputHandlingLock = CServiceBroker::GetPeripherals().RegisterEventLock();
      m_portMap.erase(inputProvider);

      SetChanged(true);
    }
  }
}

void CGameAgentManager::UpdateConnectedJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                                 const PortMap& newPortMap,
                                                 PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  for (auto& peripheralJoystick : joysticks)
  {
    // Upcast peripheral to input interface
    JOYSTICK::IInputProvider* inputProvider = peripheralJoystick.get();

    // Get connection states
    auto itConnectedPort = newPortMap.find(inputProvider);
    auto itDisconnectedPort = m_portMap.find(inputProvider);

    // Get possibly connected joystick
    std::shared_ptr<CGameClientJoystick> newJoystick;
    if (itConnectedPort != newPortMap.end())
      newJoystick = itConnectedPort->second;

    // Get possibly disconnected joystick
    std::shared_ptr<CGameClientJoystick> oldJoystick;
    if (itDisconnectedPort != m_portMap.end())
      oldJoystick = itDisconnectedPort->second;

    // Check for a change in joysticks
    if (oldJoystick != newJoystick)
    {
      // Unregister old input handler
      if (oldJoystick != nullptr)
      {
        oldJoystick->UnregisterInput(inputProvider);

        if (!inputHandlingLock)
          inputHandlingLock = CServiceBroker::GetPeripherals().RegisterEventLock();
        m_portMap.erase(itDisconnectedPort);

        SetChanged(true);
      }

      // Register new handler
      if (newJoystick != nullptr)
      {
        newJoystick->RegisterInput(inputProvider);

        m_portMap[inputProvider] = std::move(newJoystick);

        SetChanged(true);
      }
    }
  }
}

CGameAgentManager::PortMap CGameAgentManager::MapJoysticks(
    const PERIPHERALS::PeripheralVector& peripheralJoysticks,
    const JoystickMap& gameClientjoysticks,
    int playerLimit)
{
  PortMap result;

  //! @todo Preserve existing joystick ports
  PERIPHERALS::PeripheralVector sortedJoysticks = peripheralJoysticks;
  std::sort(sortedJoysticks.begin(), sortedJoysticks.end(),
            [](const PERIPHERALS::PeripheralPtr& lhs, const PERIPHERALS::PeripheralPtr& rhs) {
              PERIPHERALS::CPeripheralJoystick* lhsJoystick =
                  dynamic_cast<PERIPHERALS::CPeripheralJoystick*>(lhs.get());
              PERIPHERALS::CPeripheralJoystick* rhsJoystick =
                  dynamic_cast<PERIPHERALS::CPeripheralJoystick*>(lhs.get());

              // Sort joysticks before other peripheral types
              if (lhsJoystick && !rhsJoystick)
                return true;
              if (!lhsJoystick && rhsJoystick)
                return false;

              if (lhsJoystick && rhsJoystick)
              {
                // Sort joysticks with a requested port before joysticks without a requested port
                if (lhsJoystick->RequestedPort() != -1 && rhsJoystick->RequestedPort() == -1)
                  return true;
                if (lhsJoystick->RequestedPort() == -1 && rhsJoystick->RequestedPort() != -1)
                  return false;

                // Sort by requested port, if provided
                if (lhsJoystick->RequestedPort() < rhsJoystick->RequestedPort())
                  return true;
                if (lhsJoystick->RequestedPort() > rhsJoystick->RequestedPort())
                  return false;

                // Sort by location on the peripheral bus
                if (lhs->Location() < rhs->Location())
                  return true;
                if (lhs->Location() > rhs->Location())
                  return false;
              }

              // Shouldn't happen
              return false;
            });

  unsigned int i = 0;
  for (const auto& [portAddress, gameClientJoystick] : gameClientjoysticks)
  {
    // Break when we're out of joystick peripherals
    if (i >= peripheralJoysticks.size())
      break;

    // Check topology player limit
    if (playerLimit >= 0 && static_cast<int>(i) >= playerLimit)
      break;

    // Dereference iterator
    PERIPHERALS::PeripheralPtr peripheralJoystick = sortedJoysticks[i];

    // Map input provider to input handler
    result[peripheralJoystick.get()] = gameClientJoystick;

    ++i;
  }

  return result;
}
