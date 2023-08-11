/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameControllerList.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/input/GameClientJoystick.h"
#include "games/addons/input/GameClientTopology.h"
#include "games/agents/GameAgent.h"
#include "games/agents/GameAgentManager.h"
#include "games/controllers/Controller.h"
#include "games/controllers/listproviders/GUIGameControllerProvider.h"
#include "guilib/GUIListItem.h"
#include "guilib/GUIMessage.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/Variant.h"

#include <algorithm>
#include <cstdlib>

using namespace KODI;
using namespace GAME;

CGUIGameControllerList::CGUIGameControllerList(int parentID,
                                               int controlID,
                                               float posX,
                                               float posY,
                                               float width,
                                               float height,
                                               ORIENTATION orientation,
                                               uint32_t alignment,
                                               const CScroller& scroller)
  : CGUIListContainer(parentID, controlID, posX, posY, width, height, orientation, scroller, 0),
    m_alignment(alignment)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLERLIST;
}

CGUIGameControllerList::CGUIGameControllerList(const CGUIGameControllerList& other)
  : CGUIListContainer(other), m_alignment(other.m_alignment)
{
  // Initialize CGUIControl
  ControlType = GUICONTROL_GAMECONTROLLERLIST;
}

CGUIGameControllerList* CGUIGameControllerList::Clone(void) const
{
  return new CGUIGameControllerList(*this);
}

void CGUIGameControllerList::UpdateInfo(const CGUIListItem* item)
{
  CGUIListContainer::UpdateInfo(item);

  if (item == nullptr)
    return;

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  // Update port count
  const std::vector<std::string> inputPorts = agentManager.GetInputPorts();
  m_portCount = inputPorts.size();

  // Update port index
  UpdatePort(item->GetCurrentItem(), inputPorts);

  bool bUpdateListProvider = false;

  // Update CGUIListContainer
  if (!m_listProvider)
  {
    m_listProvider = std::make_unique<CGUIGameControllerProvider>(
        m_portCount, m_portIndex, m_peripheralLocation, m_alignment, GetParentID());
    bUpdateListProvider = true;
  }

  // Update controller provider
  CGUIGameControllerProvider* controllerProvider =
      dynamic_cast<CGUIGameControllerProvider*>(m_listProvider.get());
  if (controllerProvider != nullptr)
  {
    // Update port count
    if (controllerProvider->GetPortCount() != m_portCount)
    {
      controllerProvider->SetPortCount(m_portCount);
      bUpdateListProvider = true;
    }

    // Update player count
    if (controllerProvider->GetPortIndex() != m_portIndex)
    {
      controllerProvider->SetPortIndex(m_portIndex);
      bUpdateListProvider = true;
    }

    // Update current controller
    const std::string newControllerId = item->GetProperty("Addon.ID").asString();
    if (!newControllerId.empty())
    {
      std::string currentControllerId;
      if (controllerProvider->GetControllerProfile())
        currentControllerId = controllerProvider->GetControllerProfile()->ID();

      if (currentControllerId != newControllerId)
      {
        CGameServices& gameServices = CServiceBroker::GetGameServices();
        ControllerPtr controller = gameServices.GetController(newControllerId);
        controllerProvider->SetControllerProfile(std::move(controller));
        bUpdateListProvider = true;
      }
    }

    // Update peripheral location
    if (controllerProvider->GetPeripheralLocation() != m_peripheralLocation)
    {
      controllerProvider->SetPeripheralLocation(m_peripheralLocation);
      bUpdateListProvider = true;
    }
  }

  if (bUpdateListProvider)
    UpdateListProvider(true);
}

void CGUIGameControllerList::SetGameClient(GAME::GameClientPtr gameClient)
{
  m_gameClient = std::move(gameClient);
}

void CGUIGameControllerList::ClearGameClient()
{
  m_gameClient.reset();
}

void CGUIGameControllerList::UpdatePort(int itemNumber, const std::vector<std::string>& inputPorts)
{
  // Item numbers start from 1
  if (itemNumber < 1)
    return;

  const unsigned int agentIndex = static_cast<unsigned int>(itemNumber - 1);

  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  GameAgentVec agents = agentManager.GetAgents();
  if (agentIndex < static_cast<unsigned int>(agents.size()))
  {
    const GameAgentPtr& agent = agents.at(agentIndex);
    UpdatePortIndex(agent->GetPeripheral(), inputPorts);
    UpdatePeripheral(agent->GetPeripheral());
  }
}

void CGUIGameControllerList::UpdatePortIndex(const PERIPHERALS::PeripheralPtr& agentPeripheral,
                                             const std::vector<std::string>& inputPorts)
{
  CGameAgentManager& agentManager = CServiceBroker::GetGameServices().GameAgentManager();

  // Upcast peripheral to input provider
  JOYSTICK::IInputProvider* const inputProvider =
      static_cast<JOYSTICK::IInputProvider*>(agentPeripheral.get());

  // See if the input provider has a port address
  std::string portAddress = agentManager.GetPortAddress(inputProvider);
  if (portAddress.empty())
    return;

  m_portIndex = -1;

  // Search ports for input provider's address
  for (size_t i = 0; i < inputPorts.size(); ++i)
  {
    if (inputPorts.at(i) == portAddress)
    {
      // Found port address, record index
      m_portIndex = i;
      break;
    }
  }
}

void CGUIGameControllerList::UpdatePeripheral(const PERIPHERALS::PeripheralPtr& agentPeripheral)
{
  m_peripheralLocation = agentPeripheral->Location();
}
