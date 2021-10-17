/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PortNode.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerPort.h"
#include "games/controllers/types/ControllerHub.h"

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
    m_portType = rhs.m_portType;
    m_portId = rhs.m_portId;
    m_controllers = rhs.m_controllers;
    m_address = rhs.m_address;
    m_active = rhs.m_active;
  }

  return *this;
}

CPortNode& CPortNode::operator=(CPortNode&& rhs)
{
  if (this != &rhs)
  {
    m_bConnected = rhs.m_bConnected;
    m_active = rhs.m_active;
    m_portType = rhs.m_portType;
    m_portId = std::move(rhs.m_portId);
    m_address = std::move(rhs.m_address);
    m_controllers = std::move(rhs.m_controllers);
  }

  return *this;
}

const CControllerNode& CPortNode::ActiveController() const
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static const CControllerNode invalid{};
  return invalid;
}

CControllerNode& CPortNode::ActiveController()
{
  if (m_bConnected && m_active < m_controllers.size())
    return m_controllers[m_active];

  static CControllerNode invalid;
  invalid.Clear();
  return invalid;
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
  CControllerPort port;
  GetPort(port);
  if (port.IsCompatible(controllerId))
    return true;

  // Visit nodes
  return std::any_of(m_controllers.begin(), m_controllers.end(),
                     [controllerId](const CControllerNode& node) {
                       return node.IsControllerAccepted(controllerId);
                     });
}

bool CPortNode::IsControllerAccepted(const std::string& portAddress,
                                     const std::string& controllerId) const
{
  bool bAccepted = false;

  if (m_address == portAddress)
  {
    // Base case
    CControllerPort port;
    GetPort(port);
    if (port.IsCompatible(controllerId))
      bAccepted = true;
  }
  else
  {
    // Visit nodes
    if (std::any_of(m_controllers.begin(), m_controllers.end(),
                    [portAddress, controllerId](const CControllerNode& node) {
                      return node.IsControllerAccepted(portAddress, controllerId);
                    }))
    {
      bAccepted = true;
    }
  }

  return bAccepted;
}

void CPortNode::GetPort(CControllerPort& port) const
{
  std::vector<std::string> accepts;
  for (const CControllerNode& node : m_controllers)
    accepts.emplace_back(node.Controller()->ID());

  port = CControllerPort(m_portId, std::move(accepts));
}
