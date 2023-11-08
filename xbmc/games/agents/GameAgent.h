/*
 *  Copyright (C) 2017-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "XBDateTime.h"
#include "games/controllers/ControllerTypes.h"
#include "peripherals/PeripheralTypes.h"

#include <memory>
#include <string>

namespace KODI
{
namespace GAME
{
class CGameAgentJoystick;

/*!
 * \ingroup games
 *
 * \brief Class to represent a game player (a.k.a. agent)
 *
 * The term "agent" is used to distinguish game players from the myriad of other
 * uses of the term "player" in Kodi, such as media players and player cores.
 */
class CGameAgent
{
public:
  CGameAgent(PERIPHERALS::PeripheralPtr peripheral);
  ~CGameAgent();

  // Lifecycle functions
  void Initialize();
  void Deinitialize();

  // Input properties
  PERIPHERALS::PeripheralPtr GetPeripheral() const { return m_peripheral; }
  std::string GetPeripheralName() const;
  const std::string& GetPeripheralLocation() const;
  ControllerPtr GetController() const;
  CDateTime LastActive() const;
  float GetActivation() const;

private:
  // Construction parameters
  const PERIPHERALS::PeripheralPtr m_peripheral;

  // Input parameters
  std::unique_ptr<CGameAgentJoystick> m_joystick;
};

} // namespace GAME
} // namespace KODI
