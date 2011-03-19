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

#include "GUIEditControl.h"
#include "GUIWindowManager.h"
#include "utils/CharsetConverter.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogNumeric.h"
#include "LocalizeStrings.h"
#include "DateTime.h"
#include "utils/md5.h"

#ifdef __APPLE__
#include "CocoaInterface.h"
#endif

const char* CGUIEditControl::smsLetters[10] = { " !@#$%^&*()[]{}<>/\\|0", ".,;:\'\"-+_=?`~1", "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9" };
const unsigned int CGUIEditControl::smsDelay = 1000;

using namespace std;

#ifdef WIN32
extern HWND g_hWnd;
#endif

CGUIEditControl::CGUIEditControl(int parentID, int controlID, float posX, float posY,
                                 float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                                 const CLabelInfo& labelInfo, const std::string &text)
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
  DefaultConstructor();
  SetLabel(text);
}

void CGUIEditControl::DefaultConstructor()
{
  ControlType = GUICONTROL_EDIT;
  m_textOffset = 0;
  m_textWidth = GetWidth();
  m_cursorPos = 0;
  m_cursorBlink = 0;
  m_inputHeading = 0;
  m_inputType = INPUT_TYPE_TEXT;
  m_smsLastKey = 0;
  m_smsKeyIndex = 0;
  m_label.SetAlign(m_label.GetLabelInfo().align & XBFONT_CENTER_Y); // left align
  m_label2.GetLabelInfo().offsetX = 0;
  m_isMD5 = false;
}

CGUIEditControl::CGUIEditControl(const CGUIButtonControl &button)
    : CGUIButtonControl(button)
{
  DefaultConstructor();
}

CGUIEditControl::~CGUIEditControl(void)
{
}

bool CGUIEditControl::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_SET_TYPE)
  {
    SetInputType((INPUT_TYPE)message.GetParam1(), (int)message.GetParam2());
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
  {
    message.SetLabel(GetLabel2());
    return true;
  }
  else if (message.GetMessage() == GUI_MSG_SETFOCUS ||
           message.GetMessage() == GUI_MSG_LOSTFOCUS)
  {
    m_smsTimer.Stop();
  }
  return CGUIButtonControl::OnMessage(message);
}

bool CGUIEditControl::OnAction(const CAction &action)
{
  ValidateCursor();

  if (action.GetID() == ACTION_BACKSPACE)
  {
    // backspace
    if (m_cursorPos)
    {
      if (!ClearMD5())
        m_text2.erase(--m_cursorPos, 1);
      UpdateText();
    }
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_LEFT)
  {
    if (m_cursorPos > 0)
    {
      m_cursorPos--;
      UpdateText(false);
      return true;
    }
  }
  else if (action.GetID() == ACTION_MOVE_RIGHT)
  {
    if ((unsigned int) m_cursorPos < m_text2.size())
    {
      m_cursorPos++;
      UpdateText(false);
      return true;
    }
  }
  else if (action.GetID() == ACTION_PASTE)
  {
    ClearMD5();
    OnPasteClipboard();
  }
  else if (action.GetID() >= KEY_VKEY && action.GetID() < KEY_ASCII)
  {
    // input from the keyboard (vkey, not ascii)
    BYTE b = action.GetID() & 0xFF;
    if (b == 0x24) // home
    {
      m_cursorPos = 0;
      UpdateText(false);
      return true;
    }
    else if (b == 0x23) // end
    {
      m_cursorPos = m_text2.length();
      UpdateText(false);
      return true;
    }
    if (b == 0x25 && m_cursorPos > 0)
    { // left
      m_cursorPos--;
      UpdateText(false);
      return true;
    }
    if (b == 0x27 && m_cursorPos < m_text2.length())
    { // right
      m_cursorPos++;
      UpdateText(false);
      return true;
    }
    if (b == 0x2e)
    {
      if (m_cursorPos < m_text2.length())
      { // delete
        if (!ClearMD5())
          m_text2.erase(m_cursorPos, 1);
        UpdateText();
        return true;
      }
    }
    if (b == 0x8)
    {
      if (m_cursorPos > 0)
      { // backspace
        if (!ClearMD5())
          m_text2.erase(--m_cursorPos, 1);
        UpdateText();
      }
      return true;
    }
  }
  else if (action.GetID() >= KEY_ASCII)
  {
    // input from the keyboard
    switch (action.GetUnicode())
    {
    case '\t':
      break;
    case 10:
    case 13:
      {
        // enter - send click message, but otherwise ignore
        SEND_CLICK_MESSAGE(GetID(), GetParentID(), 1);
        return true;
      }
    case 27:
      { // escape - fallthrough to default action
        return CGUIButtonControl::OnAction(action);
      }
    case 8:
      {
        // backspace
        if (m_cursorPos)
        {
          if (!ClearMD5())
            m_text2.erase(--m_cursorPos, 1);
        }
        break;
      }
    default:
      {
        ClearMD5();
        m_text2.insert(m_text2.begin() + m_cursorPos++, (WCHAR)action.GetUnicode());
        break;
      }
    }
    UpdateText();
    return true;
  }
  else if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
  { // input from the remote
    ClearMD5();
    if (m_inputType == INPUT_TYPE_FILTER)
    { // filtering - use single number presses
      m_text2.insert(m_text2.begin() + m_cursorPos++, L'0' + (action.GetID() - REMOTE_0));
      UpdateText();
    }
    else
      OnSMSCharacter(action.GetID() - REMOTE_0);
    return true;
  }
  return CGUIButtonControl::OnAction(action);
}

void CGUIEditControl::OnClick()
{
  // we received a click - it's not from the keyboard, so pop up the virtual keyboard, unless
  // that is where we reside!
  if (GetParentID() == WINDOW_DIALOG_KEYBOARD)
    return;

  CStdString utf8;
  g_charsetConverter.wToUTF8(m_text2, utf8);
  bool textChanged = false;
  CStdString heading = g_localizeStrings.Get(m_inputHeading ? m_inputHeading : 16028);
  switch (m_inputType)
  {
    case INPUT_TYPE_NUMBER:
      textChanged = CGUIDialogNumeric::ShowAndGetNumber(utf8, heading);
      break;
    case INPUT_TYPE_SECONDS:
      textChanged = CGUIDialogNumeric::ShowAndGetSeconds(utf8, g_localizeStrings.Get(21420));
      break;
    case INPUT_TYPE_DATE:
    {
      CDateTime dateTime;
      dateTime.SetFromDBDate(utf8);
      if (dateTime < CDateTime(2000,1, 1, 0, 0, 0))
        dateTime = CDateTime(2000, 1, 1, 0, 0, 0);
      SYSTEMTIME date;
      dateTime.GetAsSystemTime(date);
      if (CGUIDialogNumeric::ShowAndGetDate(date, g_localizeStrings.Get(21420)))
      {
        dateTime = CDateTime(date);
        utf8 = dateTime.GetAsDBDate();
        textChanged = true;
      }
      break;
    }
    case INPUT_TYPE_IPADDRESS:
      textChanged = CGUIDialogNumeric::ShowAndGetIPAddress(utf8, heading);
      break;
    case INPUT_TYPE_SEARCH:
      textChanged = CGUIDialogKeyboard::ShowAndGetFilter(utf8, true);
      break;
    case INPUT_TYPE_FILTER:
      textChanged = CGUIDialogKeyboard::ShowAndGetFilter(utf8, false);
      break;
    case INPUT_TYPE_PASSWORD_MD5:
      utf8 = ""; // TODO: Ideally we'd send this to the keyboard and tell the keyboard we have this type of input
      // fallthrough
    case INPUT_TYPE_TEXT:
    default:
      textChanged = CGUIDialogKeyboard::ShowAndGetInput(utf8, heading, true, m_inputType == INPUT_TYPE_PASSWORD || m_inputType == INPUT_TYPE_PASSWORD_MD5);
      break;
  }
  if (textChanged)
  {
    ClearMD5();
    g_charsetConverter.utf8ToW(utf8, m_text2);
    m_cursorPos = m_text2.size();
    UpdateText();
    m_cursorPos = m_text2.size();
  }
}

void CGUIEditControl::UpdateText(bool sendUpdate)
{
  m_smsTimer.Stop();
  if (sendUpdate)
  {
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);

    vector<CGUIActionDescriptor> textChangeActions = m_textChangeActions;
    for (unsigned int i = 0; i < textChangeActions.size(); i++)
    {
      CGUIMessage message(GUI_MSG_EXECUTE, GetID(), GetParentID());
      message.SetAction(textChangeActions[i]);
      g_windowManager.SendMessage(message);
    }
  }
  SetInvalid();
}

void CGUIEditControl::SetInputType(CGUIEditControl::INPUT_TYPE type, int heading)
{
  m_inputType = type;
  m_inputHeading = heading;
  // TODO: Verify the current input string?
}

void CGUIEditControl::RecalcLabelPosition()
{
  // ensure that our cursor is within our width
  ValidateCursor();

  CStdStringW text = GetDisplayedText();
  m_textWidth = m_label.CalcTextWidth(text + L'|');
  float beforeCursorWidth = m_label.CalcTextWidth(text.Left(m_cursorPos));
  float afterCursorWidth = m_label.CalcTextWidth(text.Left(m_cursorPos) + L'|');
  float leftTextWidth = m_label.GetRenderRect().Width();
  float maxTextWidth = m_label.GetMaxWidth();
  if (leftTextWidth > 0)
    maxTextWidth -= leftTextWidth + spaceWidth;

  // if skinner forgot to set height :p
  if (m_height == 0 && m_label.GetLabelInfo().font)
    m_height = m_label.GetLabelInfo().font->GetTextHeight(1);

  if (m_textWidth > maxTextWidth)
  { // we render taking up the full width, so make sure our cursor position is
    // within the render window
    if (m_textOffset + afterCursorWidth > maxTextWidth)
    {
      // move the position to the left (outside of the viewport)
      m_textOffset = maxTextWidth - afterCursorWidth;
    }
    else if (m_textOffset + beforeCursorWidth < 0) // offscreen to the left
    {
      // otherwise use original position
      m_textOffset = -beforeCursorWidth;
    }
    else if (m_textOffset + m_textWidth < maxTextWidth)
    { // we have more text than we're allowed, but we aren't filling all the space
      m_textOffset = maxTextWidth - m_textWidth;
    }
  }
  else
    m_textOffset = 0;
}

void CGUIEditControl::RenderText()
{
  if (m_smsTimer.GetElapsedMilliseconds() > smsDelay)
    UpdateText();

  if (m_bInvalidated)
  {
    m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
    m_label.SetText(m_info.GetLabel(GetParentID()));
    RecalcLabelPosition();
  }


  float posX = m_label.GetRenderRect().x1;
  float maxTextWidth = m_label.GetMaxWidth();

  // start by rendering the normal text
  float leftTextWidth = m_label.GetRenderRect().Width();
  if (leftTextWidth > 0)
  {
    // render the text on the left
    m_label.SetColor(GetTextColor());
    m_label.Render();
    
    posX += leftTextWidth + spaceWidth;
    maxTextWidth -= leftTextWidth + spaceWidth;
  }

  if (g_graphicsContext.SetClipRegion(posX, m_posY, maxTextWidth, m_height))
  {
    uint32_t align = m_label.GetLabelInfo().align & XBFONT_CENTER_Y; // start aligned left
    if (m_label2.GetTextWidth() < maxTextWidth)
    { // align text as our text fits
      if (leftTextWidth > 0)
      { // right align as we have 2 labels
        align |= XBFONT_RIGHT;
      }
      else
      { // align by whatever the skinner requests
        align |= (m_label2.GetLabelInfo().align & 3);
      }
    }
    CStdStringW text = GetDisplayedText();
    // add the cursor if we're focused
    if (HasFocus())
    {
      CStdStringW col;
      if ((m_focusCounter % 64) > 32)
        col = L"|";
      else
        col = L"[COLOR 00FFFFFF]|[/COLOR]";
      text.Insert(m_cursorPos, col);
    }

    m_label2.SetMaxRect(posX + m_textOffset, m_posY, maxTextWidth - m_textOffset, m_height);
    m_label2.SetTextW(text);
    m_label2.SetAlign(align);
    m_label2.SetColor(GetTextColor());
    m_label2.Render();
    g_graphicsContext.RestoreClipRegion();
  }
}

CStdStringW CGUIEditControl::GetDisplayedText() const
{
  if (m_inputType == INPUT_TYPE_PASSWORD || m_inputType == INPUT_TYPE_PASSWORD_MD5)
  {
    CStdStringW text;
    text.append(m_text2.size(), L'*');
    return text;
  }
  return m_text2;
}

void CGUIEditControl::ValidateCursor()
{
  if (m_cursorPos > m_text2.size())
    m_cursorPos = m_text2.size();
}

void CGUIEditControl::SetLabel(const std::string &text)
{
  CGUIButtonControl::SetLabel(text);
  SetInvalid();
}

void CGUIEditControl::SetLabel2(const std::string &text)
{
  CStdStringW newText;
  g_charsetConverter.utf8ToW(text, newText);
  if (newText != m_text2)
  {
    m_isMD5 = m_inputType == INPUT_TYPE_PASSWORD_MD5;
    m_text2 = newText;
    m_cursorPos = m_text2.size();
    SetInvalid();
  }
}

CStdString CGUIEditControl::GetLabel2() const
{
  CStdString text;
  g_charsetConverter.wToUTF8(m_text2, text);
  if (m_inputType == INPUT_TYPE_PASSWORD_MD5 && !m_isMD5)
    return XBMC::XBMC_MD5::GetMD5(text);
  return text;
}

bool CGUIEditControl::ClearMD5()
{
  if (m_inputType != INPUT_TYPE_PASSWORD_MD5 || !m_isMD5)
    return false;
  
  m_text2.Empty();
  m_cursorPos = 0;
  m_isMD5 = false;
  return true;
}

unsigned int CGUIEditControl::GetCursorPosition() const
{
  return m_cursorPos;
}

void CGUIEditControl::SetCursorPosition(unsigned int iPosition)
{
  m_cursorPos = iPosition;
}

void CGUIEditControl::OnSMSCharacter(unsigned int key)
{
  assert(key < 10);
  bool sendUpdate = false;
  if (m_smsTimer.IsRunning())
  {
    // we're already entering an SMS character
    if (key != m_smsLastKey || m_smsTimer.GetElapsedMilliseconds() > smsDelay)
    { // a different key was clicked than last time, or we have timed out
      m_smsLastKey = key;
      m_smsKeyIndex = 0;
      sendUpdate = true;
    }
    else
    { // same key as last time within the appropriate time period
      m_smsKeyIndex++;
      if (m_cursorPos)
        m_text2.erase(--m_cursorPos, 1);
    }
  }
  else
  { // key is pressed for the first time
    m_smsLastKey = key;
    m_smsKeyIndex = 0;
  }

  m_smsKeyIndex = m_smsKeyIndex % strlen(smsLetters[key]);

  m_text2.insert(m_text2.begin() + m_cursorPos++, smsLetters[key][m_smsKeyIndex]);
  UpdateText(sendUpdate);
  m_smsTimer.StartZero();
}

void CGUIEditControl::OnPasteClipboard()
{
#if defined(__APPLE__) && !defined(__arm__)
  const char *szStr = Cocoa_Paste();
  if (szStr)
  {
    m_text2 += szStr;
    m_cursorPos+=strlen(szStr);
    UpdateText();
  }
#elif defined _WIN32
  if (OpenClipboard(g_hWnd))
  {
    HGLOBAL hglb = GetClipboardData(CF_TEXT);
    if (hglb != NULL)
    {
      LPTSTR lptstr = (LPTSTR)GlobalLock(hglb);
      if (lptstr != NULL)
      {
        m_text2 = (char*)lptstr;
        GlobalUnlock(hglb);
      }
    }
    CloseClipboard();
    UpdateText();
  }
#endif
}
