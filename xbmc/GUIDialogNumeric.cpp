#include "stdafx.h"
#include "GUIDialogNumeric.h"
#include "util.h"

CGUIDialogNumeric::CGUIDialogNumeric(void)
:CGUIDialog(0)
{
  m_bConfirmed=false;
  m_bCanceled=false;
  CStdStringW m_strUserInput = L"";
  CStdStringW m_strPassword = L"";
  int m_iRetries = 0;
  m_bUserInputCleanup = true;
  m_bHideInputChars = true;
}

CGUIDialogNumeric::~CGUIDialogNumeric(void)
{
}

void CGUIDialogNumeric::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU || action.wID == ACTION_PARENT_DIR)
  {
    m_bConfirmed=false;
    m_bCanceled=true;
    m_strUserInput=L"";
    m_bHideInputChars = true;
    Close();
    return;
  }
  else if  (action.m_dwButtonCode == KEY_BUTTON_START || action.wID == ACTION_MUSIC_PLAY)
  {
    m_bConfirmed=false;
    m_bCanceled=false;
    if (m_strUserInput!=m_strPassword)
    {
      // incorrect password entered
      m_iRetries--;

      // don't clean up if the calling code wants the bad user input
      if (m_bUserInputCleanup)
        m_strUserInput=L"";
      else
        m_bUserInputCleanup = true;

      m_bHideInputChars = true;
      Close();
      return;
    }

    // correct password entered
    m_bConfirmed=true;
    m_iRetries=0;
    m_strUserInput=L"";
    m_bHideInputChars = true;
    Close();
    return;
  }
  else if (action.wID >= REMOTE_0 && action.wID <= REMOTE_9)
  {
    m_strUserInput+=(char)action.wID - 10;
    if (!m_bHideInputChars)
    {
      SetLine(2, m_strUserInput);
    }
    else
    {
			CStdStringW strHiddenInput(m_strUserInput);
			for (int i = 0; i < (int)strHiddenInput.size(); i++)
			{
				strHiddenInput[i] = m_cHideInputChar;
			}
			SetLine(2, strHiddenInput);
    }
  }
  else
  {
    CGUIDialog::OnAction(action);
  }
}

bool CGUIDialogNumeric::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {            
      m_bConfirmed=false;
      m_bCanceled=false;
      m_cHideInputChar = g_localizeStrings.Get(12322).c_str()[0];
      CGUIDialog::OnMessage(message);
/*
      // Borrowing strUserInput for the next if/else statement
      if (0 < m_iRetries)
      {
        CStdStringW strLine;
        strLine.Format(L"%s %i %s", g_localizeStrings.Get(12342).c_str(), m_iRetries, g_localizeStrings.Get(12343).c_str());
        SetLine(2, strLine);
      }
      else
      {
        SetLine(2, L"");
      }
      m_strUserInput = "";
*/
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      int iAction=message.GetParam1();
      m_bConfirmed=false;
      m_bCanceled=false;
      if (1||ACTION_SELECT_ITEM==iAction)
      {

        if (9 < iControl && 20 > iControl)  // User numeric entry via dialog button UI
        {
          m_strUserInput+=(char)iControl + 38;
          if (!m_bHideInputChars)
          {
            SetLine(2, m_strUserInput);
          }
          else
          {
            CStdStringW strHiddenInput(m_strUserInput);
            for (int i = 0; i < (int)strHiddenInput.size(); i++)
            {
              strHiddenInput[i] = m_cHideInputChar;
            }
            SetLine(2, strHiddenInput);
          }
          return true;
        }
        if (iControl==20)  // Cancel
        {
          m_bCanceled=true;
          m_strUserInput=L"";
          m_bHideInputChars = true;
          Close();
          return true;
        }
        if (iControl==21)  // OK, user submits password
        {
          if (m_strUserInput!=m_strPassword)
          {
            // incorrect password entered
            m_iRetries--;

            // don't clean up if the calling code wants the bad user input
            if (m_bUserInputCleanup)
              m_strUserInput=L"";
            else
              m_bUserInputCleanup = true;

            m_bHideInputChars = true;
            Close();
            return true;
          }

          // correct password entered
          m_bConfirmed=true;
          m_iRetries=0;
          m_strUserInput=L"";
          m_bHideInputChars = true;
          Close();
          return true;
        }
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

// \brief Show numeric keypad and replace aTextString with user input.
// \param aTextString String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param bHideUserInput Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndGetInput(CStdStringW& aTextString, const CStdStringW &dlgHeading, bool bHideUserInput)
{  
  // Prompt user for input
  CStdStringW strUserInput = L"";
  if (ShowAndVerifyInput(strUserInput, dlgHeading, aTextString, L"", L"", true, bHideUserInput))
  {
    // user entry was blank
    return false;
  }

  if (L"" == strUserInput)
    // user canceled out
    return false;


  // We should have a string to return
  aTextString = strUserInput;
  return true;
}

// \brief Show numeric keypad twice to get and confirm a user-entered password string.
// \param strNewPassword String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndGetNewPassword(CStdStringW& strNewPassword)
{
  // Prompt user for password input
  CStdStringW strUserInput = L"";
  if (ShowAndVerifyInput(strUserInput, "12340", "12328", "12329", L"", true, true))
  {
    // TODO: Show error to user saying the password entry was blank
    return false;
  }

  if (L"" == strUserInput)
    // user canceled out
    return false;

  // Prompt again for password input, this time sending previous input as the password to verify
  if (!ShowAndVerifyInput(strUserInput, "12341", "12328", "12329", L"", false, true))
  {
    // TODO: Show error to user saying the password re-entry failed
    return false;
  }

  // password entry and re-entry succeeded
  strNewPassword = strUserInput;
  return true;
}

// \brief Show numeric keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogNumeric::ShowAndVerifyPassword(CStdStringW& strPassword, const CStdStringW& dlgHeading, 
                                           const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, 
                                           const CStdStringW& dlgLine2)
{
  // make a copy of strPassword to prevent from overwriting it later
  CStdStringW strPassTemp = strPassword;

  if (ShowAndVerifyInput(strPassTemp, dlgHeading, dlgLine0, dlgLine1, dlgLine2, true, true))
  {
    // user entered correct password
    return 0;
  }

  if (L"" == strPassTemp)
    // user canceled out
    return -1;

  // user must have entered an incorrect password
  return 1;
}

// \brief Show numeric keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogNumeric::ShowAndVerifyPassword(CStdStringW& strPassword, const CStdStringW& dlgHeading, int iRetries)
{
  CStdStringW strLine2 = L"";
  if (0 < iRetries)
  {
    // Show a string telling user they have iRetries retries left
    strLine2.Format(L"%s %i %s", g_localizeStrings.Get(12342).c_str(), iRetries, g_localizeStrings.Get(12343).c_str());
  }
  return ShowAndVerifyPassword(strPassword, dlgHeading, "12328", "12329", strLine2);
}

// \brief Show numeric keypad and verify user input against strToVerify.
// \param strToVerify Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \param bGetUserInput If set as true and return=true, strToVerify is overwritten with user input string.
// \param bHideInputChars Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndVerifyInput(CStdStringW& strToVerify, const CStdStringW& dlgHeading, 
                                           const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, 
                                           const CStdStringW& dlgLine2, bool bGetUserInput, bool bHideInputChars)
{
  // Prompt user for password input
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  pDialog->m_strPassword = strToVerify;
  pDialog->m_bUserInputCleanup = !bGetUserInput;
  pDialog->m_bHideInputChars = bHideInputChars;

  // HACK: This won't work if the label specified is actually a positive numeric value, but that's very unlikely
  if (!CUtil::IsNaturalNumber(dlgHeading))
    pDialog->SetHeading( dlgHeading );
  else
    pDialog->SetHeading( _wtoi(dlgHeading.c_str()) );

  if (!CUtil::IsNaturalNumber(dlgLine0))
    pDialog->SetLine( 0, dlgLine0 );
  else
    pDialog->SetLine( 0, _wtoi(dlgLine0.c_str()) );

  if (!CUtil::IsNaturalNumber(dlgLine1))
    pDialog->SetLine( 1, dlgLine1 );
  else
    pDialog->SetLine( 1, _wtoi(dlgLine1.c_str()) );

  if (!CUtil::IsNaturalNumber(dlgLine2))
    pDialog->SetLine( 2, dlgLine2 );
  else
    pDialog->SetLine( 2, _wtoi(dlgLine2.c_str()) );

  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

  if (bGetUserInput)
  {
    strToVerify = pDialog->m_strUserInput;
    pDialog->m_strUserInput = L"";
  }

  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
  {
    // user canceled out or entered an incorrect password
    return false;
  }

  // user entered correct password
  return true;
}

bool CGUIDialogNumeric::IsConfirmed() const
{
  return m_bConfirmed;
}

bool CGUIDialogNumeric::IsCanceled() const
{
  return m_bCanceled;
}

void  CGUIDialogNumeric::SetHeading(const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void  CGUIDialogNumeric::SetHeading(const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogNumeric::SetLine(int iLine, const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogNumeric::SetLine(int iLine, const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
  msg.SetLabel(strLine);
  OnMessage(msg);
}
void CGUIDialogNumeric::SetHeading(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
  msg.SetLabel(iString);
  OnMessage(msg);
}


void	CGUIDialogNumeric::SetLine(int iLine, int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
  msg.SetLabel(iString);
  OnMessage(msg);
}
