/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GameClientInput.h"
#include "GameClientHardware.h"
#include "GameClientJoystick.h"
#include "GameClientKeyboard.h"
#include "GameClientMouse.h"
#include "GameClientPort.h"
#include "GameClientTopology.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTopology.h"
#include "games/GameServices.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/EventLockHandle.h"
#include "peripherals/Peripherals.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

CGameClientInput::CGameClientInput(CGameClient &gameClient,
                                   AddonInstance_Game &addonStruct,
                                   CCriticalSection &clientAccess) :
  CGameClientSubsystem(gameClient, addonStruct, clientAccess)
{
}

CGameClientInput::~CGameClientInput()
{
  Deinitialize();
}

void CGameClientInput::Initialize()
{
  LoadTopology();
}

void CGameClientInput::Start()
{
  // Open keyboard
  //! @todo Move to player manager
  if (SupportsKeyboard())
  {
    auto it = std::find_if(m_controllers.Ports().begin(), m_controllers.Ports().end(),
      [](const CControllerPortNode &port)
      {
        return port.PortType() == PORT_TYPE::KEYBOARD;
      });

    OpenKeyboard(it->CompatibleControllers().at(0).Controller());
  }

  // Open mouse
  //! @todo Move to player manager
  if (SupportsMouse())
  {
    auto it = std::find_if(m_controllers.Ports().begin(), m_controllers.Ports().end(),
      [](const CControllerPortNode &port)
      {
        return port.PortType() == PORT_TYPE::MOUSE;
      });

    OpenMouse(it->CompatibleControllers().at(0).Controller());
  }

  // Open joysticks
  //! @todo Move to player manager
  for (const auto &port : m_controllers.Ports())
  {
    if (port.PortType() == PORT_TYPE::CONTROLLER && !port.CompatibleControllers().empty())
    {
      ControllerPtr controller = port.CompatibleControllers().at(0).Controller();
      OpenJoystick(port.Address(), controller);
    }
  }

  // Ensure hardware is open to receive events
  m_hardware.reset(new CGameClientHardware(m_gameClient));
}

void CGameClientInput::Deinitialize()
{
  Stop();
}

void CGameClientInput::Stop()
{
  m_hardware.reset();

  std::vector<std::string> ports;
  for (const auto &it : m_joysticks)
    ports.emplace_back(it.first);

  for (const std::string &port : ports)
    CloseJoystick(port);
  m_portMap.clear();

  CloseMouse();

  CloseKeyboard();
}

bool CGameClientInput::AcceptsInput() const
{
  return CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog() == WINDOW_FULLSCREEN_GAME;
}

void CGameClientInput::LoadTopology()
{
  game_input_topology *topologyStruct = nullptr;

  if (m_gameClient.Initialized())
  {
    try { topologyStruct = m_struct.toAddon.GetTopology(); }
    catch (...) { m_gameClient.LogException("GetTopology()"); }
  }

  GameClientPortVec hardwarePorts;

  if (topologyStruct != nullptr)
  {
    //! @todo Guard against infinite loops provided by the game client

    game_input_port *ports = topologyStruct->ports;
    if (ports != nullptr)
    {
      for (unsigned int i = 0; i < topologyStruct->port_count; i++)
        hardwarePorts.emplace_back(new CGameClientPort(ports[i]));
    }

    m_playerLimit = topologyStruct->player_limit;

    try { m_struct.toAddon.FreeTopology(topologyStruct); }
    catch (...) { m_gameClient.LogException("FreeTopology()"); }
  }

  // If no topology is available, create a default one with a single port that
  // accepts all controllers imported by addon.xml
  if (hardwarePorts.empty())
    hardwarePorts.emplace_back(new CGameClientPort(GetControllers(m_gameClient)));

  CGameClientTopology topology(std::move(hardwarePorts));
  m_controllers = topology.GetControllerTree();
}

bool CGameClientInput::SupportsKeyboard() const
{
  auto it = std::find_if(m_controllers.Ports().begin(), m_controllers.Ports().end(),
    [](const CControllerPortNode &port)
    {
      return port.PortType() == PORT_TYPE::KEYBOARD;
    });

  return it != m_controllers.Ports().end() && !it->CompatibleControllers().empty();
}

bool CGameClientInput::SupportsMouse() const
{
  auto it = std::find_if(m_controllers.Ports().begin(), m_controllers.Ports().end(),
    [](const CControllerPortNode &port)
    {
      return port.PortType() == PORT_TYPE::MOUSE;
    });

  return it != m_controllers.Ports().end() && !it->CompatibleControllers().empty();
}

bool CGameClientInput::OpenKeyboard(const ControllerPtr &controller)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open keyboard, no controller given");
    return false;
  }

  //! @todo Move to player manager
  PERIPHERALS::PeripheralVector keyboards;
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(keyboards, PERIPHERALS::FEATURE_KEYBOARD);
  if (keyboards.empty())
    return false;

  std::string controllerId = controller->ID();

  game_controller controllerStruct{};

  controllerStruct.controller_id        = controllerId.c_str();
  controllerStruct.digital_button_count = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
  controllerStruct.analog_button_count  = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
  controllerStruct.analog_stick_count   = controller->FeatureCount(FEATURE_TYPE::ANALOG_STICK);
  controllerStruct.accelerometer_count  = controller->FeatureCount(FEATURE_TYPE::ACCELEROMETER);
  controllerStruct.key_count            = controller->FeatureCount(FEATURE_TYPE::KEY);
  controllerStruct.rel_pointer_count    = controller->FeatureCount(FEATURE_TYPE::RELPOINTER);
  controllerStruct.abs_pointer_count    = controller->FeatureCount(FEATURE_TYPE::ABSPOINTER);
  controllerStruct.motor_count          = controller->FeatureCount(FEATURE_TYPE::MOTOR);

  bool bSuccess = false;

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon.EnableKeyboard(true, &controllerStruct);
      }
      catch (...)
      {
        m_gameClient.LogException("EnableKeyboard()");
      }
    }
  }

  if (bSuccess)
  {
    m_keyboard.reset(new CGameClientKeyboard(m_gameClient, controllerId, m_struct.toAddon, keyboards.at(0).get()));
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
        m_struct.toAddon.EnableKeyboard(false, nullptr);
      }
      catch (...)
      {
        m_gameClient.LogException("EnableKeyboard()");
      }
    }
  }
}

bool CGameClientInput::OpenMouse(const ControllerPtr &controller)
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

  std::string controllerId = controller->ID();

  game_controller controllerStruct{};

  controllerStruct.controller_id        = controllerId.c_str();
  controllerStruct.digital_button_count = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
  controllerStruct.analog_button_count  = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
  controllerStruct.analog_stick_count   = controller->FeatureCount(FEATURE_TYPE::ANALOG_STICK);
  controllerStruct.accelerometer_count  = controller->FeatureCount(FEATURE_TYPE::ACCELEROMETER);
  controllerStruct.key_count            = controller->FeatureCount(FEATURE_TYPE::KEY);
  controllerStruct.rel_pointer_count    = controller->FeatureCount(FEATURE_TYPE::RELPOINTER);
  controllerStruct.abs_pointer_count    = controller->FeatureCount(FEATURE_TYPE::ABSPOINTER);
  controllerStruct.motor_count          = controller->FeatureCount(FEATURE_TYPE::MOTOR);

  bool bSuccess = false;

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon.EnableMouse(true, &controllerStruct);
      }
      catch (...)
      {
        m_gameClient.LogException("EnableMouse()");
      }
    }
  }

  if (bSuccess)
  {
    m_mouse.reset(new CGameClientMouse(m_gameClient, controllerId, m_struct.toAddon, mice.at(0).get()));
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
        m_struct.toAddon.EnableMouse(false, nullptr);
      }
      catch (...)
      {
        m_gameClient.LogException("EnableMouse()");
      }
    }
  }
}

bool CGameClientInput::OpenJoystick(const std::string &portAddress, const ControllerPtr &controller)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open port \"%s\", no controller given", portAddress.c_str());
    return false;
  }

  const CControllerPortNode &port = m_controllers.GetPort(portAddress);
  if (!port.IsControllerAccepted(portAddress, controller->ID()))
  {
    CLog::Log(LOGERROR, "Failed to open port: Invalid controller \"%s\" on port \"%s\"",
              controller->ID().c_str(), portAddress.c_str());
    return false;
  }

  std::string strId = controller->ID();

  game_controller controllerStruct{};

  controllerStruct.controller_id        = strId.c_str();
  controllerStruct.provides_input       = controller->Topology().ProvidesInput();
  controllerStruct.digital_button_count = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
  controllerStruct.analog_button_count  = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
  controllerStruct.analog_stick_count   = controller->FeatureCount(FEATURE_TYPE::ANALOG_STICK);
  controllerStruct.accelerometer_count  = controller->FeatureCount(FEATURE_TYPE::ACCELEROMETER);
  controllerStruct.key_count            = controller->FeatureCount(FEATURE_TYPE::KEY);
  controllerStruct.rel_pointer_count    = controller->FeatureCount(FEATURE_TYPE::RELPOINTER);
  controllerStruct.abs_pointer_count    = controller->FeatureCount(FEATURE_TYPE::ABSPOINTER);
  controllerStruct.motor_count          = controller->FeatureCount(FEATURE_TYPE::MOTOR);

  bool bSuccess = false;

  {
    CSingleLock lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon.ConnectController(true, portAddress.c_str(), &controllerStruct);
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

    m_joysticks[portAddress].reset(new CGameClientJoystick(m_gameClient, portAddress, controller, m_struct.toAddon));
    ProcessJoysticks();

    return true;
  }

  return false;
}

void CGameClientInput::CloseJoystick(const std::string &portAddress)
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
        m_struct.toAddon.ConnectController(false, portAddress.c_str(), nullptr);
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

bool CGameClientInput::SetRumble(const std::string &portAddress, const std::string& feature, float magnitude)
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
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(joysticks, PERIPHERALS::FEATURE_JOYSTICK);

  // Perform the port mapping
  PortMap newPortMap = MapJoysticks(joysticks, m_joysticks);

  // Update each joystick
  for (auto& peripheralJoystick : joysticks)
  {
    // Upcast to input interface
    JOYSTICK::IInputProvider *inputProvider = peripheralJoystick.get();

    auto itConnectedPort = newPortMap.find(inputProvider);
    auto itDisconnectedPort = m_portMap.find(inputProvider);

    CGameClientJoystick* newJoystick = itConnectedPort != newPortMap.end() ? itConnectedPort->second : nullptr;
    CGameClientJoystick* oldJoystick = itDisconnectedPort != m_portMap.end() ? itDisconnectedPort->second : nullptr;

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

CGameClientInput::PortMap CGameClientInput::MapJoysticks(const PERIPHERALS::PeripheralVector &peripheralJoysticks,
                                                         const JoystickMap &gameClientjoysticks) const
{
  PortMap result;

  //! @todo Preserve existing joystick ports

  unsigned int i = 0;
  for (const auto &it : gameClientjoysticks)
  {
    if (i >= peripheralJoysticks.size())
      break;

    // Check topology player limit
    if (m_playerLimit >= 0 && static_cast<int>(i) >= m_playerLimit)
      break;

    // Dereference iterators
    const PERIPHERALS::PeripheralPtr &peripheralJoystick = peripheralJoysticks[i++];
    const std::unique_ptr<CGameClientJoystick> &gameClientJoystick = it.second;

    // Map input provider to input handler
    result[peripheralJoystick.get()] = gameClientJoystick.get();
  }

  return result;
}

ControllerVector CGameClientInput::GetControllers(const CGameClient &gameClient)
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
