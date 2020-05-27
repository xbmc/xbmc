/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientCallbacks.h"
#include "peripherals/PeripheralTypes.h"

namespace PERIPHERALS
{
class CPeripherals;
}

namespace KODI
{
namespace RETRO
{
class CRetroPlayerInput : public GAME::IGameInputCallback
{
public:
  CRetroPlayerInput(PERIPHERALS::CPeripherals& peripheralManager);
  ~CRetroPlayerInput() override;

  void SetSpeed(double speed);
  void EnableInput(bool bEnabled);

  // implementation of IGameInputCallback
  bool AcceptsInput() const override { return m_bEnabled; }
  void PollInput() override;

private:
  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;

  // Input variables
  PERIPHERALS::EventPollHandlePtr m_inputPollHandle;
  bool m_bEnabled = false;
};
} // namespace RETRO
} // namespace KODI
