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

#include "games/controllers/ControllerTypes.h"
#include "games/GameTypes.h"

#include <string>

struct game_input_device;
struct game_input_port;

namespace KODI
{
namespace GAME
{
  class CControllerPort;

  /*!
   * \ingroup games
   * \brief Represents a device connected to a port
   */
  class CGameClientDevice
  {
  public:
    /*!
     * \brief Construct a device
     *
     * \param device The device Game API struct
     */
    CGameClientDevice(const game_input_device &device);

    /*!
     * \brief Construct a device from a controller add-on
     *
     * \param controller The controller add-on
     */
    CGameClientDevice(const ControllerPtr &controller);

    /*!
     * \brief Destructor
     */
    ~CGameClientDevice();

    /*!
     * \brief The controller profile
     */
    const ControllerPtr &Controller() const { return m_controller; }

    /*!
     * \brief The ports on this device
     */
    const GameClientPortVec &Ports() const { return m_ports; }

  private:
    /*!
     * \brief Add a controller port
     *
     * \param logicalPort The logical port Game API struct
     * \param physicalPort The physical port definition
     */
    void AddPort(const game_input_port &logicalPort, const CControllerPort &physicalPort);

    // Helper function
    static ControllerPtr GetController(const char *controllerId);

    ControllerPtr m_controller;
    GameClientPortVec m_ports;
  };
}
}
