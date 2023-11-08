/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientSubsystem.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/types/ControllerTree.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

class CCriticalSection;
struct game_input_event;

namespace KODI
{
namespace JOYSTICK
{
class IInputProvider;
}

namespace GAME
{
class CGameClient;
class CGameClientController;
class CGameClientHardware;
class CGameClientJoystick;
class CGameClientKeyboard;
class CGameClientMouse;
class CGameClientTopology;
class CPortManager;
class IGameInputCallback;

/*!
 * \ingroup games
 */
class CGameClientInput : protected CGameClientSubsystem, public Observable
{
public:
  //! @todo de-duplicate
  using PortAddress = std::string;
  using JoystickMap = std::map<PortAddress, std::shared_ptr<CGameClientJoystick>>;

  CGameClientInput(CGameClient& gameClient,
                   AddonInstance_Game& addonStruct,
                   CCriticalSection& clientAccess);
  ~CGameClientInput() override;

  void Initialize();
  void Deinitialize();

  void Start(IGameInputCallback* input);
  void Stop();

  // Input functions
  bool HasFeature(const std::string& controllerId, const std::string& featureName) const;
  bool AcceptsInput() const;
  bool InputEvent(const game_input_event& event);
  float GetPortActivation(const std::string& portAddress);

  // Topology functions
  const CControllerTree& GetDefaultControllerTree() const;
  CControllerTree GetActiveControllerTree() const;
  bool SupportsKeyboard() const;
  bool SupportsMouse() const;
  int GetPlayerLimit() const;
  bool ConnectController(const std::string& portAddress, const ControllerPtr& controller);
  bool DisconnectController(const std::string& portAddress);
  void SavePorts();
  void ResetPorts();

  // Joystick functions
  const JoystickMap& GetJoystickMap() const { return m_joysticks; }
  void CloseJoysticks(PERIPHERALS::EventLockHandlePtr& inputHandlingLock);

  // Keyboard functions
  bool OpenKeyboard(const ControllerPtr& controller, const PERIPHERALS::PeripheralPtr& keyboard);
  bool IsKeyboardOpen() const;
  void CloseKeyboard();

  // Mouse functions
  bool OpenMouse(const ControllerPtr& controller, const PERIPHERALS::PeripheralPtr& mouse);
  bool IsMouseOpen() const;
  void CloseMouse();

  // Agent functions
  bool HasAgent() const;

  // Hardware input functions
  void HardwareReset();

  // Input callbacks
  bool ReceiveInputEvent(const game_input_event& eventStruct);

private:
  // Private input helpers
  void LoadTopology();
  void SetControllerLayouts(const ControllerVector& controllers);
  bool OpenJoystick(const std::string& portAddress, const ControllerPtr& controller);
  void CloseJoysticks(const CPortNode& port, PERIPHERALS::EventLockHandlePtr& inputHandlingLock);
  void CloseJoystick(const std::string& portAddress,
                     PERIPHERALS::EventLockHandlePtr& inputHandlingLock);

  // Private callback helpers
  bool SetRumble(const std::string& portAddress, const std::string& feature, float magnitude);

  // Helper functions
  static ControllerVector GetControllers(const CGameClient& gameClient);
  static void ActivateControllers(CControllerHub& hub);

  // Input properties
  IGameInputCallback* m_inputCallback = nullptr;
  std::unique_ptr<CGameClientTopology> m_topology;
  using ControllerLayoutMap = std::map<std::string, std::unique_ptr<CGameClientController>>;
  ControllerLayoutMap m_controllerLayouts;

  /*!
   * \brief Map of port address to joystick handler
   *
   * The port address is a string that identifies the adress of the port.
   *
   * The joystick handler connects to joystick input of the game client.
   *
   * This property is always populated with the default joystick configuration
   * (i.e. all ports are connected to the first controller they accept).
   */
  JoystickMap m_joysticks;

  /*!
   * \brief Serializable port state
   */
  std::unique_ptr<CPortManager> m_portManager;

  /*!
   * \brief Mutex for port state
   *
   * Mutex is recursive to allow for management of several ports within the
   * function ResetPorts().
   */
  mutable std::recursive_mutex m_portMutex;

  /*!
   * \brief Keyboard handler
   *
   * This connects to the keyboard input of the game client.
   */
  std::unique_ptr<CGameClientKeyboard> m_keyboard;

  /*!
   * \brief Mouse handler
   *
   * This connects to the mouse input of the game client.
   */
  std::unique_ptr<CGameClientMouse> m_mouse;

  /*!
   * \brief Hardware input handler
   *
   * This connects to input from game console hardware belonging to the game
   * client.
   */
  std::unique_ptr<CGameClientHardware> m_hardware;
};
} // namespace GAME
} // namespace KODI
