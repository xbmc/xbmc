/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSeekBar.h"

#include "Application.h"
#include "GUIInfoManager.h"
#include "SeekHandler.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

#include <math.h>

#define POPUP_SEEK_PROGRESS           401
#define POPUP_SEEK_EPG_EVENT_PROGRESS 402
#define POPUP_SEEK_TIMESHIFT_PROGRESS 403

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
  : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekBar.xml", DialogModalityType::MODELESS)
{
  m_loadType = LOAD_ON_GUI_INIT;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void) = default;

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);
  case GUI_MSG_ITEM_SELECT:
    if (message.GetSenderId() == GetID() &&
        (message.GetControlId() == POPUP_SEEK_PROGRESS ||
         message.GetControlId() == POPUP_SEEK_EPG_EVENT_PROGRESS ||
         message.GetControlId() == POPUP_SEEK_TIMESHIFT_PROGRESS))
      return CGUIDialog::OnMessage(message);
    break;
  case GUI_MSG_REFRESH_TIMER:
    return CGUIDialog::OnMessage(message);
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::FrameMove()
{
  if (!g_application.GetAppPlayer().HasPlayer())
  {
    Close(true);
    return;
  }

  int progress = GetProgress();
  if (progress != m_lastProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_PROGRESS, m_lastProgress = progress);

  int epgEventProgress = GetEpgEventProgress();
  if (epgEventProgress != m_lastEpgEventProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_EPG_EVENT_PROGRESS, m_lastEpgEventProgress = epgEventProgress);

  int timeshiftProgress = GetTimeshiftProgress();
  if (timeshiftProgress != m_lastTimeshiftProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_TIMESHIFT_PROGRESS, m_lastTimeshiftProgress = timeshiftProgress);

  CGUIDialog::FrameMove();
}

int CGUIDialogSeekBar::GetProgress() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;

  if (g_application.GetAppPlayer().GetSeekHandler().GetSeekSize() != 0)
    infoMgr.GetInt(progress, PLAYER_SEEKBAR);
  else
    infoMgr.GetInt(progress, PLAYER_PROGRESS);

  return progress;
}

int CGUIDialogSeekBar::GetEpgEventProgress() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;
  infoMgr.GetInt(progress, PVR_EPG_EVENT_PROGRESS);

  int seekSize = g_application.GetAppPlayer().GetSeekHandler().GetSeekSize();
  if (seekSize != 0)
  {
    int total = 0;
    infoMgr.GetInt(total, PVR_EPG_EVENT_DURATION);

    float totalTime = static_cast<float>(total);
    if (totalTime == 0.0f)
      return 0;

    float percentPerSecond = 100.0f / totalTime;
    float percent = progress + percentPerSecond * seekSize;
    percent = std::max(0.0f, std::min(percent, 100.0f));
    return std::lrintf(percent);
  }

  return progress;
}

int CGUIDialogSeekBar::GetTimeshiftProgress() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;
  infoMgr.GetInt(progress, PVR_TIMESHIFT_SEEKBAR);

  return progress;
}
