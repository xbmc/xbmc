/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "GUIControllerList.h"

#include <algorithm>
#include <assert.h>
#include <iterator>

#include "GUIControllerDefines.h"
#include "GUIControllerWindow.h"
#include "GUIFeatureList.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerFeature.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/guicontrols/GUIControllerButton.h"
#include "games/controllers/guicontrols/GUIGameController.h"
#include "games/GameServices.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIWindow.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/JoystickIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "peripherals/Peripherals.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace ADDON;
using namespace GAME;

CGUIControllerList::CGUIControllerList(CGUIWindow* window, IFeatureList* featureList) :
  m_guiWindow(window),
  m_featureList(featureList),
  m_controllerList(nullptr),
  m_controllerButton(nullptr),
  m_focusedController(-1) // Initially unfocused
{
  assert(m_featureList != nullptr);
}

bool CGUIControllerList::Initialize(void)
{
  m_controllerList = dynamic_cast<CGUIControlGroupList*>(m_guiWindow->GetControl(CONTROL_CONTROLLER_LIST));
  m_controllerButton = dynamic_cast<CGUIButtonControl*>(m_guiWindow->GetControl(CONTROL_CONTROLLER_BUTTON_TEMPLATE));

  if (m_controllerButton)
    m_controllerButton->SetVisible(false);

  CAddonMgr::GetInstance().Events().Subscribe(this, &CGUIControllerList::OnEvent);
  Refresh();

  return m_controllerList != nullptr &&
         m_controllerButton != nullptr;
}

void CGUIControllerList::Deinitialize(void)
{
  CAddonMgr::GetInstance().Events().Unsubscribe(this);

  CleanupButtons();

  m_controllerList = nullptr;
  m_controllerButton = nullptr;
}

bool CGUIControllerList::Refresh(void)
{
  if (!RefreshControllers())
    return false;

  CleanupButtons();

  if (m_controllerList)
  {
    unsigned int buttonId = 0;
    for (ControllerVector::const_iterator it = m_controllers.begin(); it != m_controllers.end(); ++it)
    {
      const ControllerPtr& controller = *it;

      CGUIButtonControl* pButton = new CGUIControllerButton(*m_controllerButton, controller->Label(), buttonId++);
      m_controllerList->AddControl(pButton);

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
    CGUIGameController* pController = dynamic_cast<CGUIGameController*>(m_guiWindow->GetControl(CONTROL_GAME_CONTROLLER));
    if (pController)
      pController->ActivateController(controller);
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
  if (typeid(event) == typeid(ADDON::AddonEvents::InstalledChanged))
  {
    using namespace MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow->GetID(), CONTROL_CONTROLLER_LIST);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg);
  }
}

bool CGUIControllerList::RefreshControllers(void)
{
  // Get current controllers
  CGameServices& gameServices = CServiceBroker::GetGameServices();
  ControllerVector newControllers = gameServices.GetControllers();

  // Don't show an empty list in the GUI
  auto HasButtonForFeature = [this](const CControllerFeature &feature)
    {
      return m_featureList->HasButton(feature.Type());
    };

  auto HasButtonForController = [&](const ControllerPtr &controller)
    {
      const auto &features = controller->Layout().Features();
      auto it = std::find_if(features.begin(), features.end(), HasButtonForFeature);
      return it == features.end();
    };

  newControllers.erase(std::remove_if(newControllers.begin(), newControllers.end(), HasButtonForController), newControllers.end());

  // Check for changes
  std::set<std::string> oldControllerIds;
  std::set<std::string> newControllerIds;

  auto GetControllerID = [](const ControllerPtr& controller)
    {
      return controller->ID();
    };

  std::transform(m_controllers.begin(), m_controllers.end(), std::inserter(oldControllerIds, oldControllerIds.begin()), GetControllerID);
  std::transform(newControllers.begin(), newControllers.end(), std::inserter(newControllerIds, newControllerIds.begin()), GetControllerID);

  const bool bChanged = (oldControllerIds != newControllerIds);
  if (bChanged)
  {
    m_controllers = std::move(newControllers);

    // Sort add-ons, with default controller first
    std::sort(m_controllers.begin(), m_controllers.end(),
      [](const ControllerPtr& i, const ControllerPtr& j)
      {
        if (i->ID() == DEFAULT_CONTROLLER_ID && j->ID() != DEFAULT_CONTROLLER_ID) return true;
        if (i->ID() != DEFAULT_CONTROLLER_ID && j->ID() == DEFAULT_CONTROLLER_ID) return false;

        return i->Name() < j->Name();
      });
  }

  return bChanged;
}

void CGUIControllerList::CleanupButtons(void)
{
  if (m_controllerList)
    m_controllerList->ClearAll();
}
