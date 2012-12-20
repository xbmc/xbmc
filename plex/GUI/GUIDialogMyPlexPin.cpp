//
//  GUIDialogMyPlexPin.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-14.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#define ID_BUTTON_OK   10

#include "GUIDialogMyPlexPin.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIButtonControl.h"
#include "LocalizeStrings.h"
#include "MyPlexManager.h"

void
CGUIDialogMyPlexPin::ShowAndGetInput()
{
  CGUIDialogMyPlexPin *dialog = (CGUIDialogMyPlexPin *)g_windowManager.GetWindow(WINDOW_DIALOG_MYPLEX_PIN);
  if (!dialog) return;

  //dialog->SetButtonText(g_localizeStrings.Get(222));
  dialog->SetHeading(g_localizeStrings.Get(44100));
  dialog->SetLine(0, g_localizeStrings.Get(44101));
  CStdString line;
  line.Format("%s %s", g_localizeStrings.Get(44103), g_localizeStrings.Get(44102));
  dialog->SetLine(2, line);
  dialog->m_pinLogin.Create();

  dialog->DoModal();
  return ;
}

int CGUIDialogMyPlexPin::GetDefaultLabelID(int controlId) const
{
  if (controlId == ID_BUTTON_OK)
    return 222;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}

void
CGUIDialogMyPlexPin::SetButtonText(const CStdString& text)
{
  CGUIButtonControl * ctrl = (CGUIButtonControl*)GetControl(ID_BUTTON_OK);
  if (ctrl) {
    ctrl->SetLabel(text);
  }
}

bool
CGUIDialogMyPlexPin::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == ID_BUTTON_OK)
    {
      if (!m_done)
        m_pinLogin.StopThread(false);

      Close();
      return true;
    }
  }
  if (message.GetMessage() == GUI_MSG_MYPLEX_GOT_PIN)
  {
    CStdString line;
    line.Format("%s %s", g_localizeStrings.Get(44103), m_pinLogin.m_pin);
    SetLine(2, line);
    return true;
  }

  else if (message.GetMessage() == GUI_MSG_MYPLEX_GOT_TOKEN)
  {
    SetLine(0, g_localizeStrings.Get(44104));
    SetLine(2, "");
    SetButtonText(g_localizeStrings.Get(186));
    SetInvalid();

    m_done = true;

    return true;
  }

  return CGUIDialogBoxBase::OnMessage(message);
}
