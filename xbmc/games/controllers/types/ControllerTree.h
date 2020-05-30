/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CControllerHub;
class CControllerPort;

/*!
 * \brief Node in the controller tree
 *
 * The node identifies the controller profile, and optionally the available
 * controller ports.
 */
class CControllerNode
{
public:
  CControllerNode();
  CControllerNode(const CControllerNode& other) { *this = other; }
  CControllerNode(CControllerNode&& other) = default;
  CControllerNode& operator=(const CControllerNode& rhs);
  ~CControllerNode();

  void Clear();

  /*!
   * \brief Controller profile of this code
   *
   * \return Controller profile, or empty if this node is invalid
   *
   * \sa IsValid()
   */
  const ControllerPtr& Controller() const { return m_controller; }
  void SetController(ControllerPtr controller);

  void GetControllers(ControllerVector& controllers) const;

  /*!
   * \brief Address given to the node by the implementation
   */
  const std::string& Address() const { return m_address; }
  void SetAddress(std::string address);

  /*!
   * \brief Collection of ports on this controller
   *
   * \return A hub with controller ports, or an empty hub if this controller
   *         has no available ports
   */
  const CControllerHub& Hub() const { return *m_hub; }
  CControllerHub& Hub() { return *m_hub; }
  void SetHub(CControllerHub hub);

  /*!
   * \brief Check if this node has a valid controller profile
   */
  bool IsValid() const { return m_controller.get() != nullptr; }

  /*!
   * \brief Check to see if a controller is compatible with a controller port
   *
   * \param controllerId The ID of the controller
   *
   * \return True if the controller is compatible with a port, false otherwise
   */
  bool IsControllerAccepted(const std::string& controllerId) const;

  /*!
   * \brief Check to see if a controller is compatible with a controller port
   *
   * \param portAddress The port address
   * \param controllerId The ID of the controller
   *
   * \return True if the controller is compatible with a port, false otherwise
   */
  bool IsControllerAccepted(const std::string& portAddress, const std::string& controllerId) const;

  /*!
   * \brief Check if this node provides input
   */
  bool ProvidesInput() const;

private:
  ControllerPtr m_controller;
  std::string m_address;
  std::unique_ptr<CControllerHub> m_hub;
};

using ControllerNodeVec = std::vector<CControllerNode>;

/*!
 * \brief Collection of nodes that can be connected to this port
 */
class CControllerPortNode
{
public:
  CControllerPortNode() = default;
  CControllerPortNode(const CControllerPortNode& other) { *this = other; }
  CControllerPortNode(CControllerPortNode&& other) = default;
  CControllerPortNode& operator=(const CControllerPortNode& rhs);
  ~CControllerPortNode();

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
  void GetControllerPort(CControllerPort& port) const;

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
using ControllerPortVec = std::vector<CControllerPortNode>;

/*!
 * \brief A branch in the controller tree
 */
class CControllerHub
{
public:
  CControllerHub() = default;
  CControllerHub(const CControllerHub& other) { *this = other; }
  CControllerHub(CControllerHub&& other) = default;
  CControllerHub& operator=(const CControllerHub& rhs);
  ~CControllerHub();

  void Clear();

  bool HasPorts() const { return !m_ports.empty(); }
  ControllerPortVec& Ports() { return m_ports; }
  const ControllerPortVec& Ports() const { return m_ports; }
  void SetPorts(ControllerPortVec ports);

  bool IsControllerAccepted(const std::string& controllerId) const;
  bool IsControllerAccepted(const std::string& portAddress, const std::string& controllerId) const;
  ControllerVector GetControllers() const;
  void GetControllers(ControllerVector& controllers) const;

  const CControllerPortNode& GetPort(const std::string& address) const;

private:
  static const CControllerPortNode& GetPort(const ControllerPortVec& ports,
                                            const std::string& address);

  ControllerPortVec m_ports;
};

/*!
 * \brief Collection of ports on a console
 */
using CControllerTree = CControllerHub;
} // namespace GAME
} // namespace KODI
