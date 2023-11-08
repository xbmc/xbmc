/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "games/controllers/types/ControllerNode.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CPhysicalPort;

/*!
 * \ingroup games
 *
 * \brief Collection of nodes that can be connected to this port
 */
class CPortNode
{
public:
  CPortNode() = default;
  CPortNode(const CPortNode& other) { *this = other; }
  CPortNode(CPortNode&& other) = default;
  CPortNode& operator=(const CPortNode& rhs);
  CPortNode& operator=(CPortNode&& rhs) noexcept;
  ~CPortNode();

  /*!
   * \brief Connection state of the port
   *
   * \return True if a controller is connected, false otherwise
   */
  bool IsConnected() const { return m_bConnected; }
  void SetConnected(bool bConnected) { m_bConnected = bConnected; }

  /*!
   * \brief The controller that is active on this port
   *
   * \return The active controller, or invalid if port is disconnected
   */
  const CControllerNode& GetActiveController() const;
  CControllerNode& GetActiveController();
  void SetActiveController(unsigned int controllerIndex) { m_active = controllerIndex; }
  bool SetActiveController(const std::string& controllerId);

  /*!
   * \brief The port type
   *
   * \return The port type, if known
   */
  PORT_TYPE GetPortType() const { return m_portType; }
  void SetPortType(PORT_TYPE type) { m_portType = type; }

  /*!
   * \brief The hardware or controller port ID
   *
   * \return The port ID of the hardware port or controller port, or empty if
   *         the port is only identified by its type
   */
  const std::string& GetPortID() const { return m_portId; }
  void SetPortID(std::string portId);

  /*!
   * \brief Address given to the node by the implementation
   */
  const std::string& GetAddress() const { return m_address; }
  void SetAddress(std::string address);

  /*!
   * \brief If true, prevents a disconnection option from being shown for this
   * port
   */
  bool IsForceConnected() const { return m_forceConnected; }
  void SetForceConnected(bool forceConnected) { m_forceConnected = forceConnected; }

  /*!
   * \brief Return the controller profiles that are compatible with this port
   *
   * \return The controller profiles, or empty if this port doesn't support
   *         any controller profiles
   */
  const ControllerNodeVec& GetCompatibleControllers() const { return m_controllers; }
  ControllerNodeVec& GetCompatibleControllers() { return m_controllers; }
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

  /*!
   * \brief Get a list of ports that accept player input
   *
   * \param[out] inputPorts The list of input ports
   */
  void GetInputPorts(std::vector<std::string>& inputPorts) const;

private:
  void GetPort(CPhysicalPort& port) const;

  bool m_bConnected = false;
  unsigned int m_active = 0;
  PORT_TYPE m_portType = PORT_TYPE::UNKNOWN;
  std::string m_portId;
  std::string m_address;
  bool m_forceConnected{true};
  ControllerNodeVec m_controllers;
};

/*!
 * \brief Collection of port nodes
 */
using PortVec = std::vector<CPortNode>;
} // namespace GAME
} // namespace KODI
