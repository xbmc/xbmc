#include "stdafx.h"
#include "GUIDialogNumeric.h"
#include "util.h"
#include "GUILabelControl.h"
#include "GUIFontManager.h"

#define CONTROL_HEADING_LABEL 1
#define CONTROL_INPUT_LABEL 4
#define CONTROL_NUM0 10
#define CONTROL_NUM9 19
#define CONTROL_PREVIOUS 20
#define CONTROL_ENTER 21
#define CONTROL_NEXT 22
#define CONTROL_BACKSPACE 23


CGUIDialogNumeric::CGUIDialogNumeric(void)
    : CGUIDialog(WINDOW_DIALOG_NUMERIC, "DialogNumeric.xml")
{
  m_bConfirmed = false;
  m_bCanceled = false;

  m_mode = INPUT_PASSWORD;
  m_block = 0;
  m_integer = 0;
  memset(&m_datetime, 0, sizeof(SYSTEMTIME));
  m_dirty = false;
}

CGUIDialogNumeric::~CGUIDialogNumeric(void)
{
}

bool CGUIDialogNumeric::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
    m_bConfirmed = false;
    m_bCanceled = true;
    Close();
  }
  else if (action.wID == ACTION_NEXT_ITEM)
    OnNext();
  else if (action.wID == ACTION_PREV_ITEM)
    OnPrevious();
  else if (action.wID == ACTION_BACKSPACE)
    OnBackSpace();
  else if (action.wID >= REMOTE_0 && action.wID <= REMOTE_9)
    OnNumber(action.wID - REMOTE_0);
  else
    return CGUIDialog::OnAction(action);
  return true;
}

bool CGUIDialogNumeric::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_bConfirmed = false;
      m_bCanceled = false;
      return CGUIDialog::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      m_bConfirmed = false;
      m_bCanceled = false;
      if (CONTROL_NUM0 <= iControl && iControl <= CONTROL_NUM9)  // User numeric entry via dialog button UI
      {
        OnNumber(iControl - 10);
        return true;
      }
      else if (iControl == CONTROL_PREVIOUS)
      {
        OnPrevious();
        return true;
      }
      else if (iControl == CONTROL_NEXT)
      {
        OnNext();
        return true;
      }
      else if (iControl == CONTROL_BACKSPACE)
      {
        OnBackSpace();
        return true;
      }
      else if (iControl == CONTROL_ENTER)
      {
        m_bConfirmed = true;
        Close();
        return true;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogNumeric::OnBackSpace()
{
  if (!m_dirty && m_block)
  {
    m_block--;
    return;
  }
  if (m_mode == INPUT_NUMBER)
  { // just go back one character
    m_integer /= 10;
  }
  else if (m_mode == INPUT_PASSWORD)
  {
    if (!m_password.IsEmpty())
      m_password.Delete(m_password.GetLength() - 1);
  }
  else if (m_mode == INPUT_IP_ADDRESS)
  {
    if (m_ip[m_block])
      m_ip[m_block] /= 10;
    else if (m_block)
    {
      m_block--;
      m_dirty = false;
    }
  }
  else if (m_mode == INPUT_TIME)
  {
    if (m_block == 0)
      m_datetime.wHour /= 10;
    else if (m_datetime.wMinute)
      m_datetime.wMinute /= 10;
    else
    {
      m_block = 0;
      m_dirty = false;
    }
  }
  else if (m_mode == INPUT_DATE)
  {
    if (m_block == 0)
      m_datetime.wDay /= 10;
    else if (m_block == 1)
    {
      if (m_datetime.wMonth)
        m_datetime.wMonth /= 10;
      else
      {
        m_block = 0;
        m_dirty = false;
      }
    }
    else if (m_datetime.wYear) // m_block == 2
      m_datetime.wYear /= 10;
    else
    {
      m_block = 1;
      m_dirty = false;
    }
  }
}

void CGUIDialogNumeric::OnPrevious()
{
  if (m_block)
    m_block--;
  m_dirty = false;
}

void CGUIDialogNumeric::OnNext()
{
  if (m_mode == INPUT_IP_ADDRESS && m_block==0 && m_ip[0]==0)
    return;

  if (m_block < m_lastblock)
    m_block++;
  m_dirty = false;
  if (m_mode == INPUT_DATE)
    VerifyDate(m_block == 2);
}

void CGUIDialogNumeric::Render()
{
  CStdStringW strLabel;
  BYTE *colors = NULL;
  if (m_mode == INPUT_PASSWORD)
  {
    for (unsigned int i=0; i < m_password.size(); i++)
      strLabel += L'*';
  }
  else if (m_mode == INPUT_NUMBER)
  { // simple - just render text directly
    strLabel.Format(L"%d", m_integer);
  }
  else if (m_mode == INPUT_TIME)
  { // format up the time
    strLabel.Format(L"%2d:%02d", m_datetime.wHour, m_datetime.wMinute);
    colors = new BYTE[strLabel.GetLength()];
    for (unsigned int i=0; i < strLabel.size(); i++)
      colors[i] = 0;
    colors[m_block * 3] = colors[m_block * 3 + 1] = 1;
  }
  else if (m_mode == INPUT_DATE)
  { // format up the date
    strLabel.Format(L"%2d/%2d/%4d", m_datetime.wDay, m_datetime.wMonth, m_datetime.wYear);
    colors = new BYTE[strLabel.GetLength()];
    for (unsigned int i=0; i < strLabel.size(); i++)
      colors[i] = 0;
    colors[m_block * 3] = colors[m_block * 3 + 1] = 1;
    if (m_block == 2)
      colors[8] = colors[9] = 1;
  }
  else if (m_mode == INPUT_IP_ADDRESS)
  { // format up the date
    strLabel.Format(L"%3d.%3d.%3d.%3d", m_ip[0], m_ip[1], m_ip[2], m_ip[3]);
    colors = new BYTE[strLabel.GetLength()];
    for (unsigned int i=0; i < strLabel.size(); i++)
      colors[i] = 0;
    colors[m_block * 4] = colors[m_block * 4 + 1] = colors[m_block * 4 + 2] = 1;
  }
  CGUIDialog::Render();
  // render the label in multi-colour so that people can see what they're editting!
  CGUILabelControl *pLabel = (CGUILabelControl *)GetControl(CONTROL_INPUT_LABEL);
  if (pLabel)
  {
    CGUIFont *pFont = g_fontManager.GetFont(pLabel->GetFontName());
    DWORD palette[2];
    palette[0] = pLabel->GetDisabledColor();
    palette[1] = pLabel->GetTextColor();
    float fPosX = (float)pLabel->GetXPosition();
    if (pLabel->m_dwTextAlign & XBFONT_CENTER_X)
      fPosX += (pLabel->GetWidth() - pFont->GetTextWidth(strLabel.c_str())) * 0.5f;
    else if (pLabel->m_dwTextAlign & XBFONT_RIGHT)
      fPosX -= pFont->GetTextWidth(strLabel.c_str());
    pFont->DrawColourTextWidth(fPosX, (float)pLabel->GetYPosition(), palette, strLabel.c_str(), colors, 0);
    delete[] colors;
  }
}

void CGUIDialogNumeric::OnNumber(unsigned int num)
{
  if (m_mode == INPUT_NUMBER)
  {
    m_integer *= 10;
    m_integer += num;
  }
  else if (m_mode == INPUT_PASSWORD)
  {
    m_password += num + '0';
  }
  else if (m_mode == INPUT_TIME)
  {
    if (m_block == 0) // hour
    {
      if (m_dirty) // have input the first digit
      {
        if (m_datetime.wHour < 2 || num < 4)
        {
          m_datetime.wHour *= 10;
          m_datetime.wHour += num;
        }
        else
          m_datetime.wHour = num;
        m_block = 1;             // move to minutes
        m_dirty = false;
      }
      else  // this is the first digit
      {
        m_datetime.wHour = num;
        if (num > 2)
        {
          m_block = 1;             // move to minutes
          m_dirty = false;
        }
        else
          m_dirty = true;
      }
    }
    else  // minute
    {
      if (m_dirty) // have input the first digit
      {
        m_datetime.wMinute *= 10;
        m_datetime.wMinute += num;
        m_block = 0;             // move to hours
        m_dirty = false;
      }
      else  // this is the first digit
      {
        m_datetime.wMinute = num;
        if (num > 5)
        {
          m_block = 0;           // move to hours
          m_dirty = false;
        }
        else
          m_dirty = true;
      }
    }
  }
  else if (m_mode == INPUT_DATE)
  {
    if (m_block == 0) // day of month
    {
      if (m_dirty && (m_datetime.wDay < 3 || num < 2))
      {
        m_datetime.wDay *= 10;
        m_datetime.wDay += num;
      }
      else
        m_datetime.wDay = num;
      if (m_datetime.wDay > 3)
      {
        m_block = 1;             // move to months
        m_dirty = false;
      }
      else
        m_dirty = true;
    }
    else if (m_block == 1)  // months
    {
      if (m_dirty && num < 3)
      {
        m_datetime.wMonth *= 10;
        m_datetime.wMonth += num;
      }
      else
        m_datetime.wMonth = num;
      if (m_datetime.wMonth > 1)
      {
        VerifyDate(false);
        m_block = 2;             // move to year
        m_dirty = false;
      }
      else
        m_dirty = true;
    }
    else // year
    {
      if (m_dirty && m_datetime.wYear < 1000)  // have taken input
      {
        m_datetime.wYear *= 10;
        m_datetime.wYear += num;
      }
      else
        m_datetime.wYear = num;
      if (m_datetime.wYear > 1000)
      {
        VerifyDate(true);
        m_block = 0;        // move to day of month
        m_dirty = false;
      }
      else
        m_dirty = true;
    }
  }
  else if (m_mode == INPUT_IP_ADDRESS)
  {
    if (m_dirty && ((m_ip[m_block] < 25) || (m_ip[m_block] == 25 && num < 6) || !(m_block==0 && num==0)))
    {
      m_ip[m_block] *= 10;
      m_ip[m_block] += num;
    }
    else
      m_ip[m_block] = num;
    if (m_ip[m_block] > 25 || m_ip[m_block] == 0 && num == 0)
    {
      m_block++;
      if (m_block > 3) m_block = 0;
      m_dirty = false;
    }
    else
      m_dirty = true;
  }
}

void CGUIDialogNumeric::SetMode(INPUT_MODE mode, void *initial)
{
  m_mode = mode;
  m_block = 0;
  m_lastblock = 0;
  if (m_mode == INPUT_TIME || m_mode == INPUT_DATE)
  {
    m_datetime = *(SYSTEMTIME *)initial;
    m_lastblock = (m_mode == INPUT_TIME) ? 1 : 2;
  }
  if (m_mode == INPUT_IP_ADDRESS)
  {
    m_lastblock = 3;
    m_ip[0] = m_ip[1] = m_ip[2] = m_ip[3] = 0;
    // copy ip string into numeric form
    CStdString ip = *(CStdString *)initial;
    unsigned int block = 0;
    for (unsigned int i=0; i < ip.size(); i++)
    {
      if (ip[i] == '.')
      {
        block++;
        if (block > m_lastblock)
          break;
      }
      else if (isdigit(ip[i]))
      {
        m_ip[block] *= 10;
        m_ip[block] += ip[i] - '0';
      }
    }
  }
  if (m_mode == INPUT_NUMBER)
  { // convert number from string to number
    m_integer = 0;
    CStdString password = *(CStdString *)initial;
    unsigned int ch = 0;
    while (ch < password.size() && isdigit(password[ch]))
    {
      m_integer *= 10;
      m_integer += password[ch++] - '0';
    }
  }
  if (m_mode == INPUT_PASSWORD)
  {
    m_password = *(CStdString *)initial;
  }
}

void CGUIDialogNumeric::GetOutput(void *output)
{
  if (!output) return;
  if (m_mode == INPUT_TIME || m_mode == INPUT_DATE)
    *(SYSTEMTIME*)output = m_datetime;
  if (m_mode == INPUT_IP_ADDRESS)
  {
    CStdString *ipaddress = (CStdString *)output;
    ipaddress->Format("%d.%d.%d.%d", m_ip[0], m_ip[1], m_ip[2], m_ip[3]);
  }
  if (m_mode == INPUT_NUMBER)
  {
    CStdString *number = (CStdString *)output;
    number->Format("%d", m_integer);
  }
  if (m_mode == INPUT_PASSWORD)
  {
    CStdString *pass = (CStdString *)output;
    *pass = m_password;
  }
}

bool CGUIDialogNumeric::ShowAndGetTime(SYSTEMTIME &time, const CStdStringW &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog) return false;
  pDialog->SetMode(INPUT_TIME, (void *)&time);
  pDialog->SetHeading(heading);
  pDialog->DoModal(m_gWindowManager.GetActiveWindow());
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&time);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetDate(SYSTEMTIME &date, const CStdStringW &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog) return false;
  pDialog->SetMode(INPUT_DATE, (void *)&date);
  pDialog->SetHeading(heading);
  pDialog->DoModal(m_gWindowManager.GetActiveWindow());
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&date);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetIPAddress(CStdString &IPAddress, const CStdStringW &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog || !IPAddress) return false;
  pDialog->SetMode(INPUT_IP_ADDRESS, (void *)&IPAddress);
  pDialog->SetHeading(heading);
  pDialog->DoModal(m_gWindowManager.GetActiveWindow());
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&IPAddress);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetNumber(CStdString& strInput, const CStdStringW &strHeading)
{
  // Prompt user for password input
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  pDialog->SetHeading( strHeading );

  pDialog->SetMode(INPUT_NUMBER, (void *)strInput.c_str());
  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&strInput);
  return true;
}

// \brief Show numeric keypad twice to get and confirm a user-entered password string.
// \param strNewPassword String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndGetNewPassword(CStdString& strNewPassword)
{
  // Prompt user for password input
  CStdString strUserInput = "";
  if (!ShowAndVerifyInput(strUserInput, g_localizeStrings.Get(12340), false))
  {
    // Show error to user saying the password entry was blank
	  CGUIDialogOK::ShowAndGetInput(12357, 12358, 0, 0); // Password is empty/blank
	  return false;
  }

  if (strUserInput.IsEmpty())
    // user canceled out
    return false;

  // Prompt again for password input, this time sending previous input as the password to verify
  if (!ShowAndVerifyInput(strUserInput, g_localizeStrings.Get(12341), true))
  {
    // Show error to user saying the password re-entry failed
	  CGUIDialogOK::ShowAndGetInput(12357, 12344, 0, 0); // Password do not match
    return false;
  }

  // password entry and re-entry succeeded
  strNewPassword = strUserInput;
  return true;
}

// \brief Show numeric keypad and verify user input against strPassword.
// \param strPassword Value to compare against user input.
// \param strHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param iRetries If greater than 0, shows "Incorrect password, %d retries left" on dialog line 2, else line 2 is blank.
// \return 0 if successful display and user input. 1 if unsucessful input. -1 if no user input or canceled editing.
int CGUIDialogNumeric::ShowAndVerifyPassword(CStdString& strPassword, const CStdStringW& strHeading, int iRetries)
{
  CStdStringW strTempHeading = strHeading;
  if (0 < iRetries)
  {
    // Show a string telling user they have iRetries retries left
    strTempHeading.Format(L"%s. %s %i %s", strHeading.c_str(), g_localizeStrings.Get(12342).c_str(), iRetries, g_localizeStrings.Get(12343).c_str());
  }
  // make a copy of strPassword to prevent from overwriting it later
  CStdString strPassTemp = strPassword;
  if (ShowAndVerifyInput(strPassTemp, strTempHeading, true))
  {
    // user entered correct password
    return 0;
  }

  if (strPassTemp.IsEmpty())
    // user canceled out
    return -1;

  // user must have entered an incorrect password
  return 1;
}

// \brief Show numeric keypad and verify user input against strToVerify.
// \param strToVerify Value to compare against user input.
// \param dlgHeading String shown on dialog title.
// \param bVerifyInput If set as true we verify the users input versus strToVerify.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndVerifyInput(CStdString& strToVerify, const CStdStringW& dlgHeading, bool bVerifyInput)
{
  // Prompt user for password input
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)m_gWindowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  pDialog->SetHeading( dlgHeading );

  CStdString strInput = "";
  if (!bVerifyInput)
    strInput = strToVerify;
  pDialog->SetMode(INPUT_PASSWORD, (void *)&strInput);
  pDialog->DoModal(m_gWindowManager.GetActiveWindow());

  pDialog->GetOutput(&strInput);

  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
  {
    // user canceled out
    return false;
  }

  if (!bVerifyInput)
  {
    strToVerify = strInput;
    return true;
  }

  if (strToVerify.Equals(strInput))
    return true;  // entered correct password

  // incorrect password
  return false;
}

bool CGUIDialogNumeric::IsConfirmed() const
{
  return m_bConfirmed;
}

bool CGUIDialogNumeric::IsCanceled() const
{
  return m_bCanceled;
}

void CGUIDialogNumeric::SetHeading(const CStdStringW& strHeading)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING_LABEL);
  msg.SetLabel(strHeading);
  OnMessage(msg);
}

void CGUIDialogNumeric::VerifyDate(bool checkYear)
{
  if (m_datetime.wDay == 0)
    m_datetime.wDay = 1;
  if (m_datetime.wMonth == 0)
    m_datetime.wMonth = 1;
  // check for number of days in the month
  if (m_datetime.wDay == 31)
  {
    if (m_datetime.wMonth == 4 || m_datetime.wMonth == 6 || m_datetime.wMonth == 9 || m_datetime.wMonth == 11)
      m_datetime.wDay = 30;
  }
  if (m_datetime.wMonth == 2 && m_datetime.wDay > 28)
  {
    m_datetime.wDay = 29;   // max in february.
    if (checkYear)
    {
      // leap years occur when the year is divisible by 4 but not by 100, or the year is divisible by 400
      // thus they don't occur, if the year has a remainder when divided by 4, or when the year is divisible by 100 but not by 400
      if ( (m_datetime.wYear % 4) || ( !(m_datetime.wYear % 100) && (m_datetime.wYear % 400) ) )
        m_datetime.wDay = 28;
    }
  }
}
