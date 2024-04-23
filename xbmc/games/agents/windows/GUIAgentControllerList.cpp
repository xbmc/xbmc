/*
 *  Copyright (C) 2022-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIAgentControllerList.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIAgentDefines.h"
#include "GUIAgentWindow.h"
#include "ServiceBroker.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/agents/input/AgentController.h"
#include "games/agents/input/AgentInput.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "peripherals/Peripherals.h"
#include "peripherals/devices/Peripheral.h"
#include "peripherals/dialogs/GUIDialogPeripheralSettings.h"
#include "utils/log.h"
#include "view/GUIViewControl.h"
#include "view/ViewState.h"

using namespace KODI;
using namespace GAME;

CGUIAgentControllerList::CGUIAgentControllerList(CGUIWindow& window)
  : m_guiWindow(window),
    m_viewControl(std::make_unique<CGUIViewControl>()),
    m_vecItems(std::make_unique<CFileItemList>())
{
}

CGUIAgentControllerList::~CGUIAgentControllerList()
{
  Deinitialize();
}

void CGUIAgentControllerList::OnWindowLoaded()
{
  m_viewControl->Reset();
  m_viewControl->SetParentWindow(m_guiWindow.GetID());
  m_viewControl->AddView(m_guiWindow.GetControl(CONTROL_AGENT_CONTROLLER_LIST));
}

void CGUIAgentControllerList::OnWindowUnload()
{
  m_viewControl->Reset();
}

bool CGUIAgentControllerList::Initialize(GameClientPtr gameClient)
{
  // Validate parameters
  if (!gameClient)
    return false;

  // Initialize state
  m_gameClient = std::move(gameClient);
  m_viewControl->SetCurrentView(DEFAULT_VIEW_LIST);

  // Initialize GUI
  Refresh();
  m_viewControl->SetSelectedItem(0);

  // Register observers
  if (m_gameClient)
    m_gameClient->Input().RegisterObserver(this);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIAgentControllerList::OnEvent);
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetGameServices().AgentInput().RegisterObserver(this);

  return true;
}

void CGUIAgentControllerList::Deinitialize()
{
  // Unregister observers in reverse order
  if (CServiceBroker::IsServiceManagerUp())
    CServiceBroker::GetGameServices().AgentInput().UnregisterObserver(this);
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  if (m_gameClient)
    m_gameClient->Input().UnregisterObserver(this);

  // Deinitialize GUI
  CleanupItems();

  // Reset state
  m_gameClient.reset();
}

bool CGUIAgentControllerList::HasControl(int controlId) const
{
  return m_viewControl->HasControl(controlId);
}

int CGUIAgentControllerList::GetCurrentControl() const
{
  return m_viewControl->GetCurrentControl();
}

void CGUIAgentControllerList::FrameMove()
{
  CGUIBaseContainer* thumbs =
      dynamic_cast<CGUIBaseContainer*>(m_guiWindow.GetControl(CONTROL_AGENT_CONTROLLER_LIST));
  if (thumbs != nullptr)
  {
    const int selectedItem = thumbs->GetSelectedItem();
    if (0 <= selectedItem && selectedItem < m_vecItems->Size())
    {
      const unsigned int focusedItem = static_cast<unsigned int>(selectedItem);
      if (focusedItem != m_currentItem)
        OnItemFocus(focusedItem);
    }
  }
}

void CGUIAgentControllerList::Refresh()
{
  // Send a synchronous message to clear the view control
  m_viewControl->Clear();

  CleanupItems();

  CAgentInput& agentInput = CServiceBroker::GetGameServices().AgentInput();

  std::vector<std::shared_ptr<const CAgentController>> agentControllers =
      agentInput.GetControllers();
  for (const std::shared_ptr<const CAgentController>& agentController : agentControllers)
    AddItem(*agentController);

  // Add a "No controllers connected" item if no agents are available
  if (m_vecItems->IsEmpty())
  {
    CFileItemPtr item =
        std::make_shared<CFileItem>(g_localizeStrings.Get(35173)); // "No controllers connected"
    m_vecItems->Add(std::move(item));
  }

  // Update items
  m_viewControl->SetItems(*m_vecItems);

  // Try to restore focus to the previously focused agent
  for (unsigned int currentItem = 0; static_cast<int>(currentItem) < m_vecItems->Size();
       ++currentItem)
  {
    CFileItemPtr item = m_vecItems->Get(currentItem);
    if (item && item->GetPath() == m_currentAgent)
    {
      m_viewControl->SetSelectedItem(currentItem);
      break;
    }
  }
}

void CGUIAgentControllerList::SetFocused()
{
  m_viewControl->SetFocused();
}

void CGUIAgentControllerList::OnSelect()
{
  const int itemIndex = m_viewControl->GetSelectedItem();
  if (itemIndex >= 0)
    OnItemSelect(static_cast<unsigned int>(itemIndex));
}

void CGUIAgentControllerList::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageAgentControllersChanged:
    case ObservableMessageGamePortsChanged:
    {
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AGENT_CONTROLLER_LIST);
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
      break;
    }
    default:
      break;
  }
}

void CGUIAgentControllerList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // Also called on install
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // Not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow.GetID(), CONTROL_AGENT_CONTROLLER_LIST);
    msg.SetStringParam(event.addonId);
    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow.GetID());
  }
}

void CGUIAgentControllerList::AddItem(const CAgentController& agentController)
{
  // Create the list item from agent properties
  const std::string label = agentController.GetPeripheralName();
  const ControllerPtr controller = agentController.GetController();
  const std::string& path = agentController.GetPeripheralLocation();

  CFileItemPtr item = std::make_shared<CFileItem>(label);
  item->SetPath(path);
  if (controller)
  {
    item->SetProperty("Addon.ID", controller->ID());
    item->SetArt("icon", controller->Layout().ImagePath());
  }
  m_vecItems->Add(std::move(item));
}

void CGUIAgentControllerList::CleanupItems()
{
  m_vecItems->Clear();
}

void CGUIAgentControllerList::OnItemFocus(unsigned int itemIndex)
{
  // Remember the focused item
  m_currentItem = itemIndex;

  // Handle the focused agent
  CFileItemPtr item = m_vecItems->Get(m_currentItem);
  if (item)
    OnControllerFocus(item->GetPath());
}

void CGUIAgentControllerList::OnControllerFocus(const std::string& focusedAgent)
{
  if (!focusedAgent.empty())
  {
    // Remember the focused agent
    m_currentAgent = focusedAgent;
  }
}

void CGUIAgentControllerList::OnItemSelect(unsigned int itemIndex)
{
  // Handle the selected agent
  CFileItemPtr item = m_vecItems->Get(itemIndex);
  if (item)
    OnControllerSelect(*item);
}

void CGUIAgentControllerList::OnControllerSelect(const CFileItem& selectedAgentItem)
{
  CAgentInput& agentInput = CServiceBroker::GetGameServices().AgentInput();

  std::vector<std::shared_ptr<const CAgentController>> agentControllers =
      agentInput.GetControllers();
  for (const std::shared_ptr<const CAgentController>& agentController : agentControllers)
  {
    PERIPHERALS::PeripheralPtr peripheral = agentController->GetPeripheral();
    if (peripheral && peripheral->Location() == selectedAgentItem.GetPath())
    {
      if (peripheral->GetSettings().empty())
      {
        // Show an error if the peripheral doesn't have any settings
        CLog::Log(LOGERROR, "Peripheral has no settings");

        // "Peripherals"
        // "There are no settings available for this peripheral"
        MESSAGING::HELPERS::ShowOKDialogText(CVariant{35000}, CVariant{35004});
      }
      else
      {
        ShowControllerDialog(*agentController);
      }
      break;
    }
  }
}

void CGUIAgentControllerList::ShowControllerDialog(const CAgentController& agentController)
{
  // Get the dialog
  PERIPHERALS::CGUIDialogPeripheralSettings* pSettingsDialog =
      CServiceBroker::GetGUI()
          ->GetWindowManager()
          .GetWindow<PERIPHERALS::CGUIDialogPeripheralSettings>(WINDOW_DIALOG_PERIPHERAL_SETTINGS);
  if (pSettingsDialog == nullptr)
    return;

  const PERIPHERALS::PeripheralPtr peripheral = agentController.GetPeripheral();

  // Get a file item for the peripheral settings dialog
  CFileItemPtr peripheralItem;

  CFileItemList peripherals;
  CServiceBroker::GetPeripherals().GetDirectory("peripherals://all/", peripherals);
  for (int i = 0; i < peripherals.Size(); ++i)
  {
    if (peripherals[i]->GetProperty("location").asString() == peripheral->Location())
    {
      peripheralItem = peripherals[i];
      break;
    }
  }

  if (!peripheralItem)
  {
    CLog::Log(LOGERROR, "Failed to get peripheral for location {}", peripheral->Location());
    if (peripherals.IsEmpty())
      CLog::Log(LOGERROR, "No peripherals available");
    else
    {
      CLog::Log(LOGERROR, "Available peripherals are:");
      for (int i = 0; i < peripherals.Size(); ++i)
        CLog::Log(LOGERROR, "  - \"{}\" ({})", peripherals[i]->GetProperty("location").asString(),
                  peripherals[i]->GetPath());
    }
    return;
  }

  // Remember controller profile to detect changes
  const std::string oldControllerId =
      peripheral->ControllerProfile() ? peripheral->ControllerProfile()->ID() : "";

  // Pass peripheral item properties to settings dialog so skin authors
  // can use it to show more detailed information about the device
  pSettingsDialog->SetProperty("vendor", peripheralItem->GetProperty("vendor"));
  pSettingsDialog->SetProperty("product", peripheralItem->GetProperty("product"));
  pSettingsDialog->SetProperty("bus", peripheralItem->GetProperty("bus"));
  pSettingsDialog->SetProperty("location", peripheralItem->GetProperty("location"));
  pSettingsDialog->SetProperty("class", peripheralItem->GetProperty("class"));
  pSettingsDialog->SetProperty("version", peripheralItem->GetProperty("version"));

  // Open settings dialog
  pSettingsDialog->SetFileItem(peripheralItem.get());
  pSettingsDialog->RegisterPeripheralManager(CServiceBroker::GetPeripherals());
  pSettingsDialog->Open();
  pSettingsDialog->UnregisterPeripheralManager();

  // Check for changes in the controller profile
  const std::string newControllerId =
      peripheral->ControllerProfile() ? peripheral->ControllerProfile()->ID() : "";

  if (oldControllerId != newControllerId)
  {
    CServiceBroker::GetPeripherals().SetChanged(true);
    CServiceBroker::GetPeripherals().NotifyObservers(ObservableMessagePeripheralsChanged);
  }
}
