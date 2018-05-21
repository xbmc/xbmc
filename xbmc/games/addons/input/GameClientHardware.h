/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "input/hardware/IHardwareInput.h"

namespace KODI
{
namespace GAME
{
  class CGameClient;

  /*!
   * \ingroup games
   * \brief Handles events for hardware such as reset buttons
   */
  class CGameClientHardware : public HARDWARE::IHardwareInput
  {
  public:
    /*!
     * \brief Constructor
     *
     * \param gameClient The game client implementation
     */
    explicit CGameClientHardware(CGameClient &gameClient);

    ~CGameClientHardware() override = default;

    // Implementation of IHardwareInput
    void OnResetButton() override;

  private:
    // Construction parameter
    CGameClient &m_gameClient;
  };
}
}
