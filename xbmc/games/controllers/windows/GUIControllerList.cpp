/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControllerList.h"

#include "GUIControllerDefines.h"
#include "GUIControllerWindow.h"
#include "GUIFeatureList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/input/GameClientInput.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/guicontrols/GUIControllerButton.h"
#include "games/controllers/guicontrols/GUIGameController.h"
#include "games/controllers/types/ControllerTree.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindow.h"
#include "messaging/ApplicationMessenger.h"
#include "peripherals/Peripherals.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <assert.h>
#include <iterator>

using namespace KODI;
using namespace GAME;

CGUIControllerList::CGUIControllerList(CGUIWindow* window,
                                       IFeatureList* featureList,
                                       GameClientPtr gameClient,
                                       std::string controllerId)
  : m_guiWindow(window),
    m_featureList(featureList),
    m_gameClient(std::move(gameClient)),
    m_controllerId(std::move(controllerId))
{
  assert(m_featureList != nullptr);
}

bool CGUIControllerList::Initialize(void)
{
  m_controllerList =
      dynamic_cast<CGUIControlGroupList*>(m_guiWindow->GetControl(CONTROL_CONTROLLER_LIST));
  m_controllerButton =
      dynamic_cast<CGUIButtonControl*>(m_guiWindow->GetControl(CONTROL_CONTROLLER_BUTTON_TEMPLATE));

  if (m_controllerButton)
    m_controllerButton->SetVisible(false);

  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIControllerList::OnEvent);
  Refresh("");

  return m_controllerList != nullptr && m_controllerButton != nullptr;
}

void CGUIControllerList::Deinitialize(void)
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

  CleanupButtons();

  m_controllerList = nullptr;
  m_controllerButton = nullptr;
}

bool CGUIControllerList::Refresh(const std::string& controllerId)
{
  // Focus specified controller after refresh
  std::string focusController = controllerId;

  if (focusController.empty() && m_focusedController >= 0)
  {
    // If controller ID wasn't provided, focus current controller
    focusController = m_controllers[m_focusedController]->ID();
  }

  if (!RefreshControllers())
    return false;

  CleanupButtons();

  if (m_controllerList)
  {
    unsigned int buttonId = 0;
    for (const auto& controller : m_controllers)
    {
      CGUIButtonControl* pButton =
          new CGUIControllerButton(*m_controllerButton, controller->Layout().Label(), buttonId++);
      m_controllerList->AddControl(pButton);

      if (!focusController.empty() && controller->ID() == focusController)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, m_guiWindow->GetID(), pButton->GetID());
        m_guiWindow->OnMessage(msg);
      }

      // Just in case
      if (buttonId >= MAX_CONTROLLER_COUNT)
        break;
    }
  }

  return true;
}

void CGUIControllerList::OnFocus(unsigned int controllerIndex)
{
  if (controllerIndex < m_controllers.size())
  {
    m_focusedController = controllerIndex;

    const ControllerPtr& controller = m_controllers[controllerIndex];
    m_featureList->Load(controller);

    //! @todo Activate controller for all game controller controls
    CGUIGameController* pController =
        dynamic_cast<CGUIGameController*>(m_guiWindow->GetControl(CONTROL_GAME_CONTROLLER));
    if (pController)
      pController->ActivateController(controller);

    // Update controller description
    CGUIMessage msg(GUI_MSG_LABEL_SET, m_guiWindow->GetID(), CONTROL_CONTROLLER_DESCRIPTION);
    msg.SetLabel(controller->Description());
    m_guiWindow->OnMessage(msg);
  }
}

void CGUIControllerList::OnSelect(unsigned int controllerIndex)
{
  m_featureList->OnSelect(0);
}

void CGUIControllerList::ResetController(void)
{
  if (0 <= m_focusedController && m_focusedController < (int)m_controllers.size())
  {
    const std::string strControllerId = m_controllers[m_focusedController]->ID();

    //! @todo Choose peripheral
    // For now, ask the user if they would like to reset all peripherals
    // "Reset controller profile"
    // "Would you like to reset this controller profile for all devices?"
    if (!CGUIDialogYesNo::ShowAndGetInput(35060, 35061))
      return;

    CServiceBroker::GetPeripherals().ResetButtonMaps(strControllerId);
  }
}

void CGUIControllerList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) || // also called on install,
      typeid(event) == typeid(ADDON::AddonEvents::Disabled) || // not called on uninstall
      typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
      typeid(event) == typeid(ADDON::AddonEvents::UnInstalled))
  {
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow->GetID(), CONTROL_CONTROLLER_LIST);

    // Focus installed add-on
    if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::ReInstalled))
      msg.SetStringParam(event.addonId);

    CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, m_guiWindow->GetID());
  }
}

bool CGUIControllerList::RefreshControllers(void)
{
  // Get current controllers
  CGameServices& gameServices = CServiceBroker::GetGameServices();
  ControllerVector newControllers = gameServices.GetControllers();

  // Filter by specified controller ID
  if (!m_controllerId.empty())
  {
    newControllers.erase(std::remove_if(newControllers.begin(), newControllers.end(),
                                        [this](const ControllerPtr& controller)
                                        { return controller->ID() != m_controllerId; }),
                         newControllers.end());
  }
  // Filter by current game add-on
  else if (m_gameClient)
  {
    const CControllerTree& controllers = m_gameClient->Input().GetDefaultControllerTree();

    auto ControllerNotAccepted = [&controllers](const ControllerPtr& controller)
    { return !controllers.IsControllerAccepted(controller->ID()); };

    if (!std::all_of(newControllers.begin(), newControllers.end(), ControllerNotAccepted))
      newControllers.erase(
          std::remove_if(newControllers.begin(), newControllers.end(), ControllerNotAccepted),
          newControllers.end());
  }

  if (newControllers.empty())
    newControllers.emplace_back(gameServices.GetDefaultController());

  // Check for changes
  std::set<std::string> oldControllerIds;
  std::set<std::string> newControllerIds;

  auto GetControllerID = [](const ControllerPtr& controller) { return controller->ID(); };

  std::transform(m_controllers.begin(), m_controllers.end(),
                 std::inserter(oldControllerIds, oldControllerIds.begin()), GetControllerID);
  std::transform(newControllers.begin(), newControllers.end(),
                 std::inserter(newControllerIds, newControllerIds.begin()), GetControllerID);

  const bool bChanged = (oldControllerIds != newControllerIds);
  if (bChanged)
  {
    m_controllers = std::move(newControllers);

    // Sort add-ons, with default controller first
    std::sort(m_controllers.begin(), m_controllers.end(),
              [](const ControllerPtr& i, const ControllerPtr& j)
              {
                if (i->ID() == DEFAULT_CONTROLLER_ID && j->ID() != DEFAULT_CONTROLLER_ID)
                  return true;
                if (i->ID() != DEFAULT_CONTROLLER_ID && j->ID() == DEFAULT_CONTROLLER_ID)
                  return false;

                return StringUtils::CompareNoCase(i->Layout().Label(), j->Layout().Label()) < 0;
              });
  }

  return bChanged;
}

void CGUIControllerList::CleanupButtons(void)
{
  if (m_controllerList)
    m_controllerList->ClearAll();
}
