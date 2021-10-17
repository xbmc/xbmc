/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ControllerNode.h"
#include "games/controllers/ControllerTypes.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CControllerPort;

/*!
 * \brief Collection of nodes that can be connected to this port
 */
class CPortNode
{
public:
  CPortNode() = default;
  CPortNode(const CPortNode& other) { *this = other; }
  CPortNode(CPortNode&& other) = default;
  CPortNode& operator=(const CPortNode& rhs);
  CPortNode& operator=(CPortNode&& rhs);
  ~CPortNode();

  /*!
   * \brief Connection state of the port
   *
   * \return True if a controller is connected, false otherwise
   */
  bool Connected() const { return m_bConnected; }
  void SetConnected(bool bConnected) { m_bConnected = bConnected; }

  /*!
   * \brief The controller that is active on this port
   *
   * \return The active controller, or invalid if port is disconnected
   */
  const CControllerNode& ActiveController() const;
  CControllerNode& ActiveController();
  void SetActiveController(unsigned int controllerIndex) { m_active = controllerIndex; }

  /*!
   * \brief The port type
   *
   * \return The port type, if known
   */
  PORT_TYPE PortType() const { return m_portType; }
  void SetPortType(PORT_TYPE type) { m_portType = type; }

  /*!
   * \brief The hardware or controller port ID
   *
   * \return The port ID of the hardware port or controller port, or empty if
   *         the port is only identified by its type
   */
  const std::string& PortID() const { return m_portId; }
  void SetPortID(std::string portId);

  /*!
   * \brief Address given to the node by the implementation
   */
  const std::string& Address() const { return m_address; }
  void SetAddress(std::string address);

  /*!
   * \brief Return the controller profiles that are compatible with this port
   *
   * \return The controller profiles, or empty if this port doesn't support
   *         any controller profiles
   */
  const ControllerNodeVec& CompatibleControllers() const { return m_controllers; }
  void SetCompatibleControllers(ControllerNodeVec controllers);

  /*!
   * \brief Check to see if a controller is compatible with this tree
   *
   * \param controllerId The ID of the controller
   *
   * \return True if the controller is compatible with the tree, false otherwise
   */
  bool IsControllerAccepted(const std::string& controllerId) const;

  /*!
   * \brief Check to see if a controller is compatible with this tree
   *
   * \param portAddress The port address
   * \param controllerId The ID of the controller
   *
   * \return True if the controller is compatible with the tree, false otherwise
   */
  bool IsControllerAccepted(const std::string& portAddress, const std::string& controllerId) const;

private:
  void GetPort(CControllerPort& port) const;

  bool m_bConnected = false;
  unsigned int m_active = 0;
  PORT_TYPE m_portType = PORT_TYPE::UNKNOWN;
  std::string m_portId;
  std::string m_address;
  ControllerNodeVec m_controllers;
};

/*!
 * \brief Collection of port nodes
 */
using PortVec = std::vector<CPortNode>;
} // namespace GAME
} // namespace KODI
