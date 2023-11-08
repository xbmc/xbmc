/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/ports/input/PhysicalPort.h"

#include <vector>

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 *
 * \brief Represents the physical topology of controller add-ons
 *
 * The physical topology of a controller defines how many ports it has and
 * whether it can provide player input (hubs like the Super Multitap don't
 * provide input).
 */
class CPhysicalTopology
{
public:
  CPhysicalTopology() = default;
  CPhysicalTopology(bool bProvidesInput, std::vector<CPhysicalPort> ports);

  void Reset();

  /*!
   * \brief Check if the controller can provide player input
   *
   * This allows hubs to specify that they provide no input
   *
   * \return True if the controller can provide player input, false otherwise
   */
  bool ProvidesInput() const { return m_bProvidesInput; }

  /*!
   * \brief Get a list of ports provided by this controller
   *
   * \return The ports
   */
  const std::vector<CPhysicalPort>& Ports() const { return m_ports; }

  bool Deserialize(const tinyxml2::XMLElement* pElement);

private:
  bool m_bProvidesInput = true;
  std::vector<CPhysicalPort> m_ports;
};

} // namespace GAME
} // namespace KODI
