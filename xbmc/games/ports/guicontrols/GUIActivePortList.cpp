/*
 *  Copyright (C) 2021-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIActivePortList.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/windows/GUIAgentDefines.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/guicontrols/GUIGameControllerList.h"
#include "games/controllers/input/PhysicalTopology.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/ports/windows/GUIPortDefines.h"
#include "guilib/GUIFont.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "messaging/ApplicationMessenger.h"

using namespace KODI;
using namespace GAME;

CGUIActivePortList::CGUIActivePortList(CGUIWindow& window, int controlId, bool showInputDisabled)
  : m_guiWindow(window),
    m_controlId(controlId),
    m_showInputDisabled(showInputDisabled),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

CGUIActivePortList::~CGUIActivePortList()
{
  Deinitialize();
}

bool CGUIActivePortList::Initialize(GameClientPtr gameClient)
{
  // Validate parameters
  if (!gameClient)
    return false;

  // Initialize state
  m_gameClient = std::move(gameClient);

  // Initialize GUI
  InitializeGUI();

  // Register observers
  m_gameClient->Input().RegisterObserver(this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIActivePortList::OnEvent);

  return true;
}

void CGUIActivePortList::Deinitialize()
{
  // Unregister observers in reverse order
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  if (m_gameClient)
    m_gameClient->Input().UnregisterObserver(this);

  // Deinitialize GUI
  DeinitializeGUI();

  // Reset state
  m_gameClient.reset();
}

void CGUIActivePortList::Refresh()
{
  CleanupItems();

  // Add input disabled icon
  if (m_showInputDisabled)
    AddInputDisabled();

  // Add controllers of active ports
  if (m_gameClient)
  {
    CControllerTree controllerTree = m_gameClient->Input().GetActiveControllerTree();
    AddItems(controllerTree.GetPorts());
  }

  // Add padding if right-aligned
  if (m_alignment == XBFONT_RIGHT)
    AddPadding();

  // Update the GUI
  CGUIMessage msg(GUI_MSG_LABEL_BIND, m_guiWindow.GetID(), m_controlId, 0, 0, m_vecItems.get());
  m_guiWindow.OnMessage(msg);
}

void CGUIActivePortList::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageGamePortsChanged:
    {
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), m_controlId);
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
    }
    break;
    default:
      break;
  }
}

void CGUIActivePortList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), m_controlId);
    msg.SetStringParam(event.addonId);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

void CGUIActivePortList::InitializeGUI()
{
  CGUIGameControllerList* activePortList =
      dynamic_cast<CGUIGameControllerList*>(m_guiWindow.GetControl(m_controlId));

  if (activePortList != nullptr)
  {
    m_alignment = activePortList->GetAlignment();
    activePortList->SetGameClient(m_gameClient);
  }

  Refresh();
}

void CGUIActivePortList::DeinitializeGUI()
{
  CleanupItems();

  CGUIGameControllerList* activePortList =
      dynamic_cast<CGUIGameControllerList*>(m_guiWindow.GetControl(m_controlId));

  if (activePortList != nullptr)
    activePortList->ClearGameClient();
}

void CGUIActivePortList::AddInputDisabled()
{
  CFileItem item;
  item.SetArt("icon", "DefaultAddonNone.png");
  m_vecItems->Add(std::move(item));
}

void CGUIActivePortList::AddItems(const PortVec& ports)
{
  for (const CPortNode& port : ports)
  {
    // Add controller
    ControllerPtr controller = port.GetActiveController().GetController();
    const std::string& controllerAddress = port.GetActiveController().GetControllerAddress();
    AddItem(controller, controllerAddress);

    // Add child ports
    AddItems(port.GetActiveController().GetHub().GetPorts());
  }
}

void CGUIActivePortList::AddItem(const ControllerPtr& controller,
                                 const std::string& controllerAddress)
{
  // Check if a controller is connected that provides input
  if (controller && controller->Topology().ProvidesInput())
  {
    // Add GUI item
    CFileItemPtr item = std::make_shared<CFileItem>(controller->Layout().Label());
    item->SetArt("icon", controller->Layout().ImagePath());
    item->SetPath(controllerAddress);
    m_vecItems->Add(std::move(item));
  }
}

void CGUIActivePortList::AddPadding()
{
  unsigned int itemCount = MAX_PORT_COUNT;
  if (m_showInputDisabled)
    itemCount++;

  while (m_vecItems->Size() < static_cast<int>(itemCount))
    m_vecItems->AddFront(std::make_shared<CFileItem>(), 0);
}

void CGUIActivePortList::CleanupItems()
{
  m_vecItems->Clear();
}
