/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <map>
#include <memory>

class Observer;

namespace PERIPHERALS
{
class CPeripherals;
}

namespace KODI
{
namespace GAME
{
class CGameServices;
}

namespace SMART_HOME
{
class CSmartHomeJoystick;
class ISmartHomeJoystickHandler;

class CSmartHomeInputManager
{
public:
  CSmartHomeInputManager(PERIPHERALS::CPeripherals& peripheralManager);
  ~CSmartHomeInputManager();

  void Initialize(GAME::CGameServices& gameServices);
  void Deinitialize();

  void RegisterPeripheralObserver(Observer* obs);
  void UnregisterPeripheralObserver(Observer* obs);

  PERIPHERALS::PeripheralVector GetPeripherals() const;

  void PrunePeripherals();

  bool OpenJoystick(const std::string& peripheralLocation,
                    const std::string& controllerProfile,
                    ISmartHomeJoystickHandler& joystickHandler);
  void CloseJoystick(const std::string& peripheralLocation);

private:
  using PeripheralAddress = std::string;
  using JoystickMap = std::map<PeripheralAddress, std::unique_ptr<CSmartHomeJoystick>>;

  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;

  // Initialization parameters
  GAME::CGameServices* m_gameServices{nullptr};

  // Input parameters
  JoystickMap m_joysticks;
};

} // namespace SMART_HOME
} // namespace KODI
