/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "GUIDialogTextViewer.h"
#include "GUIUserMessages.h"

#define CONTROL_HEADING  1
#define CONTROL_TEXTAREA 5

CGUIDialogTextViewer::CGUIDialogTextViewer(void)
    : CGUIDialog(WINDOW_DIALOG_TEXT_VIEWER, "DialogTextViewer.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogTextViewer::~CGUIDialogTextViewer(void)
{}

bool CGUIDialogTextViewer::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogTextViewer::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      SetHeading();
      SetText();
      return true;
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UPDATE)
      {
        SetText();
        SetHeading();
        return true;
      }
    }
    break;
  default:
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTextViewer::SetText()
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
  msg.SetLabel(m_strText);
  OnMessage(msg);
}

void CGUIDialogTextViewer::SetHeading()
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING);
  msg.SetLabel(m_strHeading);
  OnMessage(msg);
}

void CGUIDialogTextViewer::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // reset text area
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TEXTAREA);
  OnMessage(msgReset);

  // reset heading
  SET_CONTROL_LABEL(CONTROL_HEADING, "");
}
