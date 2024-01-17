/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogYesNo.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/ActionIDs.h"
#include "messaging/helpers/DialogHelper.h"

CGUIDialogYesNo::CGUIDialogYesNo(int overrideId /* = -1 */)
    : CGUIDialogBoxBase(overrideId == -1 ? WINDOW_DIALOG_YES_NO : overrideId, "DialogConfirm.xml")
{
  Reset();
}

CGUIDialogYesNo::~CGUIDialogYesNo() = default;

bool CGUIDialogYesNo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      if (true || ACTION_SELECT_ITEM == iAction)
      {
        if (iControl == CONTROL_NO_BUTTON)
        {
          m_bConfirmed = false;
          Close();
          return true;
        }
        if (iControl == CONTROL_YES_BUTTON)
        {
          m_bConfirmed = true;
          Close();
          return true;
        }
        if (iControl == CONTROL_CUSTOM_BUTTON)
        {
          m_bConfirmed = false;
          m_bCustom = true;
          Close();
          return true;
        }
      }
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogYesNo::OnBack(int actionID)
{
  m_bCanceled = true;
  m_bConfirmed = false;
  m_bCustom = false;
  return CGUIDialogBoxBase::OnBack(actionID);
}

void CGUIDialogYesNo::OnInitWindow()
{
  if (!m_strChoices[2].empty())
    SET_CONTROL_VISIBLE(CONTROL_CUSTOM_BUTTON);
  else
    SET_CONTROL_HIDDEN(CONTROL_CUSTOM_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_PROGRESS_BAR);
  SET_CONTROL_FOCUS(m_defaultButtonId, 0);

  CGUIDialogBoxBase::OnInitWindow();
}

bool CGUIDialogYesNo::ShowAndGetInput(const CVariant& heading,
                                      const CVariant& line0,
                                      const CVariant& line1,
                                      const CVariant& line2,
                                      bool& bCanceled)
{
  return ShowAndGetInput(heading, line0, line1, line2, bCanceled, "", "", NO_TIMEOUT);
}

bool CGUIDialogYesNo::ShowAndGetInput(const CVariant& heading,
                                      const CVariant& line0,
                                      const CVariant& line1,
                                      const CVariant& line2,
                                      const CVariant& noLabel /* = "" */,
                                      const CVariant& yesLabel /* = "" */)
{
  bool bDummy(false);
  return ShowAndGetInput(heading, line0, line1, line2, bDummy, noLabel, yesLabel, NO_TIMEOUT);
}

bool CGUIDialogYesNo::ShowAndGetInput(const CVariant& heading,
                                      const CVariant& line0,
                                      const CVariant& line1,
                                      const CVariant& line2,
                                      bool& bCanceled,
                                      const CVariant& noLabel,
                                      const CVariant& yesLabel,
                                      unsigned int autoCloseTime)
{
  CGUIDialogYesNo *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (!dialog)
    return false;

  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  if (autoCloseTime)
    dialog->SetAutoClose(autoCloseTime);
  dialog->SetChoice(0, !noLabel.empty() ? noLabel : 106);
  dialog->SetChoice(1, !yesLabel.empty() ? yesLabel : 107);
  dialog->SetChoice(2, "");
  dialog->m_bCanceled = false;
  dialog->m_defaultButtonId = CONTROL_NO_BUTTON;
  dialog->Open();

  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}

bool CGUIDialogYesNo::ShowAndGetInput(const CVariant& heading, const CVariant& text)
{
  bool bDummy(false);
  return ShowAndGetInput(heading, text, "", "", bDummy);
}

bool CGUIDialogYesNo::ShowAndGetInput(const CVariant& heading,
                                      const CVariant& text,
                                      bool& bCanceled,
                                      const CVariant& noLabel /* = "" */,
                                      const CVariant& yesLabel /* = "" */,
                                      unsigned int autoCloseTime,
                                      int defaultButtonId /* = CONTROL_NO_BUTTON */)
{
  const DialogResult result =
      ShowAndGetInput(heading, text, noLabel, yesLabel, "", autoCloseTime, defaultButtonId);

  bCanceled = result == DIALOG_RESULT_CANCEL;
  return result == 1;
}

void CGUIDialogYesNo::Reset()
{
  m_bConfirmed = false;
  m_bCanceled = false;
  m_bCustom = false;
  m_bAutoClosed = false;
  m_defaultButtonId = CONTROL_NO_BUTTON;
}

CGUIDialogYesNo::DialogResult CGUIDialogYesNo::GetResult() const
{
  if (m_bCanceled)
    return DIALOG_RESULT_CANCEL;
  else if (m_bCustom)
    return DIALOG_RESULT_CUSTOM;
  else if (IsConfirmed())
    return DIALOG_RESULT_YES;
  else
    return DIALOG_RESULT_NO;
}

CGUIDialogYesNo::DialogResult CGUIDialogYesNo::ShowAndGetInput(
    const CVariant& heading,
    const CVariant& text,
    const CVariant& noLabel,
    const CVariant& yesLabel,
    const CVariant& customLabel,
    unsigned int autoCloseTime,
    int defaultButtonId /* = CONTROL_NO_BUTTON */)
{
  CGUIDialogYesNo *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (!dialog)
    return DIALOG_RESULT_CANCEL;

  dialog->SetHeading(heading);
  dialog->SetText(text);
  if (autoCloseTime > 0)
    dialog->SetAutoClose(autoCloseTime);
  dialog->m_bCanceled = false;
  dialog->m_bCustom = false;
  dialog->m_defaultButtonId = defaultButtonId;
  dialog->SetChoice(0, !noLabel.empty() ? noLabel : 106);
  dialog->SetChoice(1, !yesLabel.empty() ? yesLabel : 107);
  dialog->SetChoice(2, customLabel);  // Button only visible when label is not empty
  dialog->Open();

  return dialog->GetResult();
}

CGUIDialogYesNo::DialogResult CGUIDialogYesNo::ShowAndGetInput(
    const KODI::MESSAGING::HELPERS::DialogYesNoMessage& options)
{
  //Set default yes/no labels, these might be overwritten further down if specified
  //by the caller
  SetChoice(0, 106);
  SetChoice(1, 107);
  SetChoice(2, "");
  if (!options.heading.isNull())
    SetHeading(options.heading);
  if (!options.text.isNull())
    SetText(options.text);
  if (!options.noLabel.isNull())
    SetChoice(0, options.noLabel);
  if (!options.yesLabel.isNull())
    SetChoice(1, options.yesLabel);
  if (!options.customLabel.isNull())
    SetChoice(2, options.customLabel);
  if (options.autoclose > 0)
    SetAutoClose(options.autoclose);
  m_bCanceled = false;
  m_bCustom = false;
  m_defaultButtonId = CONTROL_NO_BUTTON;

  for (size_t i = 0; i < 3; ++i)
  {
    if (!options.lines[i].isNull())
      SetLine(i, options.lines[i]);
  }

  Open();

  return GetResult();
}

int CGUIDialogYesNo::GetDefaultLabelID(int controlId) const
{
  if (controlId == CONTROL_NO_BUTTON)
    return 106;
  else if (controlId == CONTROL_YES_BUTTON)
    return 107;
  else if (controlId == CONTROL_CUSTOM_BUTTON)
    return -1;
  return CGUIDialogBoxBase::GetDefaultLabelID(controlId);
}
