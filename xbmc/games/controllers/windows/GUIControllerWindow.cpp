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

#include "GUIControllerWindow.h"
#include "GUIControllerDefines.h"
#include "GUIControllerList.h"
#include "GUIFeatureList.h"
#include "IConfigurationWindow.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "addons/IAddon.h"
#include "addons/AddonManager.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"

// To check for button mapping support
#include "dialogs/GUIDialogOK.h"
#include "peripherals/bus/virtual/PeripheralBusAddon.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

// To check for installable controllers
#include "addons/AddonDatabase.h"
#include "addons/AddonManager.h"

using namespace GAME;

CGUIControllerWindow::CGUIControllerWindow(void) :
  CGUIDialog(WINDOW_DIALOG_GAME_CONTROLLERS, "DialogGameControllers.xml"),
  m_controllerList(nullptr),
  m_featureList(nullptr)
{
  // initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIControllerWindow::~CGUIControllerWindow(void)
{
  delete m_controllerList;
  delete m_featureList;
}

bool CGUIControllerWindow::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      //! @todo Process parameter
      //std::string strParam = message.GetStringParam();
      break;
    }
    case GUI_MSG_CLICKED:
    {
      int controlId = message.GetSenderId();

      if (controlId == CONTROL_CLOSE_BUTTON)
      {
        Close();
        return true;
      }
      else if (controlId == CONTROL_GET_MORE)
      {
        GetMoreControllers();
        return true;
      }
      else if (controlId == CONTROL_RESET_BUTTON)
      {
        ResetController();
        return true;
      }
      else if (controlId == CONTROL_HELP_BUTTON)
      {
        ShowHelp();
        return true;
      }
      else if (CONTROL_CONTROLLER_BUTTONS_START <= controlId && controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerSelected(controlId - CONTROL_CONTROLLER_BUTTONS_START);
        return true;
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId && controlId < CONTROL_FEATURE_BUTTONS_END)
      {
        OnFeatureSelected(controlId - CONTROL_FEATURE_BUTTONS_START);
        return true;
      }
      break;
    }
    case GUI_MSG_FOCUSED:
    {
      int controlId = message.GetControlId();

      if (CONTROL_CONTROLLER_BUTTONS_START <= controlId && controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerFocused(controlId - CONTROL_CONTROLLER_BUTTONS_START);
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId && controlId < CONTROL_FEATURE_BUTTONS_END)
      {
        OnFeatureFocused(controlId - CONTROL_FEATURE_BUTTONS_START);
      }
      break;
    }
    case GUI_MSG_SETFOCUS:
    {
      int controlId = message.GetControlId();

      if (CONTROL_CONTROLLER_BUTTONS_START <= controlId && controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerFocused(controlId - CONTROL_CONTROLLER_BUTTONS_START);
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId && controlId < CONTROL_FEATURE_BUTTONS_END)
      {
        OnFeatureFocused(controlId - CONTROL_FEATURE_BUTTONS_START);
      }
      break;
    }
    case GUI_MSG_REFRESH_LIST:
    {
      int controlId = message.GetControlId();

      if (controlId == CONTROL_CONTROLLER_LIST)
      {
        if (m_controllerList && m_controllerList->Refresh())
        {
          CGUIDialog::OnMessage(message);
          return true;
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIControllerWindow::OnEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event)
{
  UpdateButtons();
}

void CGUIControllerWindow::OnInitWindow(void)
{
  using namespace PERIPHERALS;

  CGUIDialog::OnInitWindow();

  if (!m_featureList)
  {
    m_featureList = new CGUIFeatureList(this);
    if (!m_featureList->Initialize())
    {
      delete m_featureList;
      m_featureList = nullptr;
    }
  }

  if (!m_controllerList && m_featureList)
  {
    m_controllerList = new CGUIControllerList(this, m_featureList);
    if (!m_controllerList->Initialize())
    {
      delete m_controllerList;
      m_controllerList = nullptr;
    }
  }

  // Focus the first controller so that the feature list is loaded properly
  CGUIMessage msgFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_CONTROLLER_BUTTONS_START);
  OnMessage(msgFocus);

  // Check for button mapping support
  //! @todo remove this
  PeripheralBusAddonPtr bus = std::static_pointer_cast<CPeripheralBusAddon>(g_peripherals.GetBusByType(PERIPHERAL_BUS_ADDON));
  if (bus && !bus->HasFeature(FEATURE_JOYSTICK))
  {
    //! @todo Move the XML implementation of button map storage from add-on to
    //! Kodi while keeping support for add-on button-mapping

    CLog::Log(LOGERROR, "Joystick support not found");

    // "Joystick support not found"
    // "Controller configuration is disabled. Install the proper joystick support add-on."
    CGUIDialogOK::ShowAndGetInput(CVariant{35056}, CVariant{35057});

    // close the window as there's nothing that can be done
    Close();
  }

  // FIXME: not thread safe
//  ADDON::CRepositoryUpdater::GetInstance().Events().Subscribe(this, &CGUIControllerWindow::OnEvent);

  UpdateButtons();
}

void CGUIControllerWindow::OnDeinitWindow(int nextWindowID)
{
  ADDON::CRepositoryUpdater::GetInstance().Events().Unsubscribe(this);

  if (m_controllerList)
  {
    m_controllerList->Deinitialize();
    delete m_controllerList;
    m_controllerList = nullptr;
  }

  if (m_featureList)
  {
    m_featureList->Deinitialize();
    delete m_featureList;
    m_featureList = nullptr;
  }

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIControllerWindow::OnControllerFocused(unsigned int controllerIndex)
{
  if (m_controllerList)
    m_controllerList->OnFocus(controllerIndex);
}

void CGUIControllerWindow::OnControllerSelected(unsigned int controllerIndex)
{
  if (m_controllerList)
    m_controllerList->OnSelect(controllerIndex);
}

void CGUIControllerWindow::OnFeatureFocused(unsigned int featureIndex)
{
  if (m_featureList)
    m_featureList->OnFocus(featureIndex);
}

void CGUIControllerWindow::OnFeatureSelected(unsigned int featureIndex)
{
  if (m_featureList)
    m_featureList->OnSelect(featureIndex);
}

void CGUIControllerWindow::UpdateButtons(void)
{
  using namespace ADDON;

  VECADDONS addons;
  CONTROL_ENABLE_ON_CONDITION(CONTROL_GET_MORE, CAddonMgr::GetInstance().GetInstallableAddons(addons, ADDON::ADDON_GAME_CONTROLLER) && !addons.empty());
}

void CGUIControllerWindow::GetMoreControllers(void)
{
  std::string strAddonId;
  if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::ADDON_GAME_CONTROLLER, strAddonId, false, true, false, true, false) < 0)
  {
    // "Controller profiles"
    // "All available controller profiles are installed."
    CGUIDialogOK::ShowAndGetInput(CVariant{ 35050 }, CVariant{ 35062 });
    return;
  }
}

void CGUIControllerWindow::ResetController(void)
{
  if (m_controllerList)
    m_controllerList->ResetController();
}

void CGUIControllerWindow::ShowHelp(void)
{
  // "Help"
  // <help text>
  CGUIDialogOK::ShowAndGetInput(CVariant{10043}, CVariant{35055});
}
