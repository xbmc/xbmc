
#include "stdafx.h"
#include "guidialogGamepad.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"
#include "util.h"

CGUIDialogGamepad::CGUIDialogGamepad(void)
:CGUIDialog(0)
{
	m_bConfirmed=false;
    m_bCanceled=false;
    CStdStringW m_strUserInput = L"";
    CStdStringW m_strPassword = L"";
    int m_iRetries = 0;
    m_bUserInputCleanup = true;
}

CGUIDialogGamepad::~CGUIDialogGamepad(void)
{
}

void CGUIDialogGamepad::OnAction(const CAction &action)
{
    	
    if  ((action.m_dwButtonCode >= KEY_BUTTON_A && 
                action.m_dwButtonCode <= KEY_BUTTON_RIGHT_TRIGGER) || 
                (action.m_dwButtonCode >= KEY_BUTTON_DPAD_UP && 
                action.m_dwButtonCode <= KEY_BUTTON_DPAD_RIGHT))
    {
        switch (action.m_dwButtonCode)
        {
        case KEY_BUTTON_A               :   m_strUserInput+="A";    break;
        case KEY_BUTTON_B               :   m_strUserInput+="B";    break;
        case KEY_BUTTON_X               :   m_strUserInput+="X";    break;
        case KEY_BUTTON_Y               :   m_strUserInput+="Y";    break;
        case KEY_BUTTON_BLACK           :   m_strUserInput+="K";    break;
        case KEY_BUTTON_WHITE           :   m_strUserInput+="W";    break;
        case KEY_BUTTON_LEFT_TRIGGER    :   m_strUserInput+="(";    break;
        case KEY_BUTTON_RIGHT_TRIGGER   :   m_strUserInput+=")";    break;
        case KEY_BUTTON_DPAD_UP         :   m_strUserInput+="U";    break;
        case KEY_BUTTON_DPAD_DOWN       :   m_strUserInput+="D";    break;
        case KEY_BUTTON_DPAD_LEFT       :   m_strUserInput+="L";    break;
        case KEY_BUTTON_DPAD_RIGHT      :   m_strUserInput+="R";    break;
        default                         :   return;                 break;
        }
        
        wchar_t* strHiddenInput = new wchar_t[wcslen(m_strUserInput)];
        wcscpy(strHiddenInput, m_strUserInput);
        for (int i = 0; i < (int)wcslen(m_strUserInput); i++)
        {
            strHiddenInput[i] = (char)m_cHideInputChar;
        }
        SetLine(2, strHiddenInput);
        return;
    }
    else if (action.m_dwButtonCode == KEY_BUTTON_BACK || action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU || action.wID == ACTION_PARENT_DIR)
	{
        m_bConfirmed=false;
        m_bCanceled=true;
        m_strUserInput=L"";
    m_bHideInputChars = true;
        Close();
        return;
	}
    else if  (action.m_dwButtonCode == KEY_BUTTON_START)
    {
        m_bConfirmed=false;
        m_bCanceled=false;
        if (m_strUserInput!=m_strPassword.ToUpper())
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
	}
	else
	{
		CGUIDialog::OnAction(action);
	}
}

bool CGUIDialogGamepad::OnMessage(CGUIMessage& message)
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
                // nothing to do, there are no selectable controls in this dialog
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

// \brief Show gamepad keypad and replace aTextString with user input.
// \param aTextString String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param bHideUserInput Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndGetInput(CStdStringW& aTextString, const CStdStringW &strHeading, bool bHideUserInput)
{  
  // Prompt user for input
  CStdStringW strUserInput = L"";
  if (ShowAndVerifyInput(strUserInput, strHeading, aTextString, L"", L"", true, bHideUserInput))
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

// \brief Show gamepad keypad twice to get and confirm a user-entered password string.
// \param strNewPassword String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndGetNewPassword(CStdStringW& strNewPassword)
{
  // Prompt user for password input
  CStdStringW strUserInput = L"";
  if (ShowAndVerifyInput(strUserInput, "12340", "12330", "12331", L"", true, true))
  {
    // TODO: Show error to user saying the password entry was blank
    return false;
  }

  if (L"" == strUserInput)
    // user canceled out
    return false;

  // Prompt again for password input, this time sending previous input as the password to verify
  if (!ShowAndVerifyInput(strUserInput, "12341", "12330", "12331", L"", false, true))
  {
    // TODO: Show error to user saying the password re-entry failed
    return false;
  }

  // password entry and re-entry succeeded
  strNewPassword = strUserInput;
  return true;
}

// \brief Show gamepad keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogGamepad::ShowAndVerifyPassword(CStdStringW& strPassword, const CStdStringW& strHeading, 
                                           const CStdStringW& strLine0, const CStdStringW& strLine1, 
                                           const CStdStringW& strLine2)
{
  // make a copy of strPassword to prevent from overwriting it later
  CStdStringW strPassTemp = strPassword;

  if (ShowAndVerifyInput(strPassTemp, strHeading, strLine0, strLine1, strLine2, true, true))
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

// \brief Show gamepad keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogGamepad::ShowAndVerifyPassword(CStdStringW& strPassword, const CStdStringW& strHeading, int iRetries)
{
  CStdStringW strLine2 = L"";
  if (0 < iRetries)
  {
    // Show a string telling user they have iRetries retries left
    strLine2.Format(L"%s %i %s", g_localizeStrings.Get(12342).c_str(), iRetries, g_localizeStrings.Get(12343).c_str());
  }
  return ShowAndVerifyPassword(strPassword, strHeading, "12330", "12331", strLine2);
}

// \brief Show gamepad keypad and verify user input against strToVerify.
// \param strToVerify Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \param bGetUserInput If set as true and return=true, strToVerify is overwritten with user input string.
// \param bHideInputChars Masks user input as asterisks if set as true.  Currently not yet implemented.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogGamepad::ShowAndVerifyInput(CStdStringW& strToVerify, const CStdStringW& strHeading, 
                                           const CStdStringW& strLine0, const CStdStringW& strLine1, 
                                           const CStdStringW& strLine2, bool bGetUserInput, bool bHideInputChars)
{
  // Prompt user for password input
  CGUIDialogGamepad *pDialog = (CGUIDialogGamepad *)m_gWindowManager.GetWindow(WINDOW_DIALOG_GAMEPAD);
  pDialog->m_strPassword = strToVerify;
  pDialog->m_bUserInputCleanup = !bGetUserInput;
  pDialog->m_bHideInputChars = bHideInputChars;

  // HACK: This won't work if the label specified is actually a positive numeric value, but that's very unlikely
  if (!CUtil::IsNaturalNumber(strHeading))
    pDialog->SetHeading( strHeading );
  else
    pDialog->SetHeading( _wtoi(strHeading) );

  if (!CUtil::IsNaturalNumber(strLine0))
    pDialog->SetLine( 0, strLine0 );
  else
    pDialog->SetLine( 0, _wtoi(strLine0.c_str()) );

  if (!CUtil::IsNaturalNumber(strLine1))
    pDialog->SetLine( 1, strLine1 );
  else
    pDialog->SetLine( 1, _wtoi(strLine1.c_str()) );

  if (!CUtil::IsNaturalNumber(strLine2))
    pDialog->SetLine( 2, strLine2 );
  else
    pDialog->SetLine( 2, _wtoi(strLine2.c_str()) );

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

bool CGUIDialogGamepad::IsConfirmed() const
{
	return m_bConfirmed;
}

bool CGUIDialogGamepad::IsCanceled() const
{
	return m_bCanceled;
}

void  CGUIDialogGamepad::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogGamepad::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogGamepad::SetLine(int iLine, const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogGamepad::SetLine(int iLine, const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}
void CGUIDialogGamepad::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(iString);
	OnMessage(msg);
}


void	CGUIDialogGamepad::SetLine(int iLine, int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(iString);
	OnMessage(msg);
}
