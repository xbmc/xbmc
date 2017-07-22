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

#include "GameClientTopology.h"
#include "GameClientDevice.h"
#include "GameClientPort.h"
#include "games/controllers/Controller.h"

#include <sstream>
#include <utility>

using namespace KODI;
using namespace GAME;

#define CONTROLLER_ADDRESS_SEPARATOR  "/"

CGameClientTopology::CGameClientTopology(GameClientPortVec ports) :
  m_ports(std::move(ports))
{
}

CControllerTree CGameClientTopology::GetControllerTree() const
{
  return GetControllerTree(m_ports);
}

CControllerTree CGameClientTopology::GetControllerTree(const GameClientPortVec &ports)
{
  CControllerTree tree;

  ControllerPortVec controllerPorts;
  for (const GameClientPortPtr &port : ports)
  {
    CControllerPortNode portNode = GetPortNode(port, "");
    controllerPorts.emplace_back(std::move(portNode));
  }

  tree.SetPorts(std::move(controllerPorts));

  return tree;
}

CControllerPortNode CGameClientTopology::GetPortNode(const GameClientPortPtr &port, const std::string &address)
{
  CControllerPortNode portNode;

  std::string portAddress = MakeAddress(address, port->ID());

  portNode.SetConnected(false);
  portNode.SetPortType(port->PortType());
  portNode.SetPortID(port->ID());
  portNode.SetAddress(portAddress);

  ControllerNodeVec nodes;
  for (const GameClientDevicePtr &device : port->Devices())
  {
    CControllerNode controllerNode = GetControllerNode(device, portAddress);
    nodes.emplace_back(std::move(controllerNode));
  }
  portNode.SetCompatibleControllers(std::move(nodes));

  return portNode;
}

CControllerNode CGameClientTopology::GetControllerNode(const GameClientDevicePtr &device, const std::string &address)
{
  CControllerNode controllerNode;

  std::string controllerAddress = MakeAddress(address, device->Controller()->ID());

  controllerNode.SetController(device->Controller());
  controllerNode.SetAddress(controllerAddress);

  ControllerPortVec ports;
  for (const GameClientPortPtr &port : device->Ports())
  {
    CControllerPortNode portNode = GetPortNode(port, controllerAddress);
    ports.emplace_back(std::move(portNode));
  }

  CControllerHub controllerHub;
  controllerHub.SetPorts(std::move(ports));
  controllerNode.SetHub(std::move(controllerHub));

  return controllerNode;
}

std::string CGameClientTopology::MakeAddress(const std::string &baseAddress, const std::string &nodeId)
{
  std::ostringstream address;

  if (!baseAddress.empty())
    address << baseAddress << CONTROLLER_ADDRESS_SEPARATOR;

  address << nodeId;

  return address.str();
}
