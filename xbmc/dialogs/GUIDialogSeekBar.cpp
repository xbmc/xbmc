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

#ifndef DIALOGS_GUIDIALOGSEEKBAR_H_INCLUDED
#define DIALOGS_GUIDIALOGSEEKBAR_H_INCLUDED
#include "GUIDialogSeekBar.h"
#endif

#ifndef DIALOGS_GUILIB_GUISLIDERCONTROL_H_INCLUDED
#define DIALOGS_GUILIB_GUISLIDERCONTROL_H_INCLUDED
#include "guilib/GUISliderControl.h"
#endif

#ifndef DIALOGS_APPLICATION_H_INCLUDED
#define DIALOGS_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef DIALOGS_GUIINFOMANAGER_H_INCLUDED
#define DIALOGS_GUIINFOMANAGER_H_INCLUDED
#include "GUIInfoManager.h"
#endif

#ifndef DIALOGS_UTILS_TIMEUTILS_H_INCLUDED
#define DIALOGS_UTILS_TIMEUTILS_H_INCLUDED
#include "utils/TimeUtils.h"
#endif

#ifndef DIALOGS_FILEITEM_H_INCLUDED
#define DIALOGS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef DIALOGS_UTILS_SEEKHANDLER_H_INCLUDED
#define DIALOGS_UTILS_SEEKHANDLER_H_INCLUDED
#include "utils/SeekHandler.h"
#endif


#define SEEK_BAR_DISPLAY_TIME 2000L
#define SEEK_BAR_SEEK_TIME     500L

#define POPUP_SEEK_SLIDER       401
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
    {
      if (message.GetSenderId() == GetID() && message.GetControlId() == POPUP_SEEK_LABEL)
        CGUIDialog::OnMessage(message);
    }
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
  if (!g_application.GetSeekHandler()->InProgress() && !g_infoManager.m_performingSeek)
  { // position the bar at our current time
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
    if (pSlider && g_infoManager.GetTotalPlayTime())
      pSlider->SetPercentage((float)g_infoManager.GetPlayTime()/g_infoManager.GetTotalPlayTime() * 0.1f);

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(g_infoManager.GetCurrentPlayTime());
    OnMessage(msg);
  }
  else
  {
    CGUISliderControl *pSlider = (CGUISliderControl*)GetControl(POPUP_SEEK_SLIDER);
    if (pSlider)
      pSlider->SetPercentage(g_application.GetSeekHandler()->GetPercent());

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), POPUP_SEEK_LABEL);
    msg.SetLabel(g_infoManager.GetCurrentSeekTime());
    OnMessage(msg);
  }

  CGUIDialog::FrameMove();
}
