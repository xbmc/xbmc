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
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "games/GameServices.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/Peripherals.h"
#include "peripherals/PeripheralTypes.h" //! @todo
//#include "threads/SingleLock.h"
#include "utils/log.h"
#include "ServiceBroker.h"

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
  if (m_gameClient.SupportsKeyboard())
    OpenKeyboard();

  if (m_gameClient.SupportsMouse())
    OpenMouse();
}

void CGameClientInput::Deinitialize()
{
  while (!m_joysticks.empty())
    ClosePort(m_joysticks.begin()->first);

  m_hardware.reset();

  CloseKeyboard();

  CloseMouse();
}

bool CGameClientInput::AcceptsInput() const
{
  return g_windowManager.GetActiveWindowOrDialog() == WINDOW_FULLSCREEN_GAME;
}

bool CGameClientInput::OpenPort(unsigned int port)
{
  // Fail if port is already open
  if (m_joysticks.find(port) != m_joysticks.end())
    return false;

  // Ensure hardware is open to receive events from the port
  if (!m_hardware)
    m_hardware.reset(new CGameClientHardware(m_gameClient));

  ControllerVector controllers = GetControllers(m_gameClient);
  if (!controllers.empty())
  {
    //! @todo Choose controller
    ControllerPtr& controller = controllers[0];

    m_joysticks[port].reset(new CGameClientJoystick(m_gameClient, port, controller, m_struct.toAddon));

    //! @todo
    //CServiceBroker::GetGameServices().PortManager().OpenPort(m_joysticks[port].get(), m_hardware.get(), &m_gameClient, port, device);

    UpdatePort(port, controller);

    return true;
  }

  return false;
}

void CGameClientInput::ClosePort(unsigned int port)
{
  // Can't close port if it doesn't exist
  if (m_joysticks.find(port) == m_joysticks.end())
    return;

  //! @todo
  //CServiceBroker::GetGameServices().PortManager().ClosePort(m_joysticks[port].get());

  m_joysticks.erase(port);

  UpdatePort(port, CController::EmptyPtr);
}

bool CGameClientInput::ReceiveInputEvent(const game_input_event& event)
{
  bool bHandled = false;

  switch (event.type)
  {
    case GAME_INPUT_EVENT_MOTOR:
      if (event.feature_name)
        bHandled = SetRumble(event.port, event.feature_name, event.motor.magnitude);
      break;
    default:
      break;
  }

  return bHandled;
}

void CGameClientInput::UpdatePort(unsigned int port, const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  CSingleLock lock(m_clientAccess);

  if (m_gameClient.Initialized())
  {
    if (controller != CController::EmptyPtr)
    {
      std::string strId = controller->ID();

      game_controller controllerStruct;

      controllerStruct.controller_id        = strId.c_str();
      controllerStruct.digital_button_count = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::DIGITAL);
      controllerStruct.analog_button_count  = controller->FeatureCount(FEATURE_TYPE::SCALAR, INPUT_TYPE::ANALOG);
      controllerStruct.analog_stick_count   = controller->FeatureCount(FEATURE_TYPE::ANALOG_STICK);
      controllerStruct.accelerometer_count  = controller->FeatureCount(FEATURE_TYPE::ACCELEROMETER);
      controllerStruct.key_count            = controller->FeatureCount(FEATURE_TYPE::KEY);
      controllerStruct.rel_pointer_count    = controller->FeatureCount(FEATURE_TYPE::RELPOINTER);
      controllerStruct.abs_pointer_count    = controller->FeatureCount(FEATURE_TYPE::ABSPOINTER);
      controllerStruct.motor_count          = controller->FeatureCount(FEATURE_TYPE::MOTOR);

      try { m_struct.toAddon.UpdatePort(port, true, &controllerStruct); }
      catch (...) { m_gameClient.LogException("UpdatePort()"); }
    }
    else
    {
      try { m_struct.toAddon.UpdatePort(port, false, nullptr); }
      catch (...) { m_gameClient.LogException("UpdatePort()"); }
    }
  }
}

void CGameClientInput::OpenKeyboard()
{
  using namespace JOYSTICK;

  //! @todo Move to player manager
  PERIPHERALS::PeripheralVector keyboards;
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(keyboards, PERIPHERALS::FEATURE_KEYBOARD);

  if (keyboards.empty())
    return;

  CGameServices& gameServices = CServiceBroker::GetGameServices();

  ControllerPtr controller = gameServices.GetDefaultKeyboard(); //! @todo

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
    m_keyboard.reset(new CGameClientKeyboard(m_gameClient, controllerId, m_struct.toAddon, keyboards.at(0).get()));
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

void CGameClientInput::OpenMouse()
{
  using namespace JOYSTICK;

  //! @todo Move to player manager
  PERIPHERALS::PeripheralVector mice;
  CServiceBroker::GetPeripherals().GetPeripheralsWithFeature(mice, PERIPHERALS::FEATURE_MOUSE);

  if (mice.empty())
    return;

  CGameServices& gameServices = CServiceBroker::GetGameServices();

  ControllerPtr controller = gameServices.GetDefaultMouse(); //! @todo

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
    m_mouse.reset(new CGameClientMouse(m_gameClient, controllerId, m_struct.toAddon, mice.at(0).get()));
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

bool CGameClientInput::SetRumble(unsigned int port, const std::string& feature, float magnitude)
{
  bool bHandled = false;

  if (m_joysticks.find(port) != m_joysticks.end())
    bHandled = m_joysticks[port]->SetRumble(feature, magnitude);

  return bHandled;
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
