
#include "stdafx.h"
#include "GUIDialogKeyboard.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "Util.h"


// TODO: Add support for symbols.

// Symbol mapping (based on MS virtual keyboard - may need improving)
static char symbol_map[37] = "!@#$%^&*()[]{}-_=+;:\'\",.<>/?\\|`~    ";

#define CTL_BUTTON_DONE  300
#define CTL_BUTTON_CANCEL 301
#define CTL_BUTTON_SHIFT 302
#define CTL_BUTTON_CAPS  303
#define CTL_BUTTON_SYMBOLS 304
#define CTL_BUTTON_LEFT  305
#define CTL_BUTTON_RIGHT 306

#define CTL_LABEL_EDIT  310
#define CTL_LABEL_HEADING 311

#define CTL_BUTTON_BACKSPACE  8

CGUIDialogKeyboard::CGUIDialogKeyboard(void)
: CGUIDialog(WINDOW_DIALOG_KEYBOARD, "DialogKeyboard.xml")
{
  m_bIsConfirmed = false;
  m_bShift = false;
  m_hiddenInput = false;
  m_keyType = LOWER;
  m_strHeading = "";
}

CGUIDialogKeyboard::~CGUIDialogKeyboard(void)
{}

void CGUIDialogKeyboard::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_bIsConfirmed = false;

  // set alphabetic (capitals)
  UpdateButtons();

  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    pEdit->ShowCursor();
  }

  // set heading
  if (!m_strHeading.IsEmpty())
  {
    SET_CONTROL_LABEL(CTL_LABEL_HEADING, m_strHeading);
    SET_CONTROL_VISIBLE(CTL_LABEL_HEADING);
  }
  else
  {
    SET_CONTROL_HIDDEN(CTL_LABEL_HEADING);
  }
}

bool CGUIDialogKeyboard::OnAction(const CAction &action)
{
  if (action.wID == ACTION_BACKSPACE)
  {
    Backspace();
    return true;
  }
  else if (action.wID == ACTION_CURSOR_LEFT)
  {
    MoveCursor( -1);
    return true;
  }
  else if (action.wID == ACTION_CURSOR_RIGHT)
  {
    MoveCursor(1);
    return true;
  }
  else if (action.wID == ACTION_SHIFT)
  {
    OnShift();
    return true;
  }
  else if (action.wID == ACTION_SYMBOLS)
  {
    OnSymbols();
    return true;
  }
  else if (action.wID >= REMOTE_0 && action.wID <= REMOTE_9)
  {
    OnRemoteNumberClick(action.wID);
    return true;
  }
  else if (action.wID >= KEY_VKEY && action.wID < KEY_ASCII)
  { // input from the keyboard (vkey, not ascii)
    BYTE b = action.wID & 0xFF;
    if (b == 0x25) MoveCursor( -1); // left
    if (b == 0x27) MoveCursor(1);  // right
    return true;
  }
  else if (action.wID >= KEY_ASCII)
  { // input from the keyboard
    char ch = action.wID & 0xFF;
    switch (ch)
    {
    case 10:  // enter
      {
        m_bIsConfirmed = true;
        Close();
      }
      break;
    case 8:   // backspace or delete??
      Backspace();
      break;
    case 27:  // escape
      Close();
      break;
    default:  //use character input
      Character((WCHAR)ch);
      break;
    }
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogKeyboard::OnMessage(CGUIMessage& message)
{
  CGUIDialog::OnMessage(message);

  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      switch (iControl)
      {
      case CTL_BUTTON_DONE:
        {
          m_bIsConfirmed = true;
          Close();
          break;
        }
      case CTL_BUTTON_CANCEL:
        Close();
        break;
      case CTL_BUTTON_SHIFT:
        OnShift();
        break;
      case CTL_BUTTON_CAPS:
        if (m_keyType == LOWER)
          m_keyType = CAPS;
        else if (m_keyType == CAPS)
          m_keyType = LOWER;
        UpdateButtons();
        break;
      case CTL_BUTTON_SYMBOLS:
        OnSymbols();
        break;
      case CTL_BUTTON_LEFT:
        {
          MoveCursor( -1);
        }
        break;
      case CTL_BUTTON_RIGHT:
        {
          MoveCursor(1);
        }
        break;
      default:
        m_lastRemoteKeyClicked = 0;
        OnClickButton(iControl);
        break;
      }
    }
    break;
  }

  return true;
}

void CGUIDialogKeyboard::SetText(CStdString& aTextString)
{
  m_strEdit = aTextString;
  UpdateLabel();
  MoveCursor(m_strEdit.size());
}

void CGUIDialogKeyboard::Character(WCHAR wch)
{
  m_strEdit.Insert(GetCursorPos(), (TCHAR)wch);
  UpdateLabel();
  MoveCursor(1);
}

void CGUIDialogKeyboard::Render()
{
  // reset the hide state of the label when the remote
  // sms style input times out
  if (m_lastRemoteClickTime + 1000 < timeGetTime())
  {
    UpdateLabel();
  }
  CGUIDialog::Render();
}

void CGUIDialogKeyboard::UpdateLabel()
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    CStdString edit = m_strEdit;
    if (m_hiddenInput)
    { // convert to *'s
      edit.Empty();
      if (m_lastRemoteClickTime + 1000 > timeGetTime() && m_strEdit.size())
      { // using the remove to input, so display the last key input
        edit.append(m_strEdit.size() - 1, '*');
        edit.append(1, m_strEdit[m_strEdit.size() - 1]);
      }
      else
        edit.append(m_strEdit.size(), '*');
    }
    pEdit->SetText(edit);
  }
}

void CGUIDialogKeyboard::Backspace()
{
  int iPos = GetCursorPos();
  if (iPos > 0)
  {
    m_strEdit.erase(iPos - 1, 1);
    UpdateLabel();
    MoveCursor(-1);
  }
}

void CGUIDialogKeyboard::OnClickButton(int iButtonControl)
{
  if ( ((iButtonControl >= 48) && (iButtonControl <= 57)) || (iButtonControl == 32))
  { // number or space
    Character(GetCharacter(iButtonControl));
  }
  else if ((iButtonControl >= 65) && (iButtonControl <= 90))
  { // letter
    Character(GetCharacter(iButtonControl));
  }
  else if (iButtonControl == CTL_BUTTON_BACKSPACE)
  {
    Backspace();
  }
}

void CGUIDialogKeyboard::OnRemoteNumberClick(int key)
{
  DWORD now = timeGetTime();

  if (key != m_lastRemoteKeyClicked || (key == m_lastRemoteKeyClicked && m_lastRemoteClickTime + 1000 < now))
  {
    m_lastRemoteKeyClicked = key;
    m_indexInSeries = 0;
  }
  else
  {
    m_indexInSeries++;
    Backspace();
  }

  int arrayIndex = key - REMOTE_0;
  m_indexInSeries = m_indexInSeries % strlen(s_charsSeries[arrayIndex]);
  m_lastRemoteClickTime = now;

  // Select the character that will be pressed
  const char* characterPressed = s_charsSeries[arrayIndex];
  characterPressed += m_indexInSeries;

  // use caps where appropriate
  WCHAR ch = (WCHAR) * characterPressed;
  bool caps = (m_keyType == CAPS && !m_bShift) || (m_keyType == LOWER && m_bShift);
  if (!caps && *characterPressed >= 'A' && *characterPressed <= 'Z')
    ch += 32;
  Character(ch);
}

WCHAR CGUIDialogKeyboard::GetCharacter(int iButton)
{
  if (iButton >= 48 && iButton <= 57)
  {
    if (m_keyType == SYMBOLS)
    {
      OnSymbols();
      return (WCHAR)symbol_map[iButton -48];
    }
    else
      return (WCHAR)iButton;
  }
  if (iButton == 32) // space
    return (WCHAR)iButton;
  if (m_keyType == SYMBOLS)
  { // symbol
    OnSymbols();
    return (WCHAR)symbol_map[iButton -65 + 10];
  }
  if ((m_keyType == CAPS && m_bShift) || (m_keyType == LOWER && !m_bShift))
  { // make lower case
    iButton += 32;
  }
  if (m_bShift)
  { // turn off the shift key
    OnShift();
  }
  return (WCHAR) iButton;
}

void CGUIDialogKeyboard::UpdateButtons()
{
  if (m_bShift)
  { // show the button depressed
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_SHIFT);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_SHIFT);
    OnMessage(msg);
  }
  if (m_keyType == CAPS)
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_CAPS);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_CAPS);
    OnMessage(msg);
  }
  if (m_keyType == SYMBOLS)
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_SYMBOLS);
    OnMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_SYMBOLS);
    OnMessage(msg);
  }
  char szLabel[2];
  szLabel[0] = 32;
  szLabel[1] = 0;
  CStdStringW aLabel = szLabel;

  // set numerals
  for (int iButton = 48; iButton <= 57; iButton++)
  {
    CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(iButton));

    if (pButton)
    {
      if (m_keyType == SYMBOLS)
        aLabel[0] = (WCHAR)symbol_map[iButton - 48];
      else
        aLabel[0] = iButton;
      pButton->SetText(aLabel);
    }
  }

  // set correct alphabet characters...

  for (int iButton = 65; iButton <= 90; iButton++)
  {
    CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(iButton));

    if (pButton)
    {
      // set the correct case...
      if ((m_keyType == CAPS && m_bShift) || (m_keyType == LOWER && !m_bShift))
      { // make lower case
        aLabel[0] = iButton + 32;
      }
      else if (m_keyType == SYMBOLS)
      {
        aLabel[0] = (WCHAR)symbol_map[iButton - 65 + 10];
      }
      else
      {
        aLabel[0] = iButton;
      }
      pButton->SetText(aLabel);
    }
  }
}

// Show keyboard with initial value (aTextString) and replace with result string.
// Returns: true  - successful display and input (empty result may return true or false depending on parameter)
//          false - unsucessful display of the keyboard or cancelled editing
bool CGUIDialogKeyboard::ShowAndGetInput(CStdString& aTextString, const CStdStringW &strHeading, bool allowEmptyResult, bool hiddenInput /* = false */)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->CenterWindow();
  pKeyboard->SetHeading(strHeading);
  pKeyboard->SetHiddenInput(hiddenInput);
  pKeyboard->SetText(aTextString);
  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, m_gWindowManager.GetActiveWindow()};
  g_applicationMessenger.SendMessage(tMsg, true);
  pKeyboard->Close();

  // If have text - update this.
  if (pKeyboard->IsConfirmed())
  {
    aTextString = pKeyboard->GetText();
    if (!allowEmptyResult && aTextString.IsEmpty())
      return false;
    return true;
  }
  else return false;
}

bool CGUIDialogKeyboard::ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult)
{
  return ShowAndGetInput(aTextString, L"", allowEmptyResult) != 0;
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param newPassword Overwritten with user input if return=true.
// \param heading Heading to display
// \param allowEmpty Whether a blank password is valid or not.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogKeyboard::ShowAndGetNewPassword(CStdString& newPassword, const CStdStringW &heading, bool allowEmpty)
{
  // Prompt user for password input
  CStdString userInput = "";
  if (!ShowAndGetInput(userInput, heading, allowEmpty, true))
  { // user cancelled, or invalid input
    return false;
  }
  // success - verify the password
  CStdString checkInput = "";
  if (!ShowAndGetInput(checkInput, g_localizeStrings.Get(12341), allowEmpty, true))
  { // user cancelled, or invalid input
    return false;
  }
  // check the password
  if (checkInput == userInput)
  {
    newPassword = checkInput;
    return true;
  }
  CGUIDialogOK::ShowAndGetInput(12341, 12344, 0, 0);
  return false;
};

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param strNewPassword Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogKeyboard::ShowAndGetNewPassword(CStdString& newPassword)
{
  CStdStringW heading = g_localizeStrings.Get(12340);
  return ShowAndGetNewPassword(newPassword, heading, false);
}

// \brief Show keyboard and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogKeyboard::ShowAndVerifyPassword(CStdString& strPassword, const CStdStringW& strHeading, int iRetries)
{
  CStdStringW strHeadingTemp;
  if (1 > iRetries && strHeading.size())
    strHeadingTemp = strHeading;
  else
    strHeadingTemp.Format(L"%s - %i %s", g_localizeStrings.Get(12326).c_str(), g_stSettings.m_iMasterLockMaxRetry - iRetries, g_localizeStrings.Get(12343).c_str());

  CStdString strUserInput = "";
  if (!ShowAndGetInput(strUserInput, strHeadingTemp, false))
	  return -1; // user canceled out

  if (!strPassword.IsEmpty())
  {
	  if (strUserInput == strPassword)
		  return 0;		// user entered correct password
	  else return 1;	// user must have entered an incorrect password
  }
  else 
  {
	  if (!strUserInput.IsEmpty())
	  {
		  strPassword = strUserInput;
		  return 0; // user entered correct password
	  }
	  else return 1;
  }
}

void CGUIDialogKeyboard::Close(bool forceClose)
{
  // reset the heading (we don't always have this)
  m_strHeading = "";
  // call base class
  CGUIDialog::Close(forceClose);
}

void CGUIDialogKeyboard::MoveCursor(int iAmount)
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    pEdit->SetCursorPos(pEdit->GetCursorPos() + iAmount);
  }
}

int CGUIDialogKeyboard::GetCursorPos() const
{
  const CGUILabelControl* pEdit = (const CGUILabelControl*)GetControl(CTL_LABEL_EDIT);
  if (pEdit)
  {
    return pEdit->GetCursorPos();
  }
  return 0;
}

void CGUIDialogKeyboard::OnSymbols()
{
  if (m_keyType == SYMBOLS)
    m_keyType = LOWER;
  else
    m_keyType = SYMBOLS;
  UpdateButtons();
}

void CGUIDialogKeyboard::OnShift()
{
  m_bShift = !m_bShift;
  UpdateButtons();
}

const char* CGUIDialogKeyboard::s_charsSeries[10] = { " !@#$%^&*()[]{}<>/\\|0", ".,;:\'\"-+_=?`~1", "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9" };
