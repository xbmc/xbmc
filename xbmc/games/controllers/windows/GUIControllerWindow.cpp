/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIControllerWindow.h"

#include "GUIControllerDefines.h"
#include "GUIControllerList.h"
#include "GUIFeatureList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIWindowAddonBrowser.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "games/controllers/dialogs/ControllerInstaller.h"
#include "games/controllers/dialogs/GUIDialogIgnoreInput.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"

// To enable button mapping support
#include "peripherals/Peripherals.h"

using namespace KODI;
using namespace GAME;

CGUIControllerWindow::CGUIControllerWindow(void)
  : CGUIDialog(WINDOW_DIALOG_GAME_CONTROLLERS, "DialogGameControllers.xml"),
    m_installer(new CControllerInstaller)
{
  // initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

CGUIControllerWindow::~CGUIControllerWindow(void)
{
  delete m_controllerList;
  delete m_featureList;
}

void CGUIControllerWindow::DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  /*
   * Apply the faded focus texture to the current controller when unfocused
   */

  CGUIControl* control = nullptr; // The controller button
  bool bAlphaFaded = false; // True if the controller button has been focused and faded this frame

  if (m_controllerList && m_controllerList->GetFocusedController() >= 0)
  {
    control = GetFirstFocusableControl(CONTROL_CONTROLLER_BUTTONS_START +
                                       m_controllerList->GetFocusedController());
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
      m_controllerId = message.GetStringParam();
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
      else if (controlId == CONTROL_GET_ALL)
      {
        GetAllControllers();
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
      else if (CONTROL_CONTROLLER_BUTTONS_START <= controlId &&
               controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerSelected(controlId - CONTROL_CONTROLLER_BUTTONS_START);
        bHandled = true;
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId &&
               controlId < CONTROL_FEATURE_BUTTONS_END)
      {
        OnFeatureSelected(controlId - CONTROL_FEATURE_BUTTONS_START);
        bHandled = true;
      }
      break;
    }
    case GUI_MSG_FOCUSED:
    {
      int controlId = message.GetControlId();

      if (CONTROL_CONTROLLER_BUTTONS_START <= controlId &&
          controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerFocused(controlId - CONTROL_CONTROLLER_BUTTONS_START);
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId &&
               controlId < CONTROL_FEATURE_BUTTONS_END)
      {
        OnFeatureFocused(controlId - CONTROL_FEATURE_BUTTONS_START);
      }
      break;
    }
    case GUI_MSG_SETFOCUS:
    {
      int controlId = message.GetControlId();

      if (CONTROL_CONTROLLER_BUTTONS_START <= controlId &&
          controlId < CONTROL_CONTROLLER_BUTTONS_END)
      {
        OnControllerFocused(controlId - CONTROL_CONTROLLER_BUTTONS_START);
      }
      else if (CONTROL_FEATURE_BUTTONS_START <= controlId &&
               controlId < CONTROL_FEATURE_BUTTONS_END)
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
        const std::string controllerId = message.GetStringParam();
        if (m_controllerList && m_controllerList->Refresh(controllerId))
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

void CGUIControllerWindow::OnEvent(const ADDON::AddonEvent& event)
{
  using namespace ADDON;

  if (typeid(event) == typeid(AddonEvents::Enabled) || // also called on install,
      typeid(event) == typeid(AddonEvents::Disabled) || // not called on uninstall
      typeid(event) == typeid(AddonEvents::UnInstalled) ||
      typeid(event) == typeid(AddonEvents::ReInstalled))
  {
    if (CServiceBroker::GetAddonMgr().HasType(event.addonId, AddonType::GAME_CONTROLLER))
    {
      UpdateButtons();
    }
  }
}

void CGUIControllerWindow::OnInitWindow(void)
{
  // Get active game add-on
  GameClientPtr gameClient;
  {
    auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    if (gameSettingsHandle)
    {
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon,
                                                 ADDON::AddonType::GAMEDLL,
                                                 ADDON::OnlyEnabled::CHOICE_YES))
        gameClient = std::static_pointer_cast<CGameClient>(addon);
    }
  }
  m_gameClient = std::move(gameClient);

  CGUIDialog::OnInitWindow();

  if (!m_featureList)
  {
    m_featureList = new CGUIFeatureList(this, m_gameClient);
    if (!m_featureList->Initialize())
    {
      delete m_featureList;
      m_featureList = nullptr;
    }
  }

  if (!m_controllerList && m_featureList)
  {
    m_controllerList = new CGUIControllerList(this, m_featureList, m_gameClient, m_controllerId);
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
  CServiceBroker::GetPeripherals().EnableButtonMapping();

  UpdateButtons();

  // subscribe to events
  CServiceBroker::GetRepositoryUpdater().Events().Subscribe(this, &CGUIControllerWindow::OnEvent);
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CGUIControllerWindow::OnEvent);
}

void CGUIControllerWindow::OnDeinitWindow(int nextWindowID)
{
  CServiceBroker::GetRepositoryUpdater().Events().Unsubscribe(this);
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);

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

  m_gameClient.reset();
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

void CGUIControllerWindow::OnFeatureFocused(unsigned int buttonIndex)
{
  if (m_featureList)
    m_featureList->OnFocus(buttonIndex);
}

void CGUIControllerWindow::OnFeatureSelected(unsigned int buttonIndex)
{
  if (m_featureList)
    m_featureList->OnSelect(buttonIndex);
}

void CGUIControllerWindow::UpdateButtons(void)
{
  using namespace ADDON;

  VECADDONS addons;
  if (m_gameClient)
  {
    SET_CONTROL_HIDDEN(CONTROL_GET_MORE);
    SET_CONTROL_HIDDEN(CONTROL_GET_ALL);
  }
  else
  {
    const bool bEnable = CServiceBroker::GetAddonMgr().GetInstallableAddons(
                             addons, ADDON::AddonType::GAME_CONTROLLER) &&
                         !addons.empty();
    CONTROL_ENABLE_ON_CONDITION(CONTROL_GET_MORE, bEnable);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_GET_ALL, bEnable);
  }
}

void CGUIControllerWindow::GetMoreControllers(void)
{
  std::string strAddonId;
  if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::AddonType::GAME_CONTROLLER, strAddonId, false,
                                            true, false, true, false) < 0)
  {
    // "Controller profiles"
    // "All available controller profiles are installed."
    MESSAGING::HELPERS::ShowOKDialogText(CVariant{35050}, CVariant{35062});
    return;
  }
}

void CGUIControllerWindow::GetAllControllers()
{
  if (m_installer->IsRunning())
    return;

  m_installer->Create(false);
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
  MESSAGING::HELPERS::ShowOKDialogText(CVariant{10043}, CVariant{35055});
}

void CGUIControllerWindow::ShowButtonCaptureDialog(void)
{
  CGUIDialogIgnoreInput dialog;
  dialog.Show();
}
