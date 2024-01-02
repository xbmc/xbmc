/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PortNode.h"

#include "games/controllers/Controller.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/ports/input/PhysicalPort.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace GAME;

CPortNode::~CPortNode() = default;

CPortNode& CPortNode::operator=(const CPortNode& rhs)
{
  if (this != &rhs)
  {
    m_bConnected = rhs.m_bConnected;
    m_active = rhs.m_active;
    m_portType = rhs.m_portType;
    m_portId = rhs.m_portId;
    m_address = rhs.m_address;
    m_forceConnected = rhs.m_forceConnected;
    m_controllers = rhs.m_controllers;
  }

  return *this;
}

CPortNode& CPortNode::operator=(CPortNode&& rhs) noexcept
{
  if (this != &rhs)
  {
    m_bConnected = rhs.m_bConnected;
    m_active = rhs.m_active;
    m_portType = rhs.m_portType;
    m_portId = std::move(rhs.m_portId);
    m_address = std::move(rhs.m_address);
    m_forceConnected = rhs.m_forceConnected;
    m_controllers = std::move(rhs.m_controllers);
  }

  return *this;
}

const CControllerNode& CPortNode::GetActiveController() const
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static const CControllerNode invalid{};
  return invalid;
}

CControllerNode& CPortNode::GetActiveController()
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static CControllerNode invalid;
  invalid.Clear();
  return invalid;
}

bool CPortNode::SetActiveController(const std::string& controllerId)
{
  for (size_t i = 0; i < m_controllers.size(); ++i)
  {
    const ControllerPtr& controller = m_controllers.at(i).GetController();
    if (controller && controller->ID() == controllerId)
    {
      m_active = i;
      return true;
    }
  }

  return false;
}

void CPortNode::SetPortID(std::string portId)
{
  m_portId = std::move(portId);
}

void CPortNode::SetAddress(std::string address)
{
  m_address = std::move(address);
}

void CPortNode::SetCompatibleControllers(ControllerNodeVec controllers)
{
  m_controllers = std::move(controllers);
}

bool CPortNode::IsControllerAccepted(const std::string& controllerId) const
{
  // Base case
  CPhysicalPort port;
  GetPort(port);
  if (port.IsCompatible(controllerId))
    return true;

  // Visit nodes
  return std::any_of(m_controllers.begin(), m_controllers.end(),
                     [controllerId](const CControllerNode& node)
                     { return node.IsControllerAccepted(controllerId); });
}

bool CPortNode::IsControllerAccepted(const std::string& portAddress,
                                     const std::string& controllerId) const
{
  bool bAccepted = false;

  if (m_address == portAddress)
  {
    // Base case
    CPhysicalPort port;
    GetPort(port);
    if (port.IsCompatible(controllerId))
      bAccepted = true;
  }
  else
  {
    // Visit nodes
    if (std::any_of(m_controllers.begin(), m_controllers.end(),
                    [portAddress, controllerId](const CControllerNode& node)
                    { return node.IsControllerAccepted(portAddress, controllerId); }))
    {
      bAccepted = true;
    }
  }

  return bAccepted;
}

void CPortNode::GetInputPorts(std::vector<std::string>& inputPorts) const
{
  if (IsConnected())
  {
    const CControllerNode& controller = GetActiveController();
    controller.GetInputPorts(inputPorts);
  }
}

void CPortNode::GetPort(CPhysicalPort& port) const
{
  std::vector<std::string> accepts;
  for (const CControllerNode& node : m_controllers)
  {
    if (node.GetController())
      accepts.emplace_back(node.GetController()->ID());
  }

  port = CPhysicalPort(m_portId, std::move(accepts));
}
