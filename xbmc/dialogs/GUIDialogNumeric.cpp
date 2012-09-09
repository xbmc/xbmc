/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogNumeric.h"
#include "guilib/GUILabelControl.h"
#include "utils/md5.h"
#include "guilib/GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "input/XBMC_vkeys.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"

#define CONTROL_HEADING_LABEL  1
#define CONTROL_INPUT_LABEL    4
#define CONTROL_NUM0          10
#define CONTROL_NUM9          19
#define CONTROL_PREVIOUS      20
#define CONTROL_ENTER         21
#define CONTROL_NEXT          22
#define CONTROL_BACKSPACE     23


CGUIDialogNumeric::CGUIDialogNumeric(void)
    : CGUIDialog(WINDOW_DIALOG_NUMERIC, "DialogNumeric.xml")
{
  m_bConfirmed = false;
  m_bCanceled = false;

  m_mode = INPUT_PASSWORD;
  m_block = 0;
  memset(&m_datetime, 0, sizeof(SYSTEMTIME));
  m_dirty = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogNumeric::~CGUIDialogNumeric(void)
{
}

void CGUIDialogNumeric::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  CVariant data;
  switch (m_mode)
  {
  case INPUT_TIME:
    data["type"] = "time";
    break;
  case INPUT_DATE:
    data["type"] = "date";
    break;
  case INPUT_IP_ADDRESS:
    data["type"] = "ip";
    break;
  case INPUT_PASSWORD:
    data["type"] = "numericpassword";
    break;
  case INPUT_NUMBER:
    data["type"] = "number";
    break;
  case INPUT_TIME_SECONDS:
    data["type"] = "seconds";
    break;
  default:
    data["type"] = "keyboard";
    break;
  }

  const CGUILabelControl *control = (const CGUILabelControl *)GetControl(CONTROL_HEADING_LABEL);
  if (control != NULL)
    data["title"] = control->GetDescription();

  data["value"] = GetOutput();
  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::Input, "xbmc", "OnInputRequested", data);
}

void CGUIDialogNumeric::OnDeinitWindow(int nextWindowID)
{
  // call base class
  CGUIDialog::OnDeinitWindow(nextWindowID);

  ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::Input, "xbmc", "OnInputFinished");
}

bool CGUIDialogNumeric::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_NEXT_ITEM)
    OnNext();
  else if (action.GetID() == ACTION_PREV_ITEM)
    OnPrevious();
  else if (action.GetID() == ACTION_BACKSPACE)
    OnBackSpace();
  else if (action.GetID() == ACTION_ENTER)
    OnOK();
  else if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
    OnNumber(action.GetID() - REMOTE_0);
  else if (action.GetID() >= KEY_VKEY && action.GetID() < KEY_ASCII)
  { // input from the keyboard (vkey, not ascii)
    BYTE b = action.GetID() & 0xFF;
    if (b == XBMCVK_LEFT) OnPrevious();
    else if (b == XBMCVK_RIGHT) OnNext();
    else if (b == XBMCVK_RETURN || b == XBMCVK_NUMPADENTER) OnOK();
    else if (b == XBMCVK_BACK) OnBackSpace();
    else if (b == XBMCVK_ESCAPE) OnCancel();
  }
  else if (action.GetID() >= KEY_ASCII) // FIXME make it KEY_UNICODE
  { // input from the keyboard
    if (action.GetUnicode() == 10 || action.GetUnicode() == 13) OnOK(); // enter
    else if (action.GetUnicode() == 8) OnBackSpace(); // backspace
    else if (action.GetUnicode() == 27) OnCancel(); // escape
    else if (action.GetUnicode() >= 48 && action.GetUnicode() < 58)  // number
      OnNumber(action.GetUnicode() - 48);
  }
  else
    return CGUIDialog::OnAction(action);
  return true;
}

bool CGUIDialogNumeric::OnBack(int actionID)
{
  OnCancel();
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
      m_dirty = false;
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
        OnOK();
        return true;
      }
    }
    break;

  case GUI_MSG_SET_TEXT:
    SetMode(m_mode, message.GetLabel());

    // close the dialog if requested
    if (message.GetParam1() > 0)
      OnOK();
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
  if (m_mode == INPUT_NUMBER || m_mode == INPUT_PASSWORD)
  { // just go back one character
    if (!m_number.IsEmpty())
      m_number.Delete(m_number.GetLength() - 1);
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
  else if (m_mode == INPUT_TIME_SECONDS)
  {
    if (m_block == 0) // minutes
      m_datetime.wMinute /= 10;
    else if (m_datetime.wSecond)
      m_datetime.wSecond /= 10;
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

void CGUIDialogNumeric::FrameMove()
{
  CStdString strLabel;
  unsigned int start = 0;
  unsigned int end = 0;
  if (m_mode == INPUT_PASSWORD)
  {
    for (unsigned int i=0; i < m_number.size(); i++)
      strLabel += '*';
  }
  else if (m_mode == INPUT_NUMBER)
  { // simple - just render text directly
    strLabel = m_number;
  }
  else if (m_mode == INPUT_TIME)
  { // format up the time
    strLabel.Format("%2d:%02d", m_datetime.wHour, m_datetime.wMinute);
    start = m_block * 3;
    end = m_block * 3 + 2;
  }
  else if (m_mode == INPUT_TIME_SECONDS)
  { // format up the time
    strLabel.Format("%2d:%02d", m_datetime.wMinute, m_datetime.wSecond);
    start = m_block * 3;
    end = m_block * 3 + 2;
  }
  else if (m_mode == INPUT_DATE)
  { // format up the date
    strLabel.Format("%2d/%2d/%4d", m_datetime.wDay, m_datetime.wMonth, m_datetime.wYear);
    start = m_block * 3;
    end = m_block * 3 + 2;
    if (m_block == 2)
      end = m_block * 3 + 4;
  }
  else if (m_mode == INPUT_IP_ADDRESS)
  { // format up the date
    strLabel.Format("%3d.%3d.%3d.%3d", m_ip[0], m_ip[1], m_ip[2], m_ip[3]);
    start = m_block * 4;
    end = m_block * 4 + 3;
  }
  CGUILabelControl *pLabel = (CGUILabelControl *)GetControl(CONTROL_INPUT_LABEL);
  if (pLabel)
  {
    pLabel->SetLabel(strLabel);
    pLabel->SetHighlight(start, end);
  }
  CGUIDialog::FrameMove();
}

void CGUIDialogNumeric::OnNumber(unsigned int num)
{
  ResetAutoClose();

  if (m_mode == INPUT_NUMBER || m_mode == INPUT_PASSWORD)
  {
    m_number += num + '0';
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
  else if (m_mode == INPUT_TIME_SECONDS)
  {
    if (m_block == 0) // minute
    {
      if (m_dirty) // have input the first digit
      {
        m_datetime.wMinute *= 10;
        m_datetime.wMinute += num;
        m_block = 1;             // move to seconds - allows up to 99 minutes
        m_dirty = false;
      }
      else  // this is the first digit
      {
        m_datetime.wMinute = num;
        m_dirty = true;
      }
    }
    else  // seconds
    {
      if (m_dirty) // have input the first digit
      {
        m_datetime.wSecond *= 10;
        m_datetime.wSecond += num;
        m_block = 0;             // move to minutes
        m_dirty = false;
      }
      else  // this is the first digit
      {
        m_datetime.wSecond = num;
        if (num > 5)
        {
          m_block = 0;           // move to minutes
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
    if (m_ip[m_block] > 25 || (m_ip[m_block] == 0 && num == 0))
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
  if (m_mode == INPUT_TIME || m_mode == INPUT_TIME_SECONDS || m_mode == INPUT_DATE)
  {
    m_datetime = *(SYSTEMTIME *)initial;
    m_lastblock = (m_mode == INPUT_DATE) ? 2 : 1;
  }
  else if (m_mode == INPUT_IP_ADDRESS)
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
  else if (m_mode == INPUT_NUMBER || m_mode == INPUT_PASSWORD)
    m_number = *(CStdString *)initial;
}

void CGUIDialogNumeric::SetMode(INPUT_MODE mode, const CStdString &initial)
{
  m_mode = mode;
  m_block = 0;
  m_lastblock = 0;
  if (m_mode == INPUT_TIME || m_mode == INPUT_TIME_SECONDS || m_mode == INPUT_DATE)
  {
    CDateTime dateTime;
    if (m_mode == INPUT_TIME || m_mode == INPUT_TIME_SECONDS)
    {
      // check if we have a pure number
      if (initial.find_first_not_of("0123456789") == std::string::npos)
      {
        long seconds = strtol(initial.c_str(), NULL, 10);
        dateTime = seconds;
      }
      else
      {
        CStdString tmp = initial;
        // if we are handling seconds and if the string only contains
        // "mm:ss" we need to add dummy "hh:" to get "hh:mm:ss"
        if (m_mode == INPUT_TIME_SECONDS && tmp.size() <= 5)
          tmp = "00:" + tmp;
        dateTime.SetFromDBTime(tmp);
      }
    }
    else if (m_mode == INPUT_DATE)
    {
      CStdString tmp = initial;
      tmp.Replace("/", ".");
      dateTime.SetFromDBDate(tmp);
    }

    if (!dateTime.IsValid())
      return;

    dateTime.GetAsSystemTime(m_datetime);
    m_lastblock = (m_mode == INPUT_DATE) ? 2 : 1;
  }
  else
    SetMode(mode, (void*)&initial);
}

void CGUIDialogNumeric::GetOutput(void *output) const
{
  if (!output) return;
  if (m_mode == INPUT_TIME || m_mode == INPUT_TIME_SECONDS || m_mode == INPUT_DATE)
    memcpy(output, &m_datetime, sizeof(m_datetime));
  else if (m_mode == INPUT_IP_ADDRESS)
  {
    CStdString *ipaddress = (CStdString *)output;
    ipaddress->Format("%d.%d.%d.%d", m_ip[0], m_ip[1], m_ip[2], m_ip[3]);
  }
  else if (m_mode == INPUT_NUMBER || m_mode == INPUT_PASSWORD)
  {
    CStdString *number = (CStdString *)output;
    *number = m_number;
  }
}

CStdString CGUIDialogNumeric::GetOutput() const
{
  CStdString output;
  if (m_mode == INPUT_DATE)
    output.Format("%02i/%02i/%04i", m_datetime.wDay, m_datetime.wMonth, m_datetime.wYear);
  else if (m_mode == INPUT_TIME)
    output.Format("%i:%02i", m_datetime.wHour, m_datetime.wMinute);
  else if (m_mode == INPUT_TIME_SECONDS)
    output.Format("%i:%02i", m_datetime.wMinute, m_datetime.wSecond);
  else
    GetOutput(&output);
  return output;
}

bool CGUIDialogNumeric::ShowAndGetSeconds(CStdString &timeString, const CStdString &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog) return false;
  int seconds = StringUtils::TimeStringToSeconds(timeString);
  SYSTEMTIME time = {0};
  time.wHour = seconds / 3600;
  time.wMinute = (seconds - time.wHour * 3600) / 60;
  time.wSecond = seconds - time.wHour * 3600 - time.wMinute * 60;
  pDialog->SetMode(INPUT_TIME_SECONDS, (void *)&time);
  pDialog->SetHeading(heading);
  pDialog->DoModal();
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&time);
  seconds = time.wHour * 3600 + time.wMinute * 60 + time.wSecond;
  timeString = StringUtils::SecondsToTimeString(seconds);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetTime(SYSTEMTIME &time, const CStdString &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog) return false;
  pDialog->SetMode(INPUT_TIME, (void *)&time);
  pDialog->SetHeading(heading);
  pDialog->DoModal();
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&time);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetDate(SYSTEMTIME &date, const CStdString &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog) return false;
  pDialog->SetMode(INPUT_DATE, (void *)&date);
  pDialog->SetHeading(heading);
  pDialog->DoModal();
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&date);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetIPAddress(CStdString &IPAddress, const CStdString &heading)
{
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  if (!pDialog || !IPAddress) return false;
  pDialog->SetMode(INPUT_IP_ADDRESS, (void *)&IPAddress);
  pDialog->SetHeading(heading);
  pDialog->DoModal();
  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
    return false;
  pDialog->GetOutput(&IPAddress);
  return true;
}

bool CGUIDialogNumeric::ShowAndGetNumber(CStdString& strInput, const CStdString &strHeading, unsigned int iAutoCloseTimeoutMs /* = 0 */)
{
  // Prompt user for password input
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  pDialog->SetHeading( strHeading );

  pDialog->SetMode(INPUT_NUMBER, (void *)&strInput);
  if (iAutoCloseTimeoutMs)
    pDialog->SetAutoClose(iAutoCloseTimeoutMs);

  pDialog->DoModal();

  if (!pDialog->IsAutoClosed() && (!pDialog->IsConfirmed() || pDialog->IsCanceled()))
    return false;
  pDialog->GetOutput(&strInput);
  return true;
}

// \brief Show numeric keypad twice to get and confirm a user-entered password string.
// \param strNewPassword String to preload into the keyboard accumulator. Overwritten with user input if return=true.
// \return true if successful display and user input entry/re-entry. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndVerifyNewPassword(CStdString& strNewPassword)
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
int CGUIDialogNumeric::ShowAndVerifyPassword(CStdString& strPassword, const CStdString& strHeading, int iRetries)
{
  CStdString strTempHeading = strHeading;
  if (0 < iRetries)
  {
    // Show a string telling user they have iRetries retries left
    strTempHeading.Format("%s. %s %i %s", strHeading.c_str(), g_localizeStrings.Get(12342).c_str(), iRetries, g_localizeStrings.Get(12343).c_str());
  }
  // make a copy of strPassword to prevent from overwriting it later
  CStdString strPassTemp = strPassword;
  if (ShowAndVerifyInput(strPassTemp, strTempHeading, true))
    return 0;   // user entered correct password
  if (strPassTemp.IsEmpty()) return -1;   // user canceled out
  return 1; // user must have entered an incorrect password
}

// \brief Show numeric keypad and verify user input against strToVerify.
// \param strToVerify Value to compare against user input.
// \param dlgHeading String shown on dialog title.
// \param bVerifyInput If set as true we verify the users input versus strToVerify.
// \return true if successful display and user input. false if unsucessful display, no user input, or canceled editing.
bool CGUIDialogNumeric::ShowAndVerifyInput(CStdString& strToVerify, const CStdString& dlgHeading, bool bVerifyInput)
{
  // Prompt user for password input
  CGUIDialogNumeric *pDialog = (CGUIDialogNumeric *)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
  pDialog->SetHeading( dlgHeading );

  CStdString strInput = "";
  if (!bVerifyInput)
    strInput = strToVerify;
  pDialog->SetMode(INPUT_PASSWORD, (void *)&strInput);
  pDialog->DoModal();

  pDialog->GetOutput(&strInput);

  if (!pDialog->IsConfirmed() || pDialog->IsCanceled())
  {
    // user canceled out
    strToVerify ="";
    return false;
  }

  CStdString md5pword2;
  XBMC::XBMC_MD5 md5state;
  md5state.append(strInput);
  md5state.getDigest(md5pword2);

  if (!bVerifyInput)
  {
    strToVerify = md5pword2;
    strToVerify.ToLower();
    return true;
  }

  if (strToVerify.Equals(md5pword2))
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

void CGUIDialogNumeric::SetHeading(const CStdString& strHeading)
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

void CGUIDialogNumeric::OnOK()
{
  m_bConfirmed = true;
  m_bCanceled = false;
  Close();
}

void CGUIDialogNumeric::OnCancel()
{
  m_bConfirmed = false;
  m_bCanceled = true;
  Close();
}

