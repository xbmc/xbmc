/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogOK.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "utils/Variant.h"

CGUIDialogOK::CGUIDialogOK(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_OK, "DialogConfirm.xml")
{
}

CGUIDialogOK::~CGUIDialogOK(void) = default;

bool CGUIDialogOK::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if (iControl == CONTROL_YES_BUTTON)
    {
      m_bConfirmed = true;
      Close();
      return true;
    }
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogOK::ShowAndGetInput(const CVariant& heading, const CVariant& text)
{
  CGUIDialogOK *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogOK>(WINDOW_DIALOG_OK);
  if (!dialog)
    return false;
  dialog->SetHeading(heading);
  dialog->SetText(text);
  dialog->Open();
  return dialog->IsConfirmed();
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
bool CGUIDialogOK::ShowAndGetInput(const CVariant& heading,
                                   const CVariant& line0,
                                   const CVariant& line1,
                                   const CVariant& line2)
{
  CGUIDialogOK *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogOK>(WINDOW_DIALOG_OK);
  if (!dialog)
    return false;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->Open();
  return dialog->IsConfirmed();
}

bool CGUIDialogOK::ShowAndGetInput(const HELPERS::DialogOKMessage & options)
{
  if (!options.heading.isNull())
    SetHeading(options.heading);
  if (!options.text.isNull())
    SetText(options.text);

  for (size_t i = 0; i < 3; ++i)
  {
    if (!options.lines[i].isNull())
      SetLine(i, options.lines[i]);
  }
  Open();
  return IsConfirmed();
}

void CGUIDialogOK::OnInitWindow()
{
  SET_CONTROL_HIDDEN(CONTROL_NO_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_CUSTOM_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_PROGRESS_BAR);
  SET_CONTROL_FOCUS(CONTROL_YES_BUTTON, 0);

  CGUIDialogBoxBase::OnInitWindow();
}

int CGUIDialogOK::GetDefaultLabelID(int controlId) const
{
  if (controlId == CONTROL_YES_BUTTON)
    return 186;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
