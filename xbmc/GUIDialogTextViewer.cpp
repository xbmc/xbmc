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
#include "GUIUserMessages.h"

#define CONTROL_HEADING  1
#define CONTROL_TEXTAREA 5

CGUIDialogTextViewer::CGUIDialogTextViewer(void)
    : CGUIDialog(WINDOW_DIALOG_TEXT_VIEWER, "DialogTextViewer.xml")
{}

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

