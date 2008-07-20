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
#include "LocalizeStrings.h"

using namespace std;

CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                                 float width, float height, const CImage &textureFocus, const CImage &textureNoFocus,
                                 const CLabelInfo& labelInfo, const std::string &text)
    : CGUIButtonControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
  ControlType = GUICONTROL_EDIT;
  m_textOffset = 0;
  m_textWidth = width;
  m_cursorPos = 0;
  m_cursorBlink = 0;
  m_inputHeading = 0;
  SetLabel(text);
}

CGUIEditControl::CGUIEditControl(const CGUIButtonControl &button)
    : CGUIButtonControl(button)
{
  ControlType = GUICONTROL_EDIT;
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
  if (message.GetMessage() == GUI_MSG_LABEL_ADD)
  {
    SetInputHeading((int)message.GetParam1());
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
    if (b == 0x27 && m_cursorPos < m_text.length())
    { // right
      m_cursorPos++;
      OnTextChanged();
      return true;
    }
    if (b == 0x2e && m_cursorPos < m_text.length())
    { // delete
      m_text.erase(m_cursorPos, 1);
      OnTextChanged();
      return true;
    }
  }
  else if (action.wID >= KEY_ASCII)
  {
    // input from the keyboard
    switch (action.unicode) 
    {
    case 10:
    case 13:
      {
        // enter - ignore
        break;
      }
    case 27:
      { // escape - fallthrough to default action
        return CGUIButtonControl::OnAction(action);
      }
    case 8:
      {
        // backspace
        if (m_cursorPos)
          m_text.erase(--m_cursorPos, 1);
        break;
      }
    default:
      {
        m_text.insert(m_text.begin() + m_cursorPos, (WCHAR)action.unicode);
        m_cursorPos++;
        break;
      }
    }
    OnTextChanged();
    return true;
  }
  return CGUIButtonControl::OnAction(action);
}

void CGUIEditControl::OnClick()
{
  // we received a click - it's not from the keyboard, so pop up the virtual keyboard
  CStdString utf8;
  g_charsetConverter.wToUTF8(m_text, utf8);
  CStdString heading = g_localizeStrings.Get(m_inputHeading ? m_inputHeading : 16028);
  if (CGUIDialogKeyboard::ShowAndGetInput(utf8, heading, true))
  {
    g_charsetConverter.utf8ToW(utf8, m_text);
    OnTextChanged();
  }
}

void CGUIEditControl::SetInputHeading(int heading)
{
  m_inputHeading = heading;
}

void CGUIEditControl::RecalcLabelPosition()
{
  if (!m_label.font) return;

  // ensure that our cursor is within our width
  ValidateCursor();

  m_textWidth = m_textLayout.GetTextWidth(m_text + L'|');
  float beforeCursorWidth = m_textLayout.GetTextWidth(m_text.Left(m_cursorPos));
  float afterCursorWidth = m_textLayout.GetTextWidth(m_text.Left(m_cursorPos) + L'|');
  float maxTextWidth = m_width - m_label.offsetX * 2;

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
  }
  else
    m_textOffset = 0;
}

void CGUIEditControl::RenderText()
{
  if (m_bInvalidated)
    RecalcLabelPosition();

  float maxTextWidth = m_width - m_label.offsetX * 2;
  if (g_graphicsContext.SetClipRegion(m_posX + m_label.offsetX, m_posY, maxTextWidth, m_height))
  {
    CStdStringW text(m_text);
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
    // now render it at the appropriate location
    float posX = m_posX + m_label.offsetX;
    float posY = m_posY;
    DWORD align = m_label.align & XBFONT_CENTER_Y;
    if (m_textWidth < maxTextWidth)
    { // need to do alignment
      if (m_label.align & XBFONT_CENTER_X)
        posX += 0.5f*maxTextWidth;
      if (m_label.align & XBFONT_RIGHT)
        posX += maxTextWidth;
      align |= (m_label.align & 3);
    }
    if (m_label.align & XBFONT_CENTER_Y)
      posY += m_height*0.5f;

    m_textLayout.SetText(text);

    if (IsDisabled())
      m_textLayout.Render(posX + m_textOffset, posY, m_label.angle, m_label.disabledColor, m_label.shadowColor, align, m_textWidth, true);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout.Render(posX + m_textOffset, posY, m_label.angle, m_label.focusedColor, m_label.shadowColor, align, m_textWidth);
    else
      m_textLayout.Render(posX + m_textOffset, posY, m_label.angle, m_label.textColor, m_label.shadowColor, align, m_textWidth);

    g_graphicsContext.RestoreClipRegion();
  }
}

void CGUIEditControl::ValidateCursor()
{
  if (m_cursorPos > m_text.size())
    m_cursorPos = m_text.size();
}

void CGUIEditControl::OnTextChanged()
{
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
  SetInvalid();
}

void CGUIEditControl::SetLabel(const std::string &text)
{
  g_charsetConverter.utf8ToW(text, m_text);
  m_cursorPos = m_text.size();
}

CStdString CGUIEditControl::GetDescription() const
{
  CStdString text;
  g_charsetConverter.wToUTF8(m_text, text);
  return text;
}