/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIDialogSeekBar.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "utils/SeekHandler.h"

#define POPUP_SEEK_PROGRESS     401
#define POPUP_SEEK_LABEL        402

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
    : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekBar.xml")
{
  m_loadType = LOAD_ON_GUI_INIT;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void)
{
}

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);

  case GUI_MSG_LABEL_SET:
    if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_SEEK_LABEL)
      return CGUIDialog::OnMessage(message);
    break;

  case GUI_MSG_ITEM_SELECT:
    if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_SEEK_PROGRESS)
      return CGUIDialog::OnMessage(message);
    break;
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::FrameMove()
{
  if (!g_application.m_pPlayer->HasPlayer())
  {
    Close(true);
    return;
  }

  // update controls
  if (!CSeekHandler::Get().InProgress() && g_infoManager.GetTotalPlayTime())
  { // position the bar at our current time
    CONTROL_SELECT_ITEM(POPUP_SEEK_PROGRESS, (unsigned int)(g_infoManager.GetPlayTime()/g_infoManager.GetTotalPlayTime() * 0.1f));
    SET_CONTROL_LABEL(POPUP_SEEK_LABEL, g_infoManager.GetCurrentPlayTime());
  }
  else
  {
    CONTROL_SELECT_ITEM(POPUP_SEEK_PROGRESS, (unsigned int)g_infoManager.GetSeekPercent());
    SET_CONTROL_LABEL(POPUP_SEEK_LABEL, g_infoManager.GetCurrentSeekTime());
  }

  CGUIDialog::FrameMove();
}
