/*
 *  Copyright (C) 2017-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "games/GameTypes.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <string>

class CInputManager;

namespace PERIPHERALS
{
class CPeripheral;
class CPeripherals;
} // namespace PERIPHERALS

namespace KODI
{
namespace JOYSTICK
{
class IInputProvider;
}

namespace GAME
{
class CGameAgent;
class CGameClient;
class CGameClientJoystick;

/*!
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
class CGameAgentManager : public Observable,
                          public Observer,
                          KEYBOARD::IKeyboardDriverHandler,
                          MOUSE::IMouseDriverHandler
{
public:
  CGameAgentManager(PERIPHERALS::CPeripherals& peripheralManager, CInputManager& inputManager);

  virtual ~CGameAgentManager();

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

private:
  //! @todo De-duplicate these types
  using PortAddress = std::string;
  using JoystickMap = std::map<PortAddress, std::shared_ptr<CGameClientJoystick>>;
  using PortMap = std::map<JOYSTICK::IInputProvider*, std::shared_ptr<CGameClientJoystick>>;

  // Internal interface
  void ProcessJoysticks(PERIPHERALS::EventLockHandlePtr& inputHandlingLock);
  void ProcessKeyboard();
  void ProcessMouse();

  // Internal helpers
  void UpdateExpiredJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                              PERIPHERALS::EventLockHandlePtr& inputHandlingLock);
  void UpdateConnectedJoysticks(const PERIPHERALS::PeripheralVector& joysticks,
                                const PortMap& newPortMap,
                                PERIPHERALS::EventLockHandlePtr& inputHandlingLock);

  // Static functionals
  static PortMap MapJoysticks(const PERIPHERALS::PeripheralVector& peripheralJoysticks,
                              const JoystickMap& gameClientjoysticks,
                              int playerLimit);

  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;
  CInputManager& m_inputManager;

  // State parameters
  GameClientPtr m_gameClient;
  bool m_bHasKeyboard = false;
  bool m_bHasMouse = false;

  /*!
   * \brief Map of input provider to joystick handler
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
};
} // namespace GAME
} // namespace KODI
