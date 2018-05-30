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
