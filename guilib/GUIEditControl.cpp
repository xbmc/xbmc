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

#include "include.h"
#include "GUIEditControl.h"
#include "utils/CharsetConverter.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogNumeric.h"
#include "LocalizeStrings.h"
#include "DateTime.h"

using namespace std;

CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                                 float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                                 const CLabelInfo& labelInfo, const std::string &text)
    : CGUIButtonControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
  ControlType = GUICONTROL_EDIT;
  m_textOffset = 0;
  m_textWidth = width;
  m_cursorPos = 0;
  m_cursorBlink = 0;
  m_inputHeading = 0;
  m_inputType = INPUT_TYPE_TEXT;
  SetLabel(text);
}

CGUIEditControl::CGUIEditControl(const CGUIButtonControl &button)
    : CGUIButtonControl(button)
{
  ControlType = GUICONTROL_EDIT;
  SetLabel(m_info.GetLabel(GetParentID()));
  m_textOffset = 0;
  m_textWidth = GetWidth();
  m_cursorPos = 0;
  m_cursorBlink = 0;
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
  return CGUIButtonControl::OnMessage(message);
}

bool CGUIEditControl::OnAction(const CAction &action)
{
  ValidateCursor();

  if (action.wID >= KEY_VKEY && action.wID < KEY_ASCII)
  {
    // input from the keyboard (vkey, not ascii)
    BYTE b = action.wID & 0xFF;
    if (b == 0x25 && m_cursorPos > 0)
    { // left
      m_cursorPos--;
      OnTextChanged();
      return true;
    }
    if (b == 0x27 && m_cursorPos < m_text2.length())
    { // right
      m_cursorPos++;
      OnTextChanged();
      return true;
    }
    if (b == 0x2e && m_cursorPos < m_text2.length())
    { // delete
      m_text2.erase(m_cursorPos, 1);
      OnTextChanged();
      return true;
    }
  }
  else if (action.wID >= KEY_ASCII)
  {
    // input from the keyboard
    switch (action.unicode)
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
          m_text2.erase(--m_cursorPos, 1);
        break;
      }
    default:
      {
        m_text2.insert(m_text2.begin() + m_cursorPos, (WCHAR)action.unicode);
        m_cursorPos++;
        break;
      }
    }
    OnTextChanged();
    return true;
  }
  else if (action.wID >= REMOTE_2 && action.wID <= REMOTE_9)
  { // input from the remote
    if (m_inputType == INPUT_TYPE_FILTER)
    { // filtering - use single number presses
      m_text2.insert(m_text2.begin() + m_cursorPos, L'0' + (action.wID - REMOTE_0));
      m_cursorPos++;
      OnTextChanged();
      return true;
    }
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
      CGUIDialogKeyboard::ShowAndGetFilter(utf8, true);
      break;
    case INPUT_TYPE_FILTER:
      CGUIDialogKeyboard::ShowAndGetFilter(utf8, false);
      break;
    case INPUT_TYPE_TEXT:
    default:
      textChanged = CGUIDialogKeyboard::ShowAndGetInput(utf8, heading, true, m_inputType == INPUT_TYPE_PASSWORD);
      break;
  }
  if (textChanged)
  {
    g_charsetConverter.utf8ToW(utf8, m_text2);
    m_cursorPos = m_text2.size();
    OnTextChanged();
  }
}

void CGUIEditControl::SetInputType(CGUIEditControl::INPUT_TYPE type, int heading)
{
  m_inputType = type;
  m_inputHeading = heading;
  // TODO: Verify the current input string?
}

void CGUIEditControl::RecalcLabelPosition()
{
  if (!m_label.font) return;

  // ensure that our cursor is within our width
  ValidateCursor();

  CStdStringW text = GetDisplayedText();
  m_textWidth = m_textLayout2.GetTextWidth(text + L'|');
  float beforeCursorWidth = m_textLayout2.GetTextWidth(text.Left(m_cursorPos));
  float afterCursorWidth = m_textLayout2.GetTextWidth(text.Left(m_cursorPos) + L'|');
  float leftTextWidth = m_textLayout.GetTextWidth();
  float maxTextWidth = m_width - m_label.offsetX*2;
  if (leftTextWidth > 0)
    maxTextWidth -= leftTextWidth + spaceWidth;

  // if skinner forgot to set height :p
  if (m_height == 0)
    m_height = 2*m_label.font->GetTextHeight(1);

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
  if (m_bInvalidated)
    RecalcLabelPosition();

  float leftTextWidth = m_textLayout.GetTextWidth();
  float maxTextWidth = m_width - m_label.offsetX * 2;

  // start by rendering the normal text
  float posX = m_posX + m_label.offsetX;
  float posY = m_posY;
  DWORD align = m_label.align & XBFONT_CENTER_Y;

  if (m_label.align & XBFONT_CENTER_Y)
    posY += m_height*0.5f;

  if (leftTextWidth > 0)
  {
    // render the text on the left
    if (IsDisabled())
      m_textLayout.Render(posX, posY, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, leftTextWidth, true);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout.Render(posX, posY, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, leftTextWidth);
    else
      m_textLayout.Render(posX, posY, m_label.angle, m_label.textColor, m_label.shadowColor, align, leftTextWidth);

    posX += leftTextWidth + spaceWidth;
    maxTextWidth -= leftTextWidth + spaceWidth;
  }

  if (g_graphicsContext.SetClipRegion(posX, m_posY, maxTextWidth, m_height))
  {
    if (m_textWidth < maxTextWidth)
    { // align text as our text fits
      if (leftTextWidth > 0)
      { // right align as we have 2 labels
        posX = m_posX + m_width - m_label.offsetX;
        align |= XBFONT_RIGHT;
      }
      else
      { // align by whatever the skinner requests
        if (m_label.align & XBFONT_CENTER_X)
          posX += 0.5f*maxTextWidth;
        if (m_label.align & XBFONT_RIGHT)
          posX += maxTextWidth;
        align |= (m_label.align & 3);
      }
    }
    CStdStringW text = GetDisplayedText();
    // let's render it ourselves
    if (HasFocus())
    { // cursor location assumes utf16 text, so deal with that (inefficient, but it's not as if it's a high-use area
      // virtual keyboard only)
      CStdStringW col;
      if ((m_dwFocusCounter % 64) > 32)
        col.Format(L"|");
      else
        col.Format(L"[COLOR %x]|[/COLOR]", 0x1000000);
      text.Insert(m_cursorPos, col);
    }

    m_textLayout2.SetText(text);

    if (IsDisabled())
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, m_textWidth, true);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, m_textWidth);
    else
      m_textLayout2.Render(posX + m_textOffset, posY, m_label.angle, m_label.textColor, m_label.shadowColor, align, m_textWidth);

    g_graphicsContext.RestoreClipRegion();
  }
}

CStdStringW CGUIEditControl::GetDisplayedText() const
{
  if (m_inputType == INPUT_TYPE_PASSWORD)
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

void CGUIEditControl::OnTextChanged()
{
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
  SetInvalid();
}

void CGUIEditControl::SetLabel(const std::string &text)
{
  m_textLayout.Update(text);
  SetInvalid();
}

void CGUIEditControl::SetLabel2(const std::string &text)
{
  CStdStringW newText;
  g_charsetConverter.utf8ToW(text, newText);
  if (newText != m_text2)
  {
    m_text2 = newText;
    m_cursorPos = m_text2.size();
    SetInvalid();
  }
}

CStdString CGUIEditControl::GetLabel2() const
{
  CStdString text;
  g_charsetConverter.wToUTF8(m_text2, text);
  return text;
}
