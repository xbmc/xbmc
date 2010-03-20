/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUISettings.h"
#include "GUIDialogKeyboard.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogOK.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "utils/RegExp.h"
#include "GUIPassword.h"
#include "utils/md5.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "AdvancedSettings.h"
#include "LocalizeStrings.h"

// Symbol mapping (based on MS virtual keyboard - may need improving)
static char symbol_map[37] = ")!@#$%^&*([]{}-_=+;:\'\",.<>/?\\|`~    ";

#define CTL_BUTTON_DONE       300
#define CTL_BUTTON_CANCEL     301
#define CTL_BUTTON_SHIFT      302
#define CTL_BUTTON_CAPS       303
#define CTL_BUTTON_SYMBOLS    304
#define CTL_BUTTON_LEFT       305
#define CTL_BUTTON_RIGHT      306
#define CTL_BUTTON_IP_ADDRESS 307

#define CTL_LABEL_EDIT        310
#define CTL_LABEL_HEADING     311

#define CTL_BUTTON_BACKSPACE    8

static char symbolButtons[] = "._-@/\\";
#define NUM_SYMBOLS sizeof(symbolButtons) - 1

#define SEARCH_DELAY         1000
#define REMOTE_SMS_DELAY     1000

CGUIDialogKeyboard::CGUIDialogKeyboard(void)
: CGUIDialog(WINDOW_DIALOG_KEYBOARD, "DialogKeyboard.xml")
{
  m_bIsConfirmed = false;
  m_bShift = false;
  m_hiddenInput = false;
  m_filtering = FILTERING_NONE;
  m_keyType = LOWER;
  m_strHeading = "";
  m_lastRemoteClickTime = 0;
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
  bool handled(true);
  if (action.GetID() == ACTION_BACKSPACE)
  {
    Backspace();
  }
  else if (action.GetID() == ACTION_ENTER)
  {
    OnOK();
  }
  else if (action.GetID() == ACTION_CURSOR_LEFT)
  {
    MoveCursor( -1);
  }
  else if (action.GetID() == ACTION_CURSOR_RIGHT)
  {
    if ((unsigned int) GetCursorPos() == m_strEdit.size() && (m_strEdit.size() == 0 || m_strEdit[m_strEdit.size() - 1] != ' '))
    { // add a space
      Character(L' ');
    }
    else
      MoveCursor(1);
  }
  else if (action.GetID() == ACTION_SHIFT)
  {
    OnShift();
  }
  else if (action.GetID() == ACTION_SYMBOLS)
  {
    OnSymbols();
  }
  else if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
  {
    OnRemoteNumberClick(action.GetID());
  }
  else if (action.GetID() >= KEY_VKEY && action.GetID() < KEY_ASCII)
  { // input from the keyboard (vkey, not ascii)
    uint8_t b = action.GetID() & 0xFF;
    if (b == 0x24) // home
    {
      MoveCursor(-GetCursorPos());
    }
    else if (b == 0x23) // end
    {
      MoveCursor(m_strEdit.GetLength() - GetCursorPos());
    }
    else if (b == 0x25) // left
    {
      MoveCursor( -1);
    }
    else if (b == 0x27) // right
    {
      MoveCursor(1);
    }
    else if (b == 0x0D) // enter
    {
      OnOK();
    }
    else if (b == 0x2E) // delete
    {
      if (GetCursorPos() < m_strEdit.GetLength())
      {
        MoveCursor(1);
        Backspace();
      }
    }
    else if (b == 0x08) Backspace();    // backspace
    else if (b == 0x1B) Close();        // escape
  }
  else if (action.GetID() >= KEY_ASCII)
  { // input from the keyboard
    //char ch = action.GetID() & 0xFF;
    switch (action.GetUnicode())
    {
    case 13:  // enter
    case 10:  // enter
      OnOK();
      break;
    case 8:   // backspace
      Backspace();
      break;
    case 27:  // escape
      Close();
      break;
    default:  //use character input
      Character(action.GetUnicode());
      break;
    }
  }
  else // unhandled by us - let's see if the baseclass wants it
    handled = CGUIDialog::OnAction(action);

  if (handled && m_filtering == FILTERING_SEARCH)
  { // we did _something_, so make sure our search message filter is reset
    SendSearchMessage();
  }
  return handled;
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
          OnOK();
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
      case CTL_BUTTON_IP_ADDRESS:
        {
          OnIPAddress();
        }
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

void CGUIDialogKeyboard::SetText(const CStdString& aTextString)
{
  m_strEdit.Empty();
  g_charsetConverter.utf8ToW(aTextString, m_strEdit);
  UpdateLabel();
  MoveCursor(m_strEdit.size());
}

CStdString CGUIDialogKeyboard::GetText() const
{
  CStdString utf8String;
  g_charsetConverter.wToUTF8(m_strEdit, utf8String);
  return utf8String;
}

void CGUIDialogKeyboard::Character(WCHAR ch)
{
  if (!ch) return;
  // TODO: May have to make this routine take a WCHAR for the symbols?
  m_strEdit.Insert(GetCursorPos(), ch);
  UpdateLabel();
  MoveCursor(1);
}

void CGUIDialogKeyboard::FrameMove()
{
  // reset the hide state of the label when the remote
  // sms style input times out
  if (m_lastRemoteClickTime && m_lastRemoteClickTime + REMOTE_SMS_DELAY < CTimeUtils::GetFrameTime())
  {
    // finished inputting a sms style character - turn off our shift and symbol states
    ResetShiftAndSymbols();
  }
  CGUIDialog::FrameMove();
}

void CGUIDialogKeyboard::UpdateLabel() // FIXME seems to be called twice for one USB SDL keyboard action/character
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    CStdStringW edit = m_strEdit;
    if (m_hiddenInput)
    { // convert to *'s
      edit.Empty();
      if (m_lastRemoteClickTime + REMOTE_SMS_DELAY > CTimeUtils::GetFrameTime() && m_strEdit.size())
      { // using the remove to input, so display the last key input
        edit.append(m_strEdit.size() - 1, L'*');
        edit.append(1, m_strEdit[m_strEdit.size() - 1]);
      }
      else
        edit.append(m_strEdit.size(), L'*');
    }
    // convert back to utf8
    CStdString utf8Edit;
    g_charsetConverter.wToUTF8(edit, utf8Edit);
    pEdit->SetLabel(utf8Edit);
    // Send off a search message
    unsigned int now = CTimeUtils::GetFrameTime();
    // don't send until the REMOTE_SMS_DELAY has passed
    if (m_lastRemoteClickTime && m_lastRemoteClickTime + REMOTE_SMS_DELAY >= now)
      return;
    if (m_filtering == FILTERING_CURRENT)
    { // send our filter message
      CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS);
      message.SetStringParam(utf8Edit);
      g_windowManager.SendMessage(message);
    }

    if (m_filtering == FILTERING_SEARCH)
      SendSearchMessage();
  }
}

void CGUIDialogKeyboard::SendSearchMessage()
{
  CStdString utf8Edit;
  g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
  // send our search message (only the active window needs it)
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_SEARCH_UPDATE);
  message.SetStringParam(utf8Edit);
  CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (window)
    window->OnMessage(message);
}

void CGUIDialogKeyboard::Backspace()
{
  int iPos = GetCursorPos();
  if (iPos > 0)
  {
    m_strEdit.erase(iPos - 1, 1);
    MoveCursor(-1);
    UpdateLabel();
  }
}

void CGUIDialogKeyboard::OnClickButton(int iButtonControl)
{
  if (iButtonControl == CTL_BUTTON_BACKSPACE)
  {
    Backspace();
  }
  else
    Character(GetCharacter(iButtonControl));
}

void CGUIDialogKeyboard::OnRemoteNumberClick(int key)
{
  unsigned int now = CTimeUtils::GetFrameTime();

  if (m_lastRemoteClickTime)
  { // a remote key has been pressed
    if (key != m_lastRemoteKeyClicked || m_lastRemoteClickTime + REMOTE_SMS_DELAY < now)
    { // a different key was clicked than last time, or we have timed out
      m_lastRemoteKeyClicked = key;
      m_indexInSeries = 0;
      // reset our shift and symbol states, and update our label to ensure the search filter is sent
      ResetShiftAndSymbols();
      UpdateLabel();
    }
    else
    { // same key as last time within the appropriate time period
      m_indexInSeries++;
      Backspace();
    }
  }
  else
  { // key is pressed for the first time
    m_lastRemoteKeyClicked = key;
    m_indexInSeries = 0;
  }

  int arrayIndex = key - REMOTE_0;
  m_indexInSeries = m_indexInSeries % strlen(s_charsSeries[arrayIndex]);
  m_lastRemoteClickTime = now;

  // Select the character that will be pressed
  const char* characterPressed = s_charsSeries[arrayIndex];
  characterPressed += m_indexInSeries;

  // use caps where appropriate
  char ch = *characterPressed;
  bool caps = (m_keyType == CAPS && !m_bShift) || (m_keyType == LOWER && m_bShift);
  if (!caps && *characterPressed >= 'A' && *characterPressed <= 'Z')
    ch += 32;
  Character(ch);
}

char CGUIDialogKeyboard::GetCharacter(int iButton)
{
  // First the numbers
  if (iButton >= 48 && iButton <= 57)
  {
    if (m_keyType == SYMBOLS)
    {
      OnSymbols();
      return symbol_map[iButton -48];
    }
    else
      return (char)iButton;
  }
  else if (iButton == 32) // space
    return (char)iButton;
  else if (iButton >= 65 && iButton < 91)
  {
    if (m_keyType == SYMBOLS)
    { // symbol
      OnSymbols();
      return symbol_map[iButton - 65 + 10];
    }
    if ((m_keyType == CAPS && m_bShift) || (m_keyType == LOWER && !m_bShift))
    { // make lower case
      iButton += 32;
    }
    if (m_bShift)
    { // turn off the shift key
      OnShift();
    }
    return (char) iButton;
  }
  else
  { // check for symbols
    for (unsigned int i = 0; i < NUM_SYMBOLS; i++)
      if (iButton == symbolButtons[i])
        return (char)iButton;
  }
  return 0;
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
  CStdString aLabel = szLabel;

  // set numerals
  for (int iButton = 48; iButton <= 57; iButton++)
  {
    if (m_keyType == SYMBOLS)
      aLabel[0] = symbol_map[iButton - 48];
    else
      aLabel[0] = iButton;
    SetControlLabel(iButton, aLabel);
  }

  // set correct alphabet characters...

  for (int iButton = 65; iButton <= 90; iButton++)
  {
    // set the correct case...
    if ((m_keyType == CAPS && m_bShift) || (m_keyType == LOWER && !m_bShift))
    { // make lower case
      aLabel[0] = iButton + 32;
    }
    else if (m_keyType == SYMBOLS)
    {
      aLabel[0] = symbol_map[iButton - 65 + 10];
    }
    else
    {
      aLabel[0] = iButton;
    }
    SetControlLabel(iButton, aLabel);
  }
  for (unsigned int i = 0; i < NUM_SYMBOLS; i++)
  {
    aLabel[0] = symbolButtons[i];
    SetControlLabel(symbolButtons[i], aLabel);
  }
}

// Show keyboard with initial value (aTextString) and replace with result string.
// Returns: true  - successful display and input (empty result may return true or false depending on parameter)
//          false - unsucessful display of the keyboard or cancelled editing
bool CGUIDialogKeyboard::ShowAndGetInput(CStdString& aTextString, const CStdString &strHeading, bool allowEmptyResult, bool hiddenInput /* = false */)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  // setup keyboard
  pKeyboard->Initialize();
  pKeyboard->CenterWindow();
  pKeyboard->SetHeading(strHeading);
  pKeyboard->SetHiddenInput(hiddenInput);
  pKeyboard->SetText(aTextString);
  // do this using a thread message to avoid render() conflicts
  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, g_windowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, true);
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
  return ShowAndGetInput(aTextString, "", allowEmptyResult) != 0;
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIDialogKeyboard::ShowAndGetNewPassword(CStdString& newPassword, const CStdString &heading, bool allowEmpty)
{
  return ShowAndGetInput(newPassword, heading, allowEmpty, true);
}

// Shows keyboard and prompts for a password.
// Differs from ShowAndVerifyNewPassword() in that no second verification is necessary.
bool CGUIDialogKeyboard::ShowAndGetNewPassword(CStdString& newPassword)
{
  CStdString heading = g_localizeStrings.Get(12340);
  return ShowAndGetNewPassword(newPassword, heading, false);
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param newPassword Overwritten with user input if return=true.
// \param heading Heading to display
// \param allowEmpty Whether a blank password is valid or not.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogKeyboard::ShowAndVerifyNewPassword(CStdString& newPassword, const CStdString &heading, bool allowEmpty)
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
    XBMC::XBMC_MD5 md5state;
    md5state.append(userInput);
    md5state.getDigest(newPassword);
    newPassword.ToLower();
    return true;
  }
  CGUIDialogOK::ShowAndGetInput(12341, 12344, 0, 0);
  return false;
}

// \brief Show keyboard twice to get and confirm a user-entered password string.
// \param strNewPassword Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogKeyboard::ShowAndVerifyNewPassword(CStdString& newPassword)
{
  CStdString heading = g_localizeStrings.Get(12340);
  return ShowAndVerifyNewPassword(newPassword, heading, false);
}

// \brief Show keyboard and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogKeyboard::ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries)
{
  CStdString strHeadingTemp;
  if (1 > iRetries && strHeading.size())
    strHeadingTemp = strHeading;
  else
    strHeadingTemp.Format("%s - %i %s", g_localizeStrings.Get(12326).c_str(), g_guiSettings.GetInt("masterlock.maxretries") - iRetries, g_localizeStrings.Get(12343).c_str());

  CStdString strUserInput = "";
  if (!ShowAndGetInput(strUserInput, strHeadingTemp, false, true))  //bool hiddenInput = false/true ? TODO: GUI Setting to enable disable this feature y/n?
    return -1; // user canceled out

  if (!strPassword.IsEmpty())
  {
    if (strPassword == strUserInput)
      return 0;

    CStdString md5pword2;
    XBMC::XBMC_MD5 md5state;
    md5state.append(strUserInput);
    md5state.getDigest(md5pword2);
    if (strPassword.Equals(md5pword2))
      return 0;     // user entered correct password
    else return 1;  // user must have entered an incorrect password
  }
  else
  {
    if (!strUserInput.IsEmpty())
    {
      XBMC::XBMC_MD5 md5state;
      md5state.append(strUserInput);
      md5state.getDigest(strPassword);
      strPassword.ToLower();
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

void CGUIDialogKeyboard::OnIPAddress()
{
  // find any IP address in the current string if there is any
  // We match to #.#.#.#
  CStdString utf8String;
  g_charsetConverter.wToUTF8(m_strEdit, utf8String);
  CStdString ip;
  CRegExp reg;
  reg.RegComp("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
  int start = reg.RegFind(utf8String.c_str());
  int length = 0;
  if (start > -1)
  {
    length = reg.GetSubLength(0);
    ip = utf8String.Mid(start, length);
  }
  else
    start = utf8String.size();
  if (CGUIDialogNumeric::ShowAndGetIPAddress(ip, g_localizeStrings.Get(14068)))
  {
    utf8String = utf8String.Left(start) + ip + utf8String.Mid(start + length);
    g_charsetConverter.utf8ToW(utf8String, m_strEdit);
    UpdateLabel();
    CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
    if (pEdit)
      pEdit->SetCursorPos(m_strEdit.size());
  }
}

void CGUIDialogKeyboard::ResetShiftAndSymbols()
{
  if (m_bShift) OnShift();
  if (m_keyType == SYMBOLS) OnSymbols();
  m_lastRemoteClickTime = 0;
}

const char* CGUIDialogKeyboard::s_charsSeries[10] = { " !@#$%^&*()[]{}<>/\\|0", ".,;:\'\"-+_=?`~1", "ABC2", "DEF3", "GHI4", "JKL5", "MNO6", "PQRS7", "TUV8", "WXYZ9" };

void CGUIDialogKeyboard::SetControlLabel(int id, const CStdString &label)
{ // find all controls with this id, and set all their labels
  CGUIMessage message(GUI_MSG_LABEL_SET, GetID(), id);
  message.SetLabel(label);
  for (unsigned int i = 0; i < m_children.size(); i++)
  {
    if (m_children[i]->GetID() == id || m_children[i]->IsGroup())
      m_children[i]->OnMessage(message);
  }
}

void CGUIDialogKeyboard::OnOK()
{
  m_bIsConfirmed = true;
  Close();
}

bool CGUIDialogKeyboard::ShowAndGetFilter(CStdString &filter, bool searching)
{
  CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (!pKeyboard)
    return false;

  pKeyboard->m_filtering = searching ? FILTERING_SEARCH : FILTERING_CURRENT;
  bool ret = ShowAndGetInput(filter, true);
  pKeyboard->m_filtering = FILTERING_NONE;
  return ret;
}

