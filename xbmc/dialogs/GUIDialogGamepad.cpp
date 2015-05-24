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

#include "GUIDialogGamepad.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"

CGUIDialogGamepad::CGUIDialogGamepad(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_GAMEPAD, "DialogGamepad.xml")
{
  m_bCanceled = false;
  m_iRetries = 0;
  m_bUserInputCleanup = true;
  m_bHideInputChars = true;
  m_cHideInputChar = '*';
}

CGUIDialogGamepad::~CGUIDialogGamepad(void)
{}

bool CGUIDialogGamepad::OnAction(const CAction &action)
{
  if ((action.GetButtonCode() >= KEY_BUTTON_A &&
       action.GetButtonCode() <= KEY_BUTTON_RIGHT_TRIGGER) ||
      (action.GetButtonCode() >= KEY_BUTTON_DPAD_UP &&
       action.GetButtonCode() <= KEY_BUTTON_DPAD_RIGHT) ||
      (action.GetID() >= ACTION_MOVE_LEFT &&
       action.GetID() <= ACTION_MOVE_DOWN) ||
      action.GetID() == ACTION_PLAYER_PLAY
     )
  {
    switch (action.GetButtonCode())
    {
    case KEY_BUTTON_A : m_strUserInput += "A"; break;
    case KEY_BUTTON_B : m_strUserInput += "B"; break;
    case KEY_BUTTON_X : m_strUserInput += "X"; break;
    case KEY_BUTTON_Y : m_strUserInput += "Y"; break;
    case KEY_BUTTON_BLACK : m_strUserInput += "K"; break;
    case KEY_BUTTON_WHITE : m_strUserInput += "W"; break;
    case KEY_BUTTON_LEFT_TRIGGER : m_strUserInput += "("; break;
    case KEY_BUTTON_RIGHT_TRIGGER : m_strUserInput += ")"; break;
    case KEY_BUTTON_DPAD_UP : m_strUserInput += "U"; break;
    case KEY_BUTTON_DPAD_DOWN : m_strUserInput += "D"; break;
    case KEY_BUTTON_DPAD_LEFT : m_strUserInput += "L"; break;
    case KEY_BUTTON_DPAD_RIGHT : m_strUserInput += "R"; break;
    default:
      switch (action.GetID())
      {
        case ACTION_MOVE_LEFT:   m_strUserInput += "L"; break;
        case ACTION_MOVE_RIGHT:  m_strUserInput += "R"; break;
        case ACTION_MOVE_UP:     m_strUserInput += "U"; break;
        case ACTION_MOVE_DOWN:   m_strUserInput += "D"; break;
        case ACTION_PLAYER_PLAY: m_strUserInput += "P"; break;
        default:
          return true;
      }
      break;
    }

    std::string strHiddenInput(m_strUserInput);
    for (int i = 0; i < (int)strHiddenInput.size(); i++)
    {
      strHiddenInput[i] = m_cHideInputChar;
    }
    SetLine(2, strHiddenInput);
    return true;
  }
  else if (action.GetButtonCode() == KEY_BUTTON_BACK || action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_NAV_BACK)
  {
    m_bConfirmed = false;
    m_bCanceled = true;
    m_strUserInput = "";
    m_bHideInputChars = true;
    Close();
    return true;
  }
  else if (action.GetButtonCode() == KEY_BUTTON_START || action.GetID() == ACTION_SELECT_ITEM)
  {
    m_bConfirmed = false;
    m_bCanceled = false;

    std::string md5pword2 = XBMC::XBMC_MD5::GetMD5(m_strUserInput);

    if (!StringUtils::EqualsNoCase(m_strPassword, md5pword2))
    {
      // incorrect password entered
      m_iRetries--;

      // don't clean up if the calling code wants the bad user input
      if (m_bUserInputCleanup)
        m_strUserInput = "";
      else
        m_bUserInputCleanup = true;

      m_bHideInputChars = true;
      Close();
      return true;
    }

    // correct password entered
    m_bConfirmed = true;
    m_iRetries = 0;
    m_strUserInput = "";
    m_bHideInputChars = true;
    Close();
    return true;
  }
  else if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
  {
    return true; // unhandled
  }
  else
  {
    return CGUIDialog::OnAction(action);
  }
}

bool CGUIDialogGamepad::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_bConfirmed = false;
      m_bCanceled = false;
      m_cHideInputChar = g_localizeStrings.Get(12322).c_str()[0];
      CGUIDialog::OnMessage(message);
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      m_bConfirmed = false;
      m_bCanceled = false;
    }
    break;
  }
  return CGUIDialogBoxBase::OnMessage(message);
}

// \brief Show gamepad keypad and replace aTextString with user input.
// \param aTextString String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param bHideUserInput Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndGetInput(std::string& aTextString, const std::string &dlgHeading, bool bHideUserInput)
{
  // Prompt user for input
  std::string strUserInput;
  if (ShowAndVerifyInput(strUserInput, dlgHeading, aTextString, "", "", true, bHideUserInput))
  {
    // user entry was blank
    return false;
  }

  if (strUserInput.empty())
    // user canceled out
    return false;


  // We should have a string to return
  aTextString = strUserInput;
  return true;
}

// \brief Show gamepad keypad twice to get and confirm a user-entered password string.
// \param strNewPassword String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsuccessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndVerifyNewPassword(std::string& strNewPassword)
{
  // Prompt user for password input
  std::string strUserInput;
  if (ShowAndVerifyInput(strUserInput, "12340", "12330", "12331", "", true, true))
  {
    // TODO: Show error to user saying the password entry was blank
    CGUIDialogOK::ShowAndGetInput(12357, 12358); // Password is empty/blank
    return false;
  }

  if (strUserInput.empty())
    // user canceled out
    return false;

  // Prompt again for password input, this time sending previous input as the password to verify
  if (!ShowAndVerifyInput(strUserInput, "12341", "12330", "12331", "", false, true))
  {
    // TODO: Show error to user saying the password re-entry failed
    CGUIDialogOK::ShowAndGetInput(12357, 12344); // Password do not match
    return false;
  }

  // password entry and re-entry succeeded
  strNewPassword = strUserInput;
  return true;
}

// \brief Show gamepad keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsuccessful input. -1 if no user input or canceled editing.
int CGUIDialogGamepad::ShowAndVerifyPassword(std::string& strPassword, const std::string& dlgHeading, int iRetries)
{
  std::string strLine2;
  if (0 < iRetries)
  {
    // Show a string telling user they have iRetries retries left
    strLine2 = StringUtils::Format("%s %i %s", g_localizeStrings.Get(12342).c_str(), iRetries, g_localizeStrings.Get(12343).c_str());
  }

  // make a copy of strPassword to prevent from overwriting it later
  std::string strPassTemp = strPassword;
  if (ShowAndVerifyInput(strPassTemp, dlgHeading, g_localizeStrings.Get(12330), g_localizeStrings.Get(12331), strLine2, true, true))
  {
    // user entered correct password
    return 0;
  }

  if (strPassTemp.empty())
    // user canceled out
    return -1;

  // user must have entered an incorrect password
  return 1;
}

// \brief Show gamepad keypad and verify user input against strToVerify.
// \param strToVerify Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \param bGetUserInput If set as true and return=true, strToVerify is overwritten with user input string.
// \param bHideInputChars Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsuccessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndVerifyInput(std::string& strToVerify, const std::string& dlgHeading,
    const std::string& dlgLine0, const std::string& dlgLine1,
    const std::string& dlgLine2, bool bGetUserInput, bool bHideInputChars)
{
  // Prompt user for password input
  CGUIDialogGamepad *pDialog = (CGUIDialogGamepad *)g_windowManager.GetWindow(WINDOW_DIALOG_GAMEPAD);
  pDialog->m_strPassword = strToVerify;
  pDialog->m_bUserInputCleanup = !bGetUserInput;
  pDialog->m_bHideInputChars = bHideInputChars;

  // HACK: This won't work if the label specified is actually a positive numeric value, but that's very unlikely
  if (!StringUtils::IsNaturalNumber(dlgHeading))
    pDialog->SetHeading( dlgHeading );
  else
    pDialog->SetHeading( atoi(dlgHeading.c_str()) );

  if (!StringUtils::IsNaturalNumber(dlgLine0))
    pDialog->SetLine( 0, dlgLine0 );
  else
    pDialog->SetLine( 0, atoi(dlgLine0.c_str()) );

  if (!StringUtils::IsNaturalNumber(dlgLine1))
    pDialog->SetLine( 1, dlgLine1 );
  else
    pDialog->SetLine( 1, atoi(dlgLine1.c_str()) );

  if (!StringUtils::IsNaturalNumber(dlgLine2))
    pDialog->SetLine( 2, dlgLine2 );
  else
    pDialog->SetLine( 2, atoi(dlgLine2.c_str()) );

  g_audioManager.Enable(false); // dont do sounds during pwd input
  pDialog->DoModal();
  g_audioManager.Enable(true);

  if (bGetUserInput && !pDialog->IsCanceled())
  {
    strToVerify = XBMC::XBMC_MD5::GetMD5(pDialog->m_strUserInput);
    StringUtils::ToLower(strToVerify);
    pDialog->m_strUserInput = "";
  }

  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
  {
    // user canceled out or entered an incorrect password
    return false;
  }

  // user entered correct password
  return true;
}

bool CGUIDialogGamepad::IsCanceled() const
{
  return m_bCanceled;
}

