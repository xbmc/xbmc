/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"

#include <string>

struct game_input_device;
struct game_input_port;

namespace KODI
{
namespace GAME
{
class CPhysicalPort;

/*!
 * \ingroup games
 *
 * \brief Represents a port that devices can connect to
 */
class CGameClientPort
{
public:
  /*!
   * \brief Construct a hardware port
   *
   * \param port The hardware port Game API struct
   */
  CGameClientPort(const game_input_port& port);

  /*!
   * \brief Construct a hardware port that accepts the given controllers
   *
   * \param controllers List of accepted controller profiles
   *
   * The port is given the ID specified by DEFAULT_PORT_ID.
   */
  CGameClientPort(const ControllerVector& controllers);

  /*!
   * \brief Construct a controller port
   *
   * \param logicalPort The logical port Game API struct
   * \param physicalPort The physical port definition
   *
   * The physical port is defined by the controller profile. This definition
   * specifies which controllers the port is physically compatible with.
   *
   * The logical port is defined by the emulator's input topology. This
   * definition specifies which controllers the emulator's logic can handle.
   *
   * Obviously, the controllers specified by the logical port must be a subset
   * of the controllers supported by the physical port.
   */
  CGameClientPort(const game_input_port& logicalPort, const CPhysicalPort& physicalPort);

  /*!
   * \brief Destructor
   */
  ~CGameClientPort();

  /*!
   * \brief Get the port type
   *
   * The port type identifies if this port is for a keyboard, mouse, or
   * controller.
   */
  PORT_TYPE PortType() const { return m_type; }

  /*!
   * \brief Get the ID of the port
   *
   * The ID is used when creating a toplogical address for the port.
   */
  const std::string& ID() const { return m_portId; }

  /*!
   * \brief True if a controller must be connected, preventing the disconnected
   * option from being shown to the user
   */
  bool ForceConnected() const { return m_forceConnected; }

  /*!
   * \brief Get the list of devices accepted by this port
   */
  const GameClientDeviceVec& Devices() const { return m_acceptedDevices; }

private:
  PORT_TYPE m_type;
  std::string m_portId;
  bool m_forceConnected{false};
  GameClientDeviceVec m_acceptedDevices;
};
} // namespace GAME
} // namespace KODI
