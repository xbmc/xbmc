/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientInput.h"

#include "GameClientController.h"
#include "GameClientHardware.h"
#include "GameClientJoystick.h"
#include "GameClientKeyboard.h"
#include "GameClientMouse.h"
#include "GameClientPort.h"
#include "GameClientTopology.h"
#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientCallbacks.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTopology.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/EventLockHandle.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGameClientInput::CGameClientInput(CGameClient& gameClient,
                                   AddonInstance_Game& addonStruct,
                                   CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess), m_topology(new CGameClientTopology)
{
}

CGameClientInput::~CGameClientInput()
{
  Deinitialize();
}

void CGameClientInput::Initialize()
{
  LoadTopology();

  ActivateControllers(m_topology->ControllerTree());

  SetControllerLayouts(m_topology->ControllerTree().GetControllers());
}

void CGameClientInput::Start(IGameInputCallback* input)
{
  m_inputCallback = input;

  const CControllerTree& controllers = m_topology->ControllerTree();

  // Open keyboard
  //! @todo Move to player manager
  if (SupportsKeyboard())
  {
    auto it = std::find_if(
        controllers.Ports().begin(), controllers.Ports().end(),
        [](const CControllerPortNode& port) { return port.PortType() == PORT_TYPE::KEYBOARD; });

    OpenKeyboard(it->ActiveController().Controller());
  }

  // Open mouse
  //! @todo Move to player manager
  if (SupportsMouse())
  {
    auto it = std::find_if(
        controllers.Ports().begin(), controllers.Ports().end(),
        [](const CControllerPortNode& port) { return port.PortType() == PORT_TYPE::MOUSE; });

    OpenMouse(it->ActiveController().Controller());
  }

  // Open joysticks
  //! @todo Move to player manager
  for (const auto& port : controllers.Ports())
  {
    if (port.PortType() == PORT_TYPE::CONTROLLER && !port.CompatibleControllers().empty())
    {
      ControllerPtr controller = port.ActiveController().Controller();
      OpenJoystick(port.Address(), controller);
    }
  }

  // Ensure hardware is open to receive events
  m_hardware.reset(new CGameClientHardware(m_gameClient));

  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetPeripherals().RegisterObserver(this);
}

void CGameClientInput::Deinitialize()
{
  Stop();

  m_topology->Clear();
  m_controllerLayouts.clear();
}

void CGameClientInput::Stop()
{
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetPeripherals().UnregisterObserver(this);

  m_hardware.reset();

  std::vector<std::string> ports;
  for (const auto& it : m_joysticks)
    ports.emplace_back(it.first);

  for (const std::string& port : ports)
    CloseJoystick(port);
  m_portMap.clear();

  CloseMouse();

  CloseKeyboard();

  m_inputCallback = nullptr;
}

bool CGameClientInput::HasFeature(const std::string& controllerId,
                                  const std::string& featureName) const
{
  bool bHasFeature = false;

  try
  {
    bHasFeature =
        m_struct.toAddon->HasFeature(&m_struct, controllerId.c_str(), featureName.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: {}: exception caught in HasFeature()", m_gameClient.ID());

    // Fail gracefully
    bHasFeature = true;
  }

  return bHasFeature;
}

bool CGameClientInput::AcceptsInput() const
{
  if (m_inputCallback != nullptr)
    return m_inputCallback->AcceptsInput();

  return false;
}

bool CGameClientInput::InputEvent(const game_input_event& event)
{
  bool bHandled = false;

  try
  {
    bHandled = m_struct.toAddon->InputEvent(&m_struct, &event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: {}: exception caught in InputEvent()", m_gameClient.ID());
  }

  return bHandled;
}

void CGameClientInput::LoadTopology()
{
  game_input_topology* topologyStruct = nullptr;

  if (m_gameClient.Initialized())
  {
    try
    {
      topologyStruct = m_struct.toAddon->GetTopology(&m_struct);
    }
    catch (...)
    {
      m_gameClient.LogException("GetTopology()");
    }
  }

  GameClientPortVec hardwarePorts;
  int playerLimit = -1;

  if (topologyStruct != nullptr)
  {
    //! @todo Guard against infinite loops provided by the game client

    game_input_port* ports = topologyStruct->ports;
    if (ports != nullptr)
    {
      for (unsigned int i = 0; i < topologyStruct->port_count; i++)
        hardwarePorts.emplace_back(new CGameClientPort(ports[i]));
    }

    playerLimit = topologyStruct->player_limit;

    try
    {
      m_struct.toAddon->FreeTopology(&m_struct, topologyStruct);
    }
    catch (...)
    {
      m_gameClient.LogException("FreeTopology()");
    }
  }

  // If no topology is available, create a default one with a single port that
  // accepts all controllers imported by addon.xml
  if (hardwarePorts.empty())
    hardwarePorts.emplace_back(new CGameClientPort(GetControllers(m_gameClient)));

  m_topology.reset(new CGameClientTopology(std::move(hardwarePorts), playerLimit));
}

void CGameClientInput::ActivateControllers(CControllerHub& hub)
{
  for (auto& port : hub.Ports())
  {
    port.SetConnected(true);
    port.SetActiveController(0);
    ActivateControllers(port.ActiveController().Hub());
  }
}

void CGameClientInput::SetControllerLayouts(const ControllerVector& controllers)
{
  if (controllers.empty())
    return;

  for (const auto& controller : controllers)
  {
    const std::string controllerId = controller->ID();
    if (m_controllerLayouts.find(controllerId) == m_controllerLayouts.end())
      m_controllerLayouts[controllerId].reset(new CGameClientController(*this, controller));
  }

  std::vector<game_controller_layout> controllerStructs;
  for (const auto& it : m_controllerLayouts)
    controllerStructs.emplace_back(it.second->TranslateController());

  try
  {
    m_struct.toAddon->SetControllerLayouts(&m_struct, controllerStructs.data(),
                                           static_cast<unsigned int>(controllerStructs.size()));
  }
  catch (...)
  {
    m_gameClient.LogException("SetControllerLayouts()");
  }
}

const CControllerTree& CGameClientInput::GetControllerTree() const
{
  return m_topology->ControllerTree();
}

bool CGameClientInput::SupportsKeyboard() const
{
  const CControllerTree& controllers = m_topology->ControllerTree();

  auto it = std::find_if(
      controllers.Ports().begin(), controllers.Ports().end(),
      [](const CControllerPortNode& port) { return port.PortType() == PORT_TYPE::KEYBOARD; });

  return it != controllers.Ports().end() && !it->CompatibleControllers().empty();
}

bool CGameClientInput::SupportsMouse() const
{
  const CControllerTree& controllers = m_topology->ControllerTree();

  auto it = std::find_if(
      controllers.Ports().begin(), controllers.Ports().end(),
      [](const CControllerPortNode& port) { return port.PortType() == PORT_TYPE::MOUSE; });

  return it != controllers.Ports().end() && !it->CompatibleControllers().empty();
}

bool CGameClientInput::HasAgent() const
{
  //! @todo We check m_portMap instead of m_joysticks because m_joysticks is
  //        always populated with the default joystick configuration (i.e.
  //        all ports are connected to the first controller they accept).
  //        The game has no way of knowing which joysticks are actually being
  //        controlled by agents -- this information is stored in m_portMap,
  //        which is not exposed to the game.
  if (!m_portMap.empty())
    return true;

  if (m_keyboard)
    return true;

  if (m_mouse)
    return true;

  return false;
}

bool CGameClientInput::OpenKeyboard(const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open keyboard, no controller given");
    return false;
  }

  //! @todo Move to player manager
  PERIPHERALS::PeripheralVector keyboards;
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(keyboards,
                                                             PERIPHERALS::FEATURE_KEYBOARD);
  if (keyboards.empty())
    return false;

  bool bSuccess = false;

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon->EnableKeyboard(&m_struct, true, controller->ID().c_str());
      }
      catch (...)
      {
        m_gameClient.LogException("EnableKeyboard()");
      }
    }
  }

  if (bSuccess)
  {
    m_keyboard.reset(
        new CGameClientKeyboard(m_gameClient, controller->ID(), keyboards.at(0).get()));
    return true;
  }

  return false;
}

void CGameClientInput::CloseKeyboard()
{
  m_keyboard.reset();

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        m_struct.toAddon->EnableKeyboard(&m_struct, false, "");
      }
      catch (...)
      {
        m_gameClient.LogException("EnableKeyboard()");
      }
    }
  }
}

bool CGameClientInput::OpenMouse(const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open mouse, no controller given");
    return false;
  }

  //! @todo Move to player manager
  PERIPHERALS::PeripheralVector mice;
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(mice, PERIPHERALS::FEATURE_MOUSE);
  if (mice.empty())
    return false;

  bool bSuccess = false;

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon->EnableMouse(&m_struct, true, controller->ID().c_str());
      }
      catch (...)
      {
        m_gameClient.LogException("EnableMouse()");
      }
    }
  }

  if (bSuccess)
  {
    m_mouse.reset(new CGameClientMouse(m_gameClient, controller->ID(), mice.at(0).get()));
    return true;
  }

  return false;
}

void CGameClientInput::CloseMouse()
{
  m_mouse.reset();

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        m_struct.toAddon->EnableMouse(&m_struct, false, "");
      }
      catch (...)
      {
        m_gameClient.LogException("EnableMouse()");
      }
    }
  }
}

bool CGameClientInput::OpenJoystick(const std::string& portAddress, const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open port \"{}\", no controller given", portAddress);
    return false;
  }

  const CControllerTree& controllerTree = m_topology->ControllerTree();

  const CControllerPortNode& port = controllerTree.GetPort(portAddress);
  if (!port.IsControllerAccepted(portAddress, controller->ID()))
  {
    CLog::Log(LOGERROR, "Failed to open port: Invalid controller \"{}\" on port \"{}\"",
              controller->ID(), portAddress);
    return false;
  }

  bool bSuccess = false;

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon->ConnectController(&m_struct, true, portAddress.c_str(),
                                                       controller->ID().c_str());
      }
      catch (...)
      {
        m_gameClient.LogException("ConnectController()");
      }
    }
  }

  if (bSuccess)
  {
    PERIPHERALS::EventLockHandlePtr lock = CServiceBroker::GetPeripherals().RegisterEventLock();

    m_joysticks[portAddress].reset(new CGameClientJoystick(m_gameClient, portAddress, controller));
    ProcessJoysticks();

    return true;
  }

  return false;
}

void CGameClientInput::CloseJoystick(const std::string& portAddress)
{
  auto it = m_joysticks.find(portAddress);
  if (it != m_joysticks.end())
  {
    std::unique_ptr<CGameClientJoystick> joystick = std::move(it->second);
    m_joysticks.erase(it);
    {
      PERIPHERALS::EventLockHandlePtr lock = CServiceBroker::GetPeripherals().RegisterEventLock();

      ProcessJoysticks();
      joystick.reset();
    }
  }

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        m_struct.toAddon->ConnectController(&m_struct, false, portAddress.c_str(), "");
      }
      catch (...)
      {
        m_gameClient.LogException("ConnectController()");
      }
    }
  }
}

void CGameClientInput::HardwareReset()
{
  if (m_hardware)
    m_hardware->OnResetButton();
}

bool CGameClientInput::ReceiveInputEvent(const game_input_event& event)
{
  bool bHandled = false;

  switch (event.type)
  {
    case GAME_INPUT_EVENT_MOTOR:
      if (event.port_address != nullptr && event.feature_name != nullptr)
        bHandled = SetRumble(event.port_address, event.feature_name, event.motor.magnitude);
      break;
    default:
      break;
  }

  return bHandled;
}

bool CGameClientInput::SetRumble(const std::string& portAddress,
                                 const std::string& feature,
                                 float magnitude)
{
  bool bHandled = false;

  auto it = m_joysticks.find(portAddress);
  if (it != m_joysticks.end())
    bHandled = it->second->SetRumble(feature, magnitude);

  return bHandled;
}

void CGameClientInput::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessagePeripheralsChanged:
    {
      PERIPHERALS::EventLockHandlePtr lock = CServiceBroker::GetPeripherals().RegisterEventLock();

      ProcessJoysticks();

      break;
    }
    default:
      break;
  }
}

void CGameClientInput::ProcessJoysticks()
{
  PERIPHERALS::PeripheralVector joysticks;
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(joysticks,
                                                             PERIPHERALS::FEATURE_JOYSTICK);

  // Update expired joysticks
  PortMap portMapCopy = m_portMap;
  for (auto& it : portMapCopy)
  {
    JOYSTICK::IInputProvider* inputProvider = it.first;
    CGameClientJoystick* gameJoystick = it.second;

    const bool bExpired =
        std::find_if(joysticks.begin(), joysticks.end(),
                     [inputProvider](const PERIPHERALS::PeripheralPtr& joystick) {
                       return inputProvider ==
                              static_cast<JOYSTICK::IInputProvider*>(joystick.get());
                     }) == joysticks.end();

    if (bExpired)
    {
      gameJoystick->UnregisterInput(nullptr);
      m_portMap.erase(inputProvider);
    }
  }

  // Perform the port mapping
  PortMap newPortMap = MapJoysticks(joysticks, m_joysticks);

  // Update connected joysticks
  for (auto& peripheralJoystick : joysticks)
  {
    // Upcast to input interface
    JOYSTICK::IInputProvider* inputProvider = peripheralJoystick.get();

    auto itConnectedPort = newPortMap.find(inputProvider);
    auto itDisconnectedPort = m_portMap.find(inputProvider);

    CGameClientJoystick* newJoystick =
        itConnectedPort != newPortMap.end() ? itConnectedPort->second : nullptr;
    CGameClientJoystick* oldJoystick =
        itDisconnectedPort != m_portMap.end() ? itDisconnectedPort->second : nullptr;

    if (oldJoystick != newJoystick)
    {
      // Unregister old input handler
      if (oldJoystick != nullptr)
      {
        oldJoystick->UnregisterInput(inputProvider);
        m_portMap.erase(itDisconnectedPort);
      }

      // Register new handler
      if (newJoystick != nullptr)
      {
        newJoystick->RegisterInput(inputProvider);
        m_portMap[inputProvider] = newJoystick;
      }
    }
  }
}

CGameClientInput::PortMap CGameClientInput::MapJoysticks(
    const PERIPHERALS::PeripheralVector& peripheralJoysticks,
    const JoystickMap& gameClientjoysticks) const
{
  PortMap result;

  //! @todo Preserve existing joystick ports

  // Sort by order of last button press
  PERIPHERALS::PeripheralVector sortedJoysticks = peripheralJoysticks;
  std::sort(sortedJoysticks.begin(), sortedJoysticks.end(),
            [](const PERIPHERALS::PeripheralPtr& lhs, const PERIPHERALS::PeripheralPtr& rhs) {
              if (lhs->LastActive().IsValid() && !rhs->LastActive().IsValid())
                return true;
              if (!lhs->LastActive().IsValid() && rhs->LastActive().IsValid())
                return false;

              return lhs->LastActive() > rhs->LastActive();
            });

  unsigned int i = 0;
  for (const auto& it : gameClientjoysticks)
  {
    if (i >= peripheralJoysticks.size())
      break;

    // Check topology player limit
    const int playerLimit = m_topology->PlayerLimit();
    if (playerLimit >= 0 && static_cast<int>(i) >= playerLimit)
      break;

    // Dereference iterators
    const PERIPHERALS::PeripheralPtr& peripheralJoystick = sortedJoysticks[i++];
    const std::unique_ptr<CGameClientJoystick>& gameClientJoystick = it.second;

    // Map input provider to input handler
    result[peripheralJoystick.get()] = gameClientJoystick.get();
  }

  return result;
}

ControllerVector CGameClientInput::GetControllers(const CGameClient& gameClient)
{
  using namespace ADDON;

  ControllerVector controllers;

  CGameServices& gameServices = CServiceBroker::GetGameServices();

  const auto& dependencies = gameClient.GetDependencies();
  for (auto it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    ControllerPtr controller = gameServices.GetController(it->id);
    if (controller)
      controllers.push_back(controller);
  }

  if (controllers.empty())
  {
    // Use the default controller
    ControllerPtr controller = gameServices.GetDefaultController();
    if (controller)
      controllers.push_back(controller);
  }

  return controllers;
}
