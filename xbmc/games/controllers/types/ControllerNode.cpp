/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerNode.h"

#include "ControllerHub.h"
#include "PortNode.h"
#include "games/controllers/Controller.h"
#include "games/controllers/input/PhysicalTopology.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace GAME;

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

CControllerNode& CControllerNode::operator=(CControllerNode&& rhs) noexcept
{
  if (this != &rhs)
  {
    m_controller = std::move(rhs.m_controller);
    m_address = std::move(rhs.m_address);
    m_hub = std::move(rhs.m_hub);
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
  if (m_controller)
  {
    const ControllerPtr& myController = m_controller;

    auto it = std::find_if(controllers.begin(), controllers.end(),
                           [&myController](const ControllerPtr& controller) {
                             return myController->ID() == controller->ID();
                           });

    if (it == controllers.end())
      controllers.emplace_back(m_controller);
  }

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

  for (const auto& port : m_hub->GetPorts())
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

  for (const auto& port : m_hub->GetPorts())
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
