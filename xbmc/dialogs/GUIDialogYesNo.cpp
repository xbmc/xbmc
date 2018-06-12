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

#include "GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/helpers/DialogHelper.h"
#include "ServiceBroker.h"

CGUIDialogYesNo::CGUIDialogYesNo(int overrideId /* = -1 */)
    : CGUIDialogBoxBase(overrideId == -1 ? WINDOW_DIALOG_YES_NO : overrideId, "DialogConfirm.xml")
{
  m_bConfirmed = false;
  m_bCanceled = false;
  m_bCustom = false;
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
      if (1 || ACTION_SELECT_ITEM == iAction)
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
  SET_CONTROL_FOCUS(CONTROL_NO_BUTTON, 0);

  CGUIDialogBoxBase::OnInitWindow();
}

bool CGUIDialogYesNo::ShowAndGetInput(CVariant heading, CVariant line0, CVariant line1, CVariant line2, bool &bCanceled)
{
  return ShowAndGetInput(heading, line0, line1, line2, bCanceled, "", "", NO_TIMEOUT);
}

bool CGUIDialogYesNo::ShowAndGetInput(CVariant heading, CVariant line0, CVariant line1, CVariant line2, CVariant noLabel /* = "" */, CVariant yesLabel /* = "" */)
{
  bool bDummy(false);
  return ShowAndGetInput(heading, line0, line1, line2, bDummy, noLabel, yesLabel, NO_TIMEOUT);
}

bool CGUIDialogYesNo::ShowAndGetInput(CVariant heading, CVariant line0, CVariant line1, CVariant line2, bool &bCanceled, CVariant noLabel, CVariant yesLabel, unsigned int autoCloseTime)
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
  dialog->Open();

  bCanceled = dialog->m_bCanceled;
  return (dialog->IsConfirmed()) ? true : false;
}

bool CGUIDialogYesNo::ShowAndGetInput(CVariant heading, CVariant text)
{
  bool bDummy(false);
  return ShowAndGetInput(heading, text, "", "", bDummy);
}

bool CGUIDialogYesNo::ShowAndGetInput(CVariant heading, CVariant text, bool &bCanceled, CVariant noLabel /* = "" */, CVariant yesLabel /* = "" */, unsigned int autoCloseTime)
{
  int result = ShowAndGetInput(heading, text, noLabel, yesLabel, "", autoCloseTime);

  bCanceled = result == -1;
  return result == 1;
}

int CGUIDialogYesNo::ShowAndGetInput(CVariant heading, CVariant text, CVariant noLabel, CVariant yesLabel, CVariant customLabel, unsigned int autoCloseTime)
{
  CGUIDialogYesNo *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (!dialog)
    return false;

  dialog->SetHeading(heading);
  dialog->SetText(text);
  if (autoCloseTime)
    dialog->SetAutoClose(autoCloseTime);
  dialog->m_bCanceled = false;
  dialog->m_bCustom = false;
  dialog->SetChoice(0, !noLabel.empty() ? noLabel : 106);
  dialog->SetChoice(1, !yesLabel.empty() ? yesLabel : 107);
  dialog->SetChoice(2, customLabel);  // Button only visible when label is not empty

  dialog->Open();

  if (dialog->m_bCanceled)
    return -1;
  else if (dialog->m_bCustom)
    return 2;
  else if (dialog->IsConfirmed())
    return 1;
  else
    return 0;
}

int CGUIDialogYesNo::ShowAndGetInput(const KODI::MESSAGING::HELPERS::DialogYesNoMessage& options)
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

  for (size_t i = 0; i < 3; ++i)
  {
    if (!options.lines[i].isNull())
      SetLine(i, options.lines[i]);
  }

  Open();

  if (m_bCanceled)
    return -1;
  else if (m_bCustom)
    return 2;
  else if (IsConfirmed())
    return 1;
  else
    return 0;
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
