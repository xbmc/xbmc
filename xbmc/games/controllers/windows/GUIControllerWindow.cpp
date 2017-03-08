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
#include "games/controllers/dialogs/GUIDialogIgnoreInput.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "ServiceBroker.h"

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

void CGUIControllerWindow::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  /*
   * Apply the faded focus texture to the current controller when unfocused
   */

  CGUIControl* control = nullptr; // The controller button
  bool bAlphaFaded = false; // True if the controller button has been focused and faded this frame

  if (m_controllerList && m_controllerList->GetFocusedController() >= 0)
  {
    control = GetFirstFocusableControl(CONTROL_CONTROLLER_BUTTONS_START + m_controllerList->GetFocusedController());
    if (control && !control->HasFocus())
    {
      if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      {
        control->SetFocus(true);
        static_cast<CGUIButtonControl*>(control)->SetAlpha(0x80);
        bAlphaFaded = true;
      }
    }
  }

  CGUIDialog::DoProcess(currentTime, dirtyregions);

  if (control && bAlphaFaded)
  {
    control->SetFocus(false);
    if (control->GetControlType() == CGUIControl::GUICONTROL_BUTTON)
      static_cast<CGUIButtonControl*>(control)->SetAlpha(0xFF);
  }
}

bool CGUIControllerWindow::OnMessage(CGUIMessage& message)
{
  // Set to true to block the call to the super class
  bool bHandled = false;

  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      //! @todo Process params here, don't just record them for later
      m_param = message.GetStringParam();
      break;
    }
    case GUI_MSG_CLICKED:
    {
      int controlId = message.GetSenderId();

      if (controlId == CONTROL_CLOSE_BUTTON)
      {
        Close();
        bHandled = true;
      }
      else if (controlId == CONTROL_GET_MORE)
      {
        GetMoreControllers();
        bHandled = true;
      }
      else if (controlId == CONTROL_RESET_BUTTON)
      {
        ResetController();
        bHandled = true;
      }
      else if (controlId == CONTROL_HELP_BUTTON)
      {
        ShowHelp();
        bHandled = true;
      }
      else if (controlId == CONTROL_FIX_SKIPPING)
      {
        ShowButtonCaptureDialog();
      }
      else if (CONTROL_CONTROLLER_BUTTONS_START <= controlId && controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerSelected(controlId - CONTROL_CONTROLLER_BUTTONS_START);
        bHandled = true;
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId && controlId < CONTROL_FEATURE_BUTTONS_END)
      {
        OnFeatureSelected(controlId - CONTROL_FEATURE_BUTTONS_START);
        bHandled = true;
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
          bHandled = true;
        }
      }
      break;
    }
    default:
      break;
  }

  if (!bHandled)
    bHandled = CGUIDialog::OnMessage(message);

  return bHandled;
}

void CGUIControllerWindow::OnEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event)
{
  UpdateButtons();
}

void CGUIControllerWindow::OnInitWindow(void)
{
  CGUIDialog::OnInitWindow();

  if (!m_featureList)
  {
    m_featureList = new CGUIFeatureList(this, m_param);
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

  // Enable button mapping support
  if (!CServiceBroker::GetPeripherals().EnableButtonMapping())
    CLog::Log(LOGDEBUG, "Joystick support not found");

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

void CGUIControllerWindow::ShowButtonCaptureDialog(void)
{
  CGUIDialogIgnoreInput dialog;
  dialog.Show();
}
