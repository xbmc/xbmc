/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientTopology.h"

#include "GameClientDevice.h"
#include "GameClientPort.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/controllers/Controller.h"

#include <sstream>
#include <utility>

using namespace KODI;
using namespace GAME;

#define CONTROLLER_ADDRESS_SEPARATOR '/'

namespace
{
// Helper function
bool ContainsOnlySeparator(const std::string& address,
                           std::string::size_type firstSep,
                           std::string::size_type lastSep)
{
  if (firstSep == std::string::npos)
    return true;

  if (firstSep == lastSep && address[firstSep] == CONTROLLER_ADDRESS_SEPARATOR)
    return true;

  return false;
}
} // namespace

CGameClientTopology::CGameClientTopology(GameClientPortVec ports, int playerLimit)
  : m_ports(std::move(ports)), m_playerLimit(playerLimit), m_controllers(GetControllerTree(m_ports))
{
}

void CGameClientTopology::Clear()
{
  m_ports.clear();
  m_controllers.Clear();
}

CControllerTree CGameClientTopology::GetControllerTree(const GameClientPortVec& ports)
{
  CControllerTree tree;

  PortVec controllerPorts;
  for (const GameClientPortPtr& port : ports)
  {
    CPortNode portNode = GetPortNode(port, "");
    controllerPorts.emplace_back(std::move(portNode));
  }

  tree.SetPorts(std::move(controllerPorts));

  return tree;
}

CPortNode CGameClientTopology::GetPortNode(const GameClientPortPtr& port,
                                           const std::string& controllerAddress)
{
  CPortNode portNode;

  std::string portAddress = MakeAddress(controllerAddress, port->ID());

  portNode.SetConnected(false);
  portNode.SetPortType(port->PortType());
  portNode.SetPortID(port->ID());
  portNode.SetAddress(portAddress);
  portNode.SetForceConnected(port->ForceConnected());

  ControllerNodeVec nodes;
  for (const GameClientDevicePtr& device : port->Devices())
  {
    CControllerNode controllerNode = GetControllerNode(device, portAddress);
    nodes.emplace_back(std::move(controllerNode));
  }
  portNode.SetCompatibleControllers(std::move(nodes));

  return portNode;
}

CControllerNode CGameClientTopology::GetControllerNode(const GameClientDevicePtr& device,
                                                       const std::string& portAddress)
{
  CControllerNode controllerNode;

  const std::string controllerAddress = MakeAddress(portAddress, device->Controller()->ID());

  controllerNode.SetController(device->Controller());
  controllerNode.SetPortAddress(portAddress);
  controllerNode.SetControllerAddress(controllerAddress);

  PortVec ports;
  for (const GameClientPortPtr& port : device->Ports())
  {
    CPortNode portNode = GetPortNode(port, controllerAddress);
    ports.emplace_back(std::move(portNode));
  }

  CControllerHub controllerHub;
  controllerHub.SetPorts(std::move(ports));
  controllerNode.SetHub(std::move(controllerHub));

  return controllerNode;
}

std::string CGameClientTopology::MakeAddress(const std::string& baseAddress,
                                             const std::string& nodeId)
{
  std::ostringstream address;

  // Strip leading and trailing slashes from base address
  std::string basePart;

  std::string::size_type first = baseAddress.find_first_not_of(CONTROLLER_ADDRESS_SEPARATOR);
  std::string::size_type last = baseAddress.find_last_not_of(CONTROLLER_ADDRESS_SEPARATOR);
  if (!ContainsOnlySeparator(baseAddress, first, last))
    basePart = baseAddress.substr(first, last - first + 1);

  // Strip leading and trailing slashes from node ID
  std::string nodePart;

  first = nodeId.find_first_not_of(CONTROLLER_ADDRESS_SEPARATOR);
  last = nodeId.find_last_not_of(CONTROLLER_ADDRESS_SEPARATOR);
  if (!ContainsOnlySeparator(nodeId, first, last))
    nodePart = nodeId.substr(first, last - first + 1);

  // Start with the root port address
  address << ROOT_PORT_ADDRESS;

  if (!basePart.empty() && !nodePart.empty())
  {
    address << basePart;
    address << CONTROLLER_ADDRESS_SEPARATOR;
    address << nodePart;
  }
  else if (!basePart.empty())
  {
    address << basePart;
  }
  else if (!nodePart.empty())
  {
    address << nodePart;
  }

  return address.str();
}

std::pair<std::string, std::string> CGameClientTopology::SplitAddress(
    const std::string& nodeAddress)
{
  std::string baseAddress;
  std::string nodeId;

  size_t separatorPos = nodeAddress.find_last_of(CONTROLLER_ADDRESS_SEPARATOR);
  if (separatorPos != std::string::npos)
  {
    baseAddress = nodeAddress.substr(0, separatorPos);
    nodeId = nodeAddress.substr(separatorPos + 1);
  }
  else
  {
    baseAddress = nodeAddress;
  }

  // Add a leading slash, if necessary
  if (baseAddress.empty() || baseAddress[0] != CONTROLLER_ADDRESS_SEPARATOR)
    baseAddress.insert(0, 1, CONTROLLER_ADDRESS_SEPARATOR);

  return std::make_pair(baseAddress, nodeId);
}
