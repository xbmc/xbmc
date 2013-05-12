//
//  GUIDialogMyPlexPin.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-14.
//  Copyright 2012 Plex Inc. All rights reserved.
//


#define ID_DESC_TEXT      10081
#define ID_PIN_NR         10082
#define ID_USERNAME       10083
#define ID_PASSWORD       10084
#define ID_BUTTON_CANCEL  10085
#define ID_BUTTON_MANUAL  10086
#define ID_BUTTON_SUBMIT  10087
#define ID_ICON_SUCCESS   10088
#define ID_ICON_ERROR     10089

#include "GUIDialogMyPlex.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIButtonControl.h"
#include "LocalizeStrings.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "GUIWindowManager.h"

void
CGUIDialogMyPlex::ShowAndGetInput()
{
  CGUIDialogMyPlex *dialog = (CGUIDialogMyPlex *)g_windowManager.GetWindow(WINDOW_DIALOG_MYPLEX_PIN);
  if (!dialog) return;

  dialog->DoModal();
  return ;
}

void
CGUIDialogMyPlex::DoModal()
{
  SET_CONTROL_HIDDEN(ID_ICON_SUCCESS);
  SET_CONTROL_HIDDEN(ID_ICON_ERROR);

  CGUIDialog::DoModal();
}

bool
CGUIDialogMyPlex::OnMessage(CGUIMessage &message)
{
  return CGUIDialog::OnMessage(message);
}
