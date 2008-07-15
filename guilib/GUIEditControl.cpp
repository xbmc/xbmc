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

using namespace std;

CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId,
                                 float posX, float posY, float width, float height,
                                 const CLabelInfo& labelInfo, const string& strLabel)
    : CGUILabelControl(dwParentID, dwControlId, posX, posY, width, height, labelInfo, false, false)
{
  ControlType = GUICONTROL_EDIT;
  m_originalPosX = posX;
  m_originalWidth = width;
  SetLabel(strLabel);
  ShowCursor(false);
}

CGUIEditControl::~CGUIEditControl(void)
{
}

bool CGUIEditControl::OnAction(const CAction &action)
{
  CStdStringW label;
  g_charsetConverter.utf8ToW(m_infoLabel.GetLabel(m_dwParentID), label);

  ValidateCursor((int)label.length());

  if (action.wID >= KEY_VKEY && action.wID < KEY_ASCII)
  {
    // input from the keyboard (vkey, not ascii)
    BYTE b = action.wID & 0xFF;
    if (b == 0x25 && m_iCursorPos > 0)
    {
      // left
      m_iCursorPos--;
      SetInvalid();
      return true;
    }
    if (b == 0x27 && m_iCursorPos < (int)label.length())
    {
      // right
      m_iCursorPos++;
      SetInvalid();
      return true;
    }
  }
  else if (action.wID >= KEY_ASCII)
  {
    // input from the keyboard
    switch (action.wID & 0xFF) // TODO: Trunk doesn't have the unicode stuff
    {
    case 27:
      { // escape
        label.clear();
        m_iCursorPos = 0;
        break;
      }
    case 10:
      {
        // enter
        break;
      }
    case 8:
      {
        // backspace or delete??
        if (m_iCursorPos > 0)
        {
          label.erase(m_iCursorPos - 1, 1);
          m_iCursorPos--;
        }
        break;
      }
    default:
      {
        label.insert( label.begin() + m_iCursorPos, (WCHAR)action.wID & 0xFF);
        m_iCursorPos++;
        break;
      }
    }
    CStdString utf8Label;
    g_charsetConverter.wToUTF8(label, utf8Label);
    SetLabel(utf8Label);
    SetInvalid();
    return true;
  }
  return CGUILabelControl::OnAction(action);  // TODO:EDIT - based off labelcontrol
}

void CGUIEditControl::RecalcLabelPosition()
{
  if (!m_label.font) return;

  // ensure that our cursor is within our width
  CStdStringW label;
  g_charsetConverter.utf8ToW(m_infoLabel.GetLabel(m_dwParentID), label);
  ValidateCursor((int)label.length());

  float beforeCursorWidth = m_textLayout.GetTextWidth(label.Left(m_iCursorPos));
  float afterCursorWidth = m_textLayout.GetTextWidth(label.Left(m_iCursorPos) + L'|');
  
  // if skinner forgot to set height :p
  if (m_height == 0)
  {
    m_height = 2*m_label.font->GetTextHeight(1);
  }

  // if text accumulated is greater than width allowed
  if (m_posX + afterCursorWidth > m_originalPosX + m_originalWidth)  // off screen to the right
  {
    // move the position of the label to the left (outside of the viewport)
    m_posX = (m_originalPosX + m_originalWidth) - afterCursorWidth;
  }
  else if (m_posX + beforeCursorWidth < m_originalPosX) // offscreen to the left
  {
    // otherwise use original position
    m_posX = m_originalPosX - beforeCursorWidth;
  }
  m_width = m_textLayout.GetTextWidth(label) + m_height*2;   // ensure it's plenty long enough :)
}

void CGUIEditControl::Render()
{
  ShowCursor(HasFocus());
  if (m_bInvalidated)
    RecalcLabelPosition();

  if (g_graphicsContext.SetClipRegion(m_originalPosX, m_posY, m_originalWidth, m_height))
  {
    CGUILabelControl::Render();
    g_graphicsContext.RestoreClipRegion();
  }
}

void CGUIEditControl::ValidateCursor(int maxLength)
{
  if (m_iCursorPos > maxLength)
    m_iCursorPos = maxLength;
  if (m_iCursorPos < 0)
    m_iCursorPos = 0;
}
