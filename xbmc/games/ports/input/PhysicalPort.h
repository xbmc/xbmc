/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
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
 */
class CPhysicalPort
{
public:
  CPhysicalPort() = default;

  /*!
   * \brief Create a controller port
   *
   * \param portId The port's ID
   * \param accepts A list of controller IDs that this port accepts
   */
  CPhysicalPort(std::string portId, std::vector<std::string> accepts);

  void Reset();

  /*!
   * \brief Get the ID of the port
   *
   * \return The port's ID, e.g. "1", as a string
   */
  const std::string& ID() const { return m_portId; }

  /*!
   * \brief Get the controllers that can connect to this port
   *
   * \return A list of controllers that are physically compatible with this port
   */
  const std::vector<std::string>& Accepts() const { return m_accepts; }

  /*!
   * \brief Check if the controller is compatible with this port
   *
   * \return True if the controller is accepted, false otherwise
   */
  bool IsCompatible(const std::string& controllerId) const;

  bool Deserialize(const tinyxml2::XMLElement* pElement);

private:
  std::string m_portId;
  std::vector<std::string> m_accepts;
};
} // namespace GAME
} // namespace KODI
