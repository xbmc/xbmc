/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"
#include "ServiceBroker.h"
#include "GUIComponent.h"
#include "messaging/ApplicationMessenger.h"
#include "LocalizeStrings.h"
#include "GUIKeyboardFactory.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include "dialogs/GUIDialogKeyboardGeneric.h"
#if defined(TARGET_DARWIN_EMBEDDED)
#include "dialogs/GUIDialogKeyboardTouch.h"
#endif

using namespace KODI::MESSAGING;
using KODI::UTILITY::CDigest;

CGUIKeyboard *CGUIKeyboardFactory::g_activeKeyboard = NULL;
FILTERING CGUIKeyboardFactory::m_filtering = FILTERING_NONE;

CGUIKeyboardFactory::CGUIKeyboardFactory(void) = default;

CGUIKeyboardFactory::~CGUIKeyboardFactory(void) = default;

void CGUIKeyboardFactory::keyTypedCB(CGUIKeyboard *ref, const std::string &typedString)
{
  if(ref)
  {
    // send our search message in safe way (only the active window needs it)
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, ref->GetWindowId(), 0);
    switch(m_filtering)
    {
      case FILTERING_SEARCH:
        message.SetParam1(GUI_MSG_SEARCH_UPDATE);
        message.SetStringParam(typedString);
        CApplicationMessenger::GetInstance().SendGUIMessage(message, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
        break;
      case FILTERING_CURRENT:
        message.SetParam1(GUI_MSG_FILTER_ITEMS);
        message.SetStringParam(typedString);
        CApplicationMessenger::GetInstance().SendGUIMessage(message);
        break;
      case FILTERING_NONE:
        break;
    }
    ref->resetAutoCloseTimer();
  }
}

bool CGUIKeyboardFactory::SendTextToActiveKeyboard(const std::string &aTextString, bool closeKeyboard /* = false */)
{
  if (!g_activeKeyboard)
    return false;
  return g_activeKeyboard->SetTextToKeyboard(aTextString, closeKeyboard);
}


// Show keyboard with initial value (aTextString) and replace with result string.
// Returns: true  - successful display and input (empty result may return true or false depending on parameter)
//          false - unsuccessful display of the keyboard or cancelled editing
bool CGUIKeyboardFactory::ShowAndGetInput(std::string& aTextString, CVariant heading, bool allowEmptyResult, bool hiddenInput /* = false */, unsigned int autoCloseMs /* = 0 */)
{
  bool confirmed = false;
  CGUIKeyboard *kb = NULL;
  //heading can be a string or a localization id
  std::string headingStr;
  if (heading.isString())
    headingStr = heading.asString();
  else if (heading.isInteger() && heading.asInteger())
    headingStr = g_localizeStrings.Get((uint32_t)heading.asInteger());

#if defined(TARGET_DARWIN_EMBEDDED)
  kb = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKeyboardTouch>(WINDOW_DIALOG_KEYBOARD_TOUCH);
#else
  kb = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKeyboardGeneric>(WINDOW_DIALOG_KEYBOARD);
#endif

  if (kb)
  {
    g_activeKeyboard = kb;
    kb->startAutoCloseTimer(autoCloseMs);
    confirmed = kb->ShowAndGetInput(keyTypedCB, aTextString, aTextString, headingStr, hiddenInput);
    g_activeKeyboard = NULL;
  }

  if (confirmed)
  {
    if (!allowEmptyResult && aTextString.empty())
      confirmed = false;
  }

  return confirmed;
}

bool CGUIKeyboardFactory::ShowAndGetInput(std::string& aTextString, bool allowEmptyResult, unsigned int autoCloseMs /* = 0 */)
{
  return ShowAndGetInput(aTextString, CVariant{""}, allowEmptyResult, false, autoCloseMs);
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIKeyboardFactory::ShowAndGetNewPassword(std::string& newPassword, CVariant heading, bool allowEmpty, unsigned int autoCloseMs /* = 0 */)
{
  return ShowAndGetInput(newPassword, heading, allowEmpty, true, autoCloseMs);
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIKeyboardFactory::ShowAndGetNewPassword(std::string& newPassword, unsigned int autoCloseMs /* = 0 */)
{
  return ShowAndGetNewPassword(newPassword, 12340, false, autoCloseMs);
}

bool CGUIKeyboardFactory::ShowAndGetFilter(std::string &filter, bool searching, unsigned int autoCloseMs /* = 0 */)
{
  m_filtering = searching ? FILTERING_SEARCH : FILTERING_CURRENT;
  bool ret = ShowAndGetInput(filter, searching ? 16017 : 16028, true, false, autoCloseMs);
  m_filtering = FILTERING_NONE;
  return ret;
}


// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param newPassword Overwritten with user input if return=true.
// \param heading Heading to display
// \param allowEmpty Whether a blank password is valid or not.
// \return true if successful display and user input entry/re-entry. false if unsuccessful display, no user input, or canceled editing.
bool CGUIKeyboardFactory::ShowAndVerifyNewPassword(std::string& newPassword, CVariant heading, bool allowEmpty, unsigned int autoCloseMs /* = 0 */)
{
  // Prompt user for password input
  std::string userInput;
  if (!ShowAndGetInput(userInput, heading, allowEmpty, true, autoCloseMs))
  { // user cancelled, or invalid input
    return false;
  }
  // success - verify the password
  std::string checkInput;
  if (!ShowAndGetInput(checkInput, 12341, allowEmpty, true, autoCloseMs))
  { // user cancelled, or invalid input
    return false;
  }
  // check the password
  if (checkInput == userInput)
  {
    newPassword = CDigest::Calculate(CDigest::Type::MD5, userInput);
    return true;
  }
  HELPERS::ShowOKDialogText(CVariant{12341}, CVariant{12344});
  return false;
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param strNewPassword Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsuccessful display, no user input, or canceled editing.
bool CGUIKeyboardFactory::ShowAndVerifyNewPassword(std::string& newPassword, unsigned int autoCloseMs /* = 0 */)
{
  std::string heading = g_localizeStrings.Get(12340);
  return ShowAndVerifyNewPassword(newPassword, heading, false, autoCloseMs);
}

// \brief Show keyboard and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsuccessful input. -1 if no user input or canceled editing.
int CGUIKeyboardFactory::ShowAndVerifyPassword(std::string& strPassword, const std::string& strHeading, int iRetries, unsigned int autoCloseMs /* = 0 */)
{
  std::string strHeadingTemp;
  if (1 > iRetries && strHeading.size())
    strHeadingTemp = strHeading;
  else
    strHeadingTemp = StringUtils::Format("%s - %i %s",
                                         g_localizeStrings.Get(12326).c_str(),
                                         CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MASTERLOCK_MAXRETRIES) - iRetries,
                                         g_localizeStrings.Get(12343).c_str());

  std::string strUserInput;
  //! @todo GUI Setting to enable disable this feature y/n?
  if (!ShowAndGetInput(strUserInput, strHeadingTemp, false, true, autoCloseMs))  //bool hiddenInput = false/true ?
    return -1; // user canceled out

  if (!strPassword.empty())
  {
    std::string md5pword2 = CDigest::Calculate(CDigest::Type::MD5, strUserInput);
    if (StringUtils::EqualsNoCase(strPassword, md5pword2))
      return 0;     // user entered correct password
    else return 1;  // user must have entered an incorrect password
  }
  else
  {
    if (!strUserInput.empty())
    {
      strPassword = CDigest::Calculate(CDigest::Type::MD5, strUserInput);
      return 0; // user entered correct password
    }
    else return 1;
  }
}

