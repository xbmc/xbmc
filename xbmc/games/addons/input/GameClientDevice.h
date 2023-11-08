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
  CGameClientDevice(const game_input_device& device);

  /*!
   * \brief Construct a device from a controller add-on
   *
   * \param controller The controller add-on
   */
  CGameClientDevice(const ControllerPtr& controller);

  /*!
   * \brief Destructor
   */
  ~CGameClientDevice();

  /*!
   * \brief The controller profile
   */
  const ControllerPtr& Controller() const { return m_controller; }

  /*!
   * \brief The ports on this device
   */
  const GameClientPortVec& Ports() const { return m_ports; }

private:
  /*!
   * \brief Add a controller port
   *
   * \param logicalPort The logical port Game API struct
   * \param physicalPort The physical port definition
   */
  void AddPort(const game_input_port& logicalPort, const CPhysicalPort& physicalPort);

  // Helper function
  static ControllerPtr GetController(const char* controllerId);

  ControllerPtr m_controller;
  GameClientPortVec m_ports;
};
} // namespace GAME
} // namespace KODI
