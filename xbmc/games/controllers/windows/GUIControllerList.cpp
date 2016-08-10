/*
 *      Copyright (C) 2014-2016 Team Kodi
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
#include "games/controllers/guicontrols/GUIControllerButton.h"
#include "games/controllers/guicontrols/GUIGameController.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIWindow.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/DefaultJoystick.h" // for DEFAULT_CONTROLLER_ID
#include "messaging/ApplicationMessenger.h"
#include "peripherals/Peripherals.h"

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

    PERIPHERALS::g_peripherals.ResetButtonMaps(strControllerId);
  }
}

void CGUIControllerList::OnEvent(const ADDON::AddonEvent& event)
{
  if (typeid(event) == typeid(ADDON::AddonEvents::InstalledChanged))
  {
    using namespace KODI::MESSAGING;
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, m_guiWindow->GetID(), CONTROL_CONTROLLER_LIST);
    CApplicationMessenger::GetInstance().SendGUIMessage(msg);
  }
}

bool CGUIControllerList::RefreshControllers(void)
{
  // Cache discovered add-ons between function calls
  VECADDONS addonCache;

  std::set<std::string> currentIds = GetControllerIDs();
  std::set<std::string> newIds = GetNewControllerIDs(addonCache);

  std::set<std::string> added;
  std::set<std::string> removed;

  std::set_difference(newIds.begin(), newIds.end(), currentIds.begin(), currentIds.end(), std::inserter(added, added.end()));
  std::set_difference(currentIds.begin(), currentIds.end(), newIds.begin(), newIds.end(), std::inserter(removed, removed.end()));

  // Register new controllers
  for (const std::string& addonId : added)
    RegisterController(addonId, addonCache);

  // Erase removed controllers
  for (const std::string& addonId : removed)
    UnregisterController(addonId);

  // Sort add-ons, with default controller first
  const bool bChanged = !added.empty() || !removed.empty();
  if (bChanged)
  {
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

std::set<std::string> CGUIControllerList::GetControllerIDs() const
{
  std::set<std::string> controllerIds;

  std::transform(m_controllers.begin(), m_controllers.end(), std::inserter(controllerIds, controllerIds.end()),
    [](const ControllerPtr& addon)
    {
      return addon->ID();
    });

  return controllerIds;
}

std::set<std::string> CGUIControllerList::GetNewControllerIDs(ADDON::VECADDONS& addonCache) const
{
  std::set<std::string> controllerIds;

  CAddonMgr::GetInstance().GetAddons(addonCache, ADDON_GAME_CONTROLLER);

  std::transform(addonCache.begin(), addonCache.end(), std::inserter(controllerIds, controllerIds.end()),
    [](const AddonPtr& addon)
    {
      return addon->ID();
    });

  return controllerIds;
}

void CGUIControllerList::RegisterController(const std::string& addonId, const ADDON::VECADDONS& addonCache)
{
  auto it = std::find_if(addonCache.begin(), addonCache.end(),
    [addonId](const AddonPtr& addon)
    {
      return addon->ID() == addonId;
    });

  if (it != addonCache.end())
  {
    ControllerPtr newController = std::dynamic_pointer_cast<CController>(*it);
    if (newController && newController->LoadLayout())
      m_controllers.push_back(newController);
  }
}

void CGUIControllerList::UnregisterController(const std::string& controllerId)
{
  m_controllers.erase(std::remove_if(m_controllers.begin(), m_controllers.end(),
    [controllerId](const ControllerPtr& controller)
    {
      return controller->ID() == controllerId;
    }), m_controllers.end());
}

void CGUIControllerList::CleanupButtons(void)
{
  if (m_controllerList)
    m_controllerList->ClearAll();
}
