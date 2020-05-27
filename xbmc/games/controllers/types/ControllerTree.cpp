/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerTree.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTopology.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace GAME;

// --- CControllerNode ---------------------------------------------------------

CControllerNode::CControllerNode() : m_hub(new CControllerHub)
{
}

CControllerNode::~CControllerNode() = default;

CControllerNode& CControllerNode::operator=(const CControllerNode& rhs)
{
  if (this != &rhs)
  {
    m_controller = rhs.m_controller;
    m_address = rhs.m_address;
    m_hub.reset(new CControllerHub(*rhs.m_hub));
  }

  return *this;
}

void CControllerNode::Clear()
{
  m_controller.reset();
  m_address.clear();
  m_hub.reset(new CControllerHub);
}

void CControllerNode::SetController(ControllerPtr controller)
{
  m_controller = std::move(controller);
}

void CControllerNode::GetControllers(ControllerVector& controllers) const
{
  const ControllerPtr& myController = m_controller;

  auto it = std::find_if(controllers.begin(), controllers.end(),
                         [&myController](const ControllerPtr& controller) {
                           return myController->ID() == controller->ID();
                         });

  if (it == controllers.end())
    controllers.emplace_back(m_controller);

  m_hub->GetControllers(controllers);
}

void CControllerNode::SetAddress(std::string address)
{
  m_address = std::move(address);
}

void CControllerNode::SetHub(CControllerHub hub)
{
  m_hub.reset(new CControllerHub(std::move(hub)));
}

bool CControllerNode::IsControllerAccepted(const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const auto& port : m_hub->Ports())
  {
    if (port.IsControllerAccepted(controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

bool CControllerNode::IsControllerAccepted(const std::string& portAddress,
                                           const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const auto& port : m_hub->Ports())
  {
    if (port.IsControllerAccepted(portAddress, controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

bool CControllerNode::ProvidesInput() const
{
  return m_controller && m_controller->Topology().ProvidesInput();
}

// --- CControllerPortNode -----------------------------------------------------

CControllerPortNode::~CControllerPortNode() = default;

CControllerPortNode& CControllerPortNode::operator=(const CControllerPortNode& rhs)
{
  if (this != &rhs)
  {
    m_bConnected = rhs.m_bConnected;
    m_portType = rhs.m_portType;
    m_portId = rhs.m_portId;
    m_controllers = rhs.m_controllers;
    m_address = rhs.m_address;
    m_active = rhs.m_active;
  }

  return *this;
}

const CControllerNode& CControllerPortNode::ActiveController() const
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static const CControllerNode invalid{};
  return invalid;
}

CControllerNode& CControllerPortNode::ActiveController()
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static CControllerNode invalid;
  invalid.Clear();
  return invalid;
}

void CControllerPortNode::SetPortID(std::string portId)
{
  m_portId = std::move(portId);
}

void CControllerPortNode::SetAddress(std::string address)
{
  m_address = std::move(address);
}

void CControllerPortNode::SetCompatibleControllers(ControllerNodeVec controllers)
{
  m_controllers = std::move(controllers);
}

bool CControllerPortNode::IsControllerAccepted(const std::string& controllerId) const
{
  // Base case
  CControllerPort port;
  GetControllerPort(port);
  if (port.IsCompatible(controllerId))
    return true;

  // Visit nodes
  for (const auto& node : m_controllers)
  {
    if (node.IsControllerAccepted(controllerId))
      return true;
  }

  return false;
}

bool CControllerPortNode::IsControllerAccepted(const std::string& portAddress,
                                               const std::string& controllerId) const
{
  bool bAccepted = false;

  if (m_address == portAddress)
  {
    // Base case
    CControllerPort port;
    GetControllerPort(port);
    if (port.IsCompatible(controllerId))
      bAccepted = true;
  }
  else
  {
    // Visit nodes
    for (const auto& node : m_controllers)
    {
      if (node.IsControllerAccepted(portAddress, controllerId))
      {
        bAccepted = true;
        break;
      }
    }
  }

  return bAccepted;
}

void CControllerPortNode::GetControllerPort(CControllerPort& port) const
{
  std::vector<std::string> accepts;
  for (const CControllerNode& node : m_controllers)
    accepts.emplace_back(node.Controller()->ID());

  port = CControllerPort(m_portId, std::move(accepts));
}

// --- CControllerHub ----------------------------------------------------------

CControllerHub::~CControllerHub() = default;

CControllerHub& CControllerHub::operator=(const CControllerHub& rhs)
{
  if (this != &rhs)
  {
    m_ports = rhs.m_ports;
  }

  return *this;
}

void CControllerHub::Clear()
{
  m_ports.clear();
}

void CControllerHub::SetPorts(ControllerPortVec ports)
{
  m_ports = std::move(ports);
}

bool CControllerHub::IsControllerAccepted(const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const CControllerPortNode& port : m_ports)
  {
    if (port.IsControllerAccepted(controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

bool CControllerHub::IsControllerAccepted(const std::string& portAddress,
                                          const std::string& controllerId) const
{
  bool bAccepted = false;

  for (const CControllerPortNode& port : m_ports)
  {
    if (port.IsControllerAccepted(portAddress, controllerId))
    {
      bAccepted = true;
      break;
    }
  }

  return bAccepted;
}

ControllerVector CControllerHub::GetControllers() const
{
  ControllerVector controllers;
  GetControllers(controllers);
  return controllers;
}

void CControllerHub::GetControllers(ControllerVector& controllers) const
{
  for (const CControllerPortNode& port : m_ports)
  {
    for (const CControllerNode& node : port.CompatibleControllers())
      node.GetControllers(controllers);
  }
}

const CControllerPortNode& CControllerHub::GetPort(const std::string& address) const
{
  return GetPort(m_ports, address);
}

const CControllerPortNode& CControllerHub::GetPort(const ControllerPortVec& ports,
                                                   const std::string& address)
{
  for (const CControllerPortNode& port : ports)
  {
    if (port.Address() == address)
      return port;

    for (const CControllerNode& controller : port.CompatibleControllers())
    {
      for (const CControllerPortNode& controllerPort : controller.Hub().Ports())
      {
        if (port.Address() == address)
          return controllerPort;
      }
    }
  }

  static const CControllerPortNode empty{};
  return empty;
}
