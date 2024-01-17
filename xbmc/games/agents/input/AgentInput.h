/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "AgentController.h"
#include "games/GameTypes.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

class CInputManager;

namespace PERIPHERALS
{
class CPeripherals;
} // namespace PERIPHERALS

namespace KODI
{
namespace JOYSTICK
{
class IInputProvider;
}
namespace KEYBOARD
{
class IKeyboardInputProvider;
}
namespace MOUSE
{
class IMouseInputProvider;
}

namespace GAME
{
class CGameClient;
class CGameClientJoystick;

/*!
 * \ingroup games
 *
 * \brief Class to manage game-playing agents for a running game client
 *
 * Currently, port mapping is controller-based and does not take into account
 * the human belonging to the controller. In the future, humans and possibly
 * bots will be managed here.
 *
 * To map ports to controllers, a list of controllers is retrieved in
 * ProcessJoysticks(). After expired controllers are removed, the port mapping
 * occurs in the static function MapJoysticks(). The strategy is to simply
 * sort controllers by heuristics and greedily assign to game ports.
 */
class CAgentInput : public Observable,
                    public Observer,
                    KEYBOARD::IKeyboardDriverHandler,
                    MOUSE::IMouseDriverHandler
{
public:
  CAgentInput(PERIPHERALS::CPeripherals& peripheralManager, CInputManager& inputManager);

  virtual ~CAgentInput();

  // Lifecycle functions
  void Start(GameClientPtr gameClient);
  void Stop();
  void Refresh();

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

  // Implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override {}

  // Implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(MOUSE::BUTTON_ID button) override;
  void OnButtonRelease(MOUSE::BUTTON_ID button) override {}

  // Public interface
  std::vector<std::shared_ptr<const CAgentController>> GetControllers() const;
  std::string GetPortAddress(JOYSTICK::IInputProvider* inputProvider) const;
  std::string GetKeyboardAddress(KEYBOARD::IKeyboardInputProvider* inputProvider) const;
  std::string GetMouseAddress(MOUSE::IMouseInputProvider* inputProvider) const;
  std::vector<std::string> GetGameInputPorts() const;
  float GetGamePortActivation(const std::string& address) const;
  float GetPeripheralActivation(const std::string& peripheralLocation) const;

private:
  //! @todo De-duplicate these types
  using PortAddress = std::string;
  using JoystickMap = std::map<PortAddress, std::shared_ptr<CGameClientJoystick>>;
  using PortMap = std::map<JOYSTICK::IInputProvider*, std::shared_ptr<CGameClientJoystick>>;

  using PeripheralLocation = std::string;
  using CurrentPortMap = std::map<PortAddress, PeripheralLocation>;
  using CurrentPeripheralMap = std::map<PeripheralLocation, PortAddress>;

  using ControllerAddress = std::string;
  using PeripheralMap = std::map<ControllerAddress, PERIPHERALS::PeripheralPtr>;

  // Internal interface
  void ProcessJoysticks(PERIPHERALS::EventLockHandlePtr& inputHandlingLock);
  void ProcessKeyboard();
  void ProcessMouse();

  // Internal helpers
  void ProcessAgentControllers(const PERIPHERALS::PeripheralVector& joysticks,
                               PERIPHERALS::EventLockHandlePtr& inputHandlingLock);
  void UpdateExpiredJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                              PERIPHERALS::EventLockHandlePtr& inputHandlingLock);
  void UpdateConnectedJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                const PortMap& newPortMap,
                                PERIPHERALS::EventLockHandlePtr& inputHandlingLock,
                                std::set<PERIPHERALS::PeripheralPtr>& disconnectedJoysticks);

  // Static functionals
  static PortMap MapJoysticks(const PERIPHERALS::PeripheralVector& peripheralJoysticks,
                              const JoystickMap& gameClientjoysticks,
                              CurrentPortMap& currentPorts,
                              CurrentPeripheralMap& currentPeripherals,
                              int playerLimit);
  static void MapJoystick(PERIPHERALS::PeripheralPtr peripheralJoystick,
                          std::shared_ptr<CGameClientJoystick> gameClientJoystick,
                          PortMap& result);
  static void LogPeripheralMap(const PeripheralMap& peripheralMap,
                               const std::set<PERIPHERALS::PeripheralPtr>& disconnectedPeripherals);

  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;
  CInputManager& m_inputManager;

  // State parameters
  GameClientPtr m_gameClient;
  bool m_bHasKeyboard = false;
  bool m_bHasMouse = false;
  int m_initialMouseX{-1};
  int m_initialMouseY{-1};
  std::vector<std::shared_ptr<CAgentController>> m_controllers;

  // Synchronization parameters
  mutable std::mutex m_controllerMutex;

  /*!
   * \brief Map of joystick input provider to joystick handler
   *
   * The input provider is a handle to agent input.
   *
   * The joystick handler connects to joystick input of the game client.
   *
   * This property remembers which joysticks are actually being controlled by
   * agents.
   *
   * Not exposed to the game.
   */
  PortMap m_portMap;

  /*!
   * \brief Map of keyboard input provider to keyboard port address
   */
  std::map<KEYBOARD::IKeyboardInputProvider*, PortAddress> m_keyboardPort;

  /*!
   * \brief Map of mouse input provider to mouse port address
   */
  std::map<MOUSE::IMouseInputProvider*, PortAddress> m_mousePort;

  /*!
   * \brief Map of the current ports to their peripheral
   *
   * This allows attempt to preserve player numbers.
   */
  CurrentPortMap m_currentPorts;

  /*!
   * \brief Map of the current peripherals to their port
   *
   * This allows attempt to preserve player numbers.
   */
  CurrentPeripheralMap m_currentPeripherals;

  /*!
   * Map of controller address to source peripheral
   *
   * Source peripherals are not exposed to the game.
   */
  PeripheralMap m_peripheralMap;

  /*!
   * Collection of disconnected joysticks
   *
   * Source peripherals are not exposed to the game.
   */
  std::set<PERIPHERALS::PeripheralPtr> m_disconnectedPeripherals;
};
} // namespace GAME
} // namespace KODI
