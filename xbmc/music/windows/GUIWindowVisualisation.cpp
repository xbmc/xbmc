/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowVisualisation.h"

#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

using namespace KODI;
using namespace MUSIC_INFO;

#define START_FADE_LENGTH  2.0f // 2 seconds on startup

#define CONTROL_VIS          2

CGUIWindowVisualisation::CGUIWindowVisualisation(void)
  : CGUIWindow(WINDOW_VISUALISATION, "MusicVisualisation.xml")
{
  m_bShowPreset = false;
  m_loadType = KEEP_IN_MEMORY;
}

bool CGUIWindowVisualisation::OnAction(const CAction &action)
{
  bool passToVis = false;
  switch (action.GetID())
  {
  case ACTION_VIS_PRESET_NEXT:
  case ACTION_VIS_PRESET_PREV:
  case ACTION_VIS_PRESET_RANDOM:
  case ACTION_VIS_RATE_PRESET_PLUS:
  case ACTION_VIS_RATE_PRESET_MINUS:
    passToVis = true;
    break;

  case ACTION_SHOW_INFO:
    {
      m_initTimer.Stop();
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_MYMUSIC_SONGTHUMBINVIS,
                                            CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().ToggleShowInfo());
      return true;
    }
    break;

  case ACTION_SHOW_OSD:
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_MUSIC_OSD);
    return true;

  case ACTION_SHOW_GUI:
    // save the settings
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
    return true;
    break;

  case ACTION_VIS_PRESET_LOCK:
    { // show the locked icon + fall through so that the vis handles the locking
      if (!m_bShowPreset)
      {
        m_lockedTimer.StartZero();
      }
      passToVis = true;
    }
    break;
  case ACTION_VIS_PRESET_SHOW:
    {
      if (!m_lockedTimer.IsRunning() || m_bShowPreset)
        m_bShowPreset = !m_bShowPreset;
      return true;
    }
    break;

  case ACTION_DECREASE_RATING:
  case ACTION_INCREASE_RATING:
    {
      // actual action is taken care of in CApplication::OnAction()
      m_initTimer.StartZero();
      CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);
    }
    break;
    //! @todo These should be mapped to its own function - at the moment it's overriding
    //! the global action of fastforward/rewind and OSD.
/*  case KEY_BUTTON_Y:
    g_application.m_CdgParser.Pause();
    return true;
    break;

    case ACTION_ANALOG_FORWARD:
    // calculate the speed based on the amount the button is held down
    if (action.GetAmount())
    {
      float AVDelay = g_application.m_CdgParser.GetAVDelay();
      g_application.m_CdgParser.SetAVDelay(AVDelay - action.GetAmount() / 4.0f);
      return true;
    }
    break;*/
  }

  if (passToVis)
  {
    CGUIControl *control = GetControl(CONTROL_VIS);
    if (control)
      return control->OnAction(action);
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowVisualisation::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_GET_VISUALISATION:
  case GUI_MSG_VISUALISATION_RELOAD:
  case GUI_MSG_PLAYBACK_STARTED:
    {
      CGUIControl *control = GetControl(CONTROL_VIS);
      if (control)
        return control->OnMessage(message);
    }
    break;
  case GUI_MSG_VISUALISATION_ACTION:
  {
    CAction action(message.GetParam1());
    return OnAction(action);
  }
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (IsActive()) // save any changed settings from the OSD
        CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      // close all active modal dialogs
      CServiceBroker::GetGUI()->GetWindowManager().CloseInternalModalDialogs(true);
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing music
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      if (message.GetParam1() == WINDOW_INVALID && !appPlayer->IsPlayingAudio())
      { // why are we here if nothing is playing???
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
        return true;
      }

      // hide or show the preset button(s)
      CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();
      infoMgr.GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);  // always show the info initially.
      CGUIWindow::OnMessage(message);
      if (infoMgr.GetCurrentSongTag())
        m_tag = *infoMgr.GetCurrentSongTag();

      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYMUSIC_SONGTHUMBINVIS))
      { // always on
        m_initTimer.Stop();
      }
      else
      {
        // start display init timer (fade out after 3 secs...)
        m_initTimer.StartZero();
      }
      return true;
    }
  }
  return CGUIWindow::OnMessage(message);
}

EVENT_RESULT CGUIWindowVisualisation::OnMouseEvent(const CPoint& point,
                                                   const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // no control found to absorb this click - go back to GUI
    OnAction(CAction(ACTION_SHOW_GUI));
    return EVENT_RESULT_HANDLED;
  }
  if (event.m_id == ACTION_GESTURE_NOTIFY)
    return EVENT_RESULT_UNHANDLED;
  if (event.m_id != ACTION_MOUSE_MOVE || event.m_offsetX || event.m_offsetY)
  { // some other mouse action has occurred - bring up the OSD
    CGUIDialog *pOSD = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_MUSIC_OSD);
    if (pOSD)
    {
      pOSD->SetAutoClose(3000);
      pOSD->Open();
    }
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUIWindowVisualisation::FrameMove()
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  // check for a tag change
  const CMusicInfoTag* tag = infoMgr.GetCurrentSongTag();
  if (tag && *tag != m_tag)
  { // need to fade in then out again
    m_tag = *tag;
    // fade in
    m_initTimer.StartZero();
    infoMgr.GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(true);
  }
  if (m_initTimer.IsRunning() && m_initTimer.GetElapsedSeconds() > (float)CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_songInfoDuration)
  {
    m_initTimer.Stop();
    if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYMUSIC_SONGTHUMBINVIS))
    { // reached end of fade in, fade out again
      infoMgr.GetInfoProviders().GetPlayerInfoProvider().SetShowInfo(false);
    }
  }
  // show or hide the locked texture
  if (m_lockedTimer.IsRunning() && m_lockedTimer.GetElapsedSeconds() > START_FADE_LENGTH)
  {
    m_lockedTimer.Stop();
  }
  CGUIWindow::FrameMove();
}
