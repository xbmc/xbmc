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
#include "games/controllers/Controller.h"

#include <sstream>
#include <utility>

using namespace KODI;
using namespace GAME;

#define CONTROLLER_ADDRESS_SEPARATOR "/"

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

  ControllerPortVec controllerPorts;
  for (const GameClientPortPtr& port : ports)
  {
    CControllerPortNode portNode = GetPortNode(port, "");
    controllerPorts.emplace_back(std::move(portNode));
  }

  tree.SetPorts(std::move(controllerPorts));

  return tree;
}

CControllerPortNode CGameClientTopology::GetPortNode(const GameClientPortPtr& port,
                                                     const std::string& address)
{
  CControllerPortNode portNode;

  std::string portAddress = MakeAddress(address, port->ID());

  portNode.SetConnected(false);
  portNode.SetPortType(port->PortType());
  portNode.SetPortID(port->ID());
  portNode.SetAddress(portAddress);

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
                                                       const std::string& address)
{
  CControllerNode controllerNode;

  std::string controllerAddress = MakeAddress(address, device->Controller()->ID());

  controllerNode.SetController(device->Controller());
  controllerNode.SetAddress(controllerAddress);

  ControllerPortVec ports;
  for (const GameClientPortPtr& port : device->Ports())
  {
    CControllerPortNode portNode = GetPortNode(port, controllerAddress);
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

  if (!baseAddress.empty())
    address << baseAddress;

  address << CONTROLLER_ADDRESS_SEPARATOR << nodeId;

  return address.str();
}
