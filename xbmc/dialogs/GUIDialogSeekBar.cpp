/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <math.h>

#include "GUIDialogSeekBar.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "SeekHandler.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

#define POPUP_SEEK_PROGRESS           401
#define POPUP_SEEK_EPG_EVENT_PROGRESS 402

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
        (message.GetControlId() == POPUP_SEEK_PROGRESS || message.GetControlId() == POPUP_SEEK_EPG_EVENT_PROGRESS))
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

  unsigned int percent = g_application.GetAppPlayer().GetSeekHandler().InProgress()
    ? std::lrintf(GetSeekPercent())
    : std::lrintf(g_application.GetPercentage());

  if (percent != m_lastPercent)
    CONTROL_SELECT_ITEM(POPUP_SEEK_PROGRESS, m_lastPercent = percent);

  unsigned int epgEventPercent = g_application.GetAppPlayer().GetSeekHandler().InProgress()
    ? GetEpgEventSeekPercent()
    : GetEpgEventProgress();

  if (epgEventPercent != m_lastEpgEventPercent)
    CONTROL_SELECT_ITEM(POPUP_SEEK_EPG_EVENT_PROGRESS, m_lastEpgEventPercent = epgEventPercent);

  CGUIDialog::FrameMove();
}

float CGUIDialogSeekBar::GetSeekPercent() const
{
  int totaltime = std::lrint(g_application.GetTotalTime());
  if (totaltime == 0)
    return 0.0f;

  float percentPlayTime = static_cast<float>(std::lrint(g_application.GetTime() * 1000)) / totaltime * 0.1f;
  float percentPerSecond = 100.0f / static_cast<float>(totaltime);
  float percent = percentPlayTime + percentPerSecond * g_application.GetAppPlayer().GetSeekHandler().GetSeekSize();

  if (percent > 100.0f)
    percent = 100.0f;
  if (percent < 0.0f)
    percent = 0.0f;

  return percent;
}

int CGUIDialogSeekBar::GetEpgEventProgress() const
{
  int value = 0;
  CServiceBroker::GetGUI()->GetInfoManager().GetInt(value, PVR_EPG_EVENT_PROGRESS);
  return value;
}

int CGUIDialogSeekBar::GetEpgEventSeekPercent() const
{
  int seekSize = g_application.GetAppPlayer().GetSeekHandler().GetSeekSize();
  if (seekSize != 0)
  {
    CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

    int progress = 0;
    infoMgr.GetInt(progress, PVR_EPG_EVENT_PROGRESS);

    int total = 0;
    infoMgr.GetInt(total, PVR_EPG_EVENT_DURATION);

    float totalTime = static_cast<float>(total);
    float percentPerSecond = 100.0f / totalTime;
    float percent = progress + percentPerSecond * seekSize;
    return std::lrintf(percent);
  }
  else
  {
    return GetEpgEventProgress();
  }
}
