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
    CRetroPlayerInput(PERIPHERALS::CPeripherals &peripheralManager);
    ~CRetroPlayerInput() override;

    void SetSpeed(double speed);

    // implementation of IGameAudioCallback
    void PollInput() override;

  private:
    // Construction parameters
    PERIPHERALS::CPeripherals &m_peripheralManager;

    // Input variables
    PERIPHERALS::EventPollHandlePtr m_inputPollHandle;
  };
}
}
