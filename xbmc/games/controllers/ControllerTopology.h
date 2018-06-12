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

#include "games/GameTypes.h"

#include <string>
#include <vector>

class TiXmlElement;

namespace KODI
{
namespace GAME
{

class CControllerPort
{
public:
  CControllerPort() = default;

  /*!
   * \brief Create a controller port
   *
   * \param portId The port's ID
   * \param accepts A list of controller IDs that this port accepts
   */
  CControllerPort(std::string portId, std::vector<std::string> accepts);

  void Reset();

  /*!
   * \brief Get the ID of the port
   *
   * \return The port's ID, e.g. "1", as a string
   */
  const std::string &ID() const { return m_portId; }

  /*!
   * \brief Get the controllers that can connect to this port
   *
   * \return A list of controllers that are physically compatible with this port
   */
  const std::vector<std::string> &Accepts() const { return m_accepts; }

  /*!
   * \brief Check if the controller is compatible with this port
   *
   * \return True if the controller is accepted, false otherwise
   */
  bool IsCompatible(const std::string &controllerId) const;

  bool Deserialize(const TiXmlElement* pElement);

private:
  std::string m_portId;
  std::vector<std::string> m_accepts;
};

/*!
 * \brief Represents the physical topology of controller add-ons
 *
 * The physical topology of a controller defines how many ports it has and
 * whether it can provide player input (hubs like the Super Multitap don't
 * provide input).
 */
class CControllerTopology
{
public:
  CControllerTopology() = default;
  CControllerTopology(bool bProvidesInput, std::vector<CControllerPort> ports);

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
  const std::vector<CControllerPort> &Ports() const { return m_ports; }

  bool Deserialize(const TiXmlElement* pElement);

private:
  bool m_bProvidesInput = true;
  std::vector<CControllerPort> m_ports;
};

}
}
