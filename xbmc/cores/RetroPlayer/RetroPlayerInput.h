/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
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
class CRPProcessInfo;

class CRetroPlayerInput : public GAME::IGameInputCallback
{
public:
  CRetroPlayerInput(PERIPHERALS::CPeripherals& peripheralManager,
                    CRPProcessInfo& processInfo,
                    GAME::GameClientPtr gameClient);
  ~CRetroPlayerInput() override;

  // Lifecycle functions
  void StartAgentManager();
  void StopAgentManager();

  // Input functions
  void SetSpeed(double speed);
  void EnableInput(bool bEnabled);

  // implementation of IGameInputCallback
  bool AcceptsInput() const override { return m_bEnabled; }
  void PollInput() override;

private:
  // Construction parameters
  PERIPHERALS::CPeripherals& m_peripheralManager;
  CRPProcessInfo& m_processInfo;

  // Input variables
  PERIPHERALS::EventPollHandlePtr m_inputPollHandle;
  bool m_bEnabled = false;

  // Game parameters
  const GAME::GameClientPtr m_gameClient;
  bool m_bAgentManagerStarted{false};
};
} // namespace RETRO
} // namespace KODI
