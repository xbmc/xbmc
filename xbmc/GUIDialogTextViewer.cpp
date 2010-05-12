/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogTextViewer.h"

#define CONTROL_HEADING  1
#define CONTROL_TEXTAREA 5

CGUIDialogTextViewer::CGUIDialogTextViewer(void)
    : CGUIDialog(WINDOW_DIALOG_TEXT_VIEWER, "DialogTextViewer.xml")
{}

CGUIDialogTextViewer::~CGUIDialogTextViewer(void)
{}

bool CGUIDialogTextViewer::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    // erase debug screen
    m_strInfo.clear();
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
    msg.SetLabel(m_strInfo);
    OnMessage(msg);
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogTextViewer::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
      msg.SetLabel(m_strInfo);
      msg.SetParam1(5); // 5 pages max
      OnMessage(msg);
      CGUIMessage msg2(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING);
      msg2.SetLabel(m_strHeading);
      OnMessage(msg2);
      return true;
    }
    break;

  case GUI_MSG_USER:
    {
      m_strInfo += message.GetLabel();
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
      msg.SetLabel(m_strInfo);
      msg.SetParam1(5); // 5 pages max
      OnMessage(msg);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

