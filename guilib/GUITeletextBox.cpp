/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "GUITeletextBox.h"
#include "utils/CharsetConverter.h"
#include "StringUtils.h"
#include "utils/GUIInfoManager.h"

using namespace std;

CGUITeletextBox::CGUITeletextBox(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , CGUITextLayout(labelInfo.font, true)
{
  m_pageControl = 0;
  m_label = labelInfo;
  // defaults
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_renderTime = 0;
  m_lastRenderTime = 0;

  m_charBuffer = new CharEntry[25*40];
  memset(m_charBuffer, 0, 25*40*sizeof(CharEntry));

  ControlType = GUICONTROL_TELETEXTBOX;
}

CGUITeletextBox::CGUITeletextBox(const CGUITeletextBox &from)
: CGUIControl(from), CGUITextLayout(from)
{
  m_pageControl = from.m_pageControl;
  m_label = from.m_label;
  // defaults
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_renderTime = 0;
  m_lastRenderTime = 0;

  m_charBuffer = new CharEntry[25*40];
  memset(m_charBuffer, 0, 25*40*sizeof(CharEntry));

  ControlType = GUICONTROL_TELETEXTBOX;
}

CGUITeletextBox::~CGUITeletextBox(void)
{
  delete m_charBuffer;
  m_charBuffer = NULL;
}

void CGUITeletextBox::DoRender(DWORD currentTime)
{
  CGUIControl::DoRender(currentTime);
}

void CGUITeletextBox::UpdateColors()
{
  m_label.UpdateColors();
  CGUIControl::UpdateColors();
}

void CGUITeletextBox::SetCharacter(cTeletextChar c, int x, int y)
{
  if (y > 25 || x > 40)
    return;

  // Get colors
  enumTeletextColor ttfg = c.GetFGColor();
  enumTeletextColor ttbg = c.GetBGColor();

  if (c.GetBoxedOut())
  {
    ttbg = ttcTransparent;
    ttfg = ttcTransparent;
  }
  
  DWORD chr = c.GetChar();
  enumCharsets font = c.GetCharset();

  switch (font) {
    case CHARSET_LATIN_G0:
    case CHARSET_LATIN_G0_EN:
    case CHARSET_LATIN_G0_FR:
    case CHARSET_LATIN_G0_IT:
    case CHARSET_LATIN_G0_PT_ES:
    case CHARSET_LATIN_G0_SV_FI:
    break;
    case CHARSET_LATIN_G0_DE:
      switch (chr) {
          case 0x23: chr = 0x5f; break;
          case 0x24: chr = 0x24; break;
          case 0x40: chr = 0xc0; break;
          case 0x5b: chr = 0xc1; break;
          case 0x5c: chr = 0xc2; break;
          case 0x5d: chr = 0xc3; break;
          case 0x5e: chr = 0xc4; break;
          case 0x5f: chr = 0xc5; break;
          case 0x60: chr = 0xc6; break;
          case 0x7b: chr = 0xc7; break;
          case 0x7c: chr = 0xc8; break;
          case 0x7d: chr = 0xc9; break;
          case 0x7e: chr = 0xca; break;
      }
      break;
    case CHARSET_LATIN_G0_CZ_SK:
    case CHARSET_LATIN_G0_EE:
    case CHARSET_LATIN_G0_LV_LT:
    case CHARSET_LATIN_G0_PL:
    case CHARSET_LATIN_G0_RO:
    case CHARSET_LATIN_G0_SR_HR_SL:
    case CHARSET_LATIN_G0_TR:
    case CHARSET_LATIN_G2:
    case CHARSET_CYRILLIC_G0_SR_HR:
    case CHARSET_CYRILLIC_G0_RU_BG:
    case CHARSET_CYRILLIC_G0_UK:
    case CHARSET_CYRILLIC_G2:
    case CHARSET_GREEK_G0:
    case CHARSET_GREEK_G2:
    case CHARSET_ARABIC_G0:
    case CHARSET_ARABIC_G2:
    case CHARSET_HEBREW_G0:
      break;
    case CHARSET_GRAPHICS_G1:
      if (chr >= 0x20 && chr < 0x40) 
      {
        chr += 0x60;
      } 
      else if (chr>=0x60 && chr<0x80)
      {
        chr += 0x40;
      }

      break;
    case CHARSET_GRAPHICS_G1_SEP:
    case CHARSET_GRAPHICS_G3:
    case CHARSET_INVALID:
      // Totally unsupported
      break;
  }

  m_charBuffer[x+y*40].chr = chr;
  m_charBuffer[x+y*40].fgColor = GetColorRGB(ttfg);
  m_charBuffer[x+y*40].bgColor = GetColorRGB(ttbg);
  m_charBuffer[x+y*40].font = font;
}

void CGUITeletextBox::Render()
{
  m_textColor = m_label.textColor;
  if (CGUITextLayout::Update(m_info.GetLabel(m_dwParentID), m_width))
  { // needed update, so reset to the top of the charbox and update our sizing/page control
    m_itemHeight = m_font->GetLineHeight();
    m_itemsPerPage = (unsigned int)(m_height / m_itemHeight);

    UpdatePageControl();
  }

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float posY = m_posY;
  if (m_font)
  {
    m_font->Begin();
    for (int y = 0; y < 25; y++)
    {
      float posX = m_posX;
      for (int x = 0; x < 40; x++)
      {
        // Draw Background character
        std::vector<DWORD> bg;
        std::vector<DWORD> bgcolor;
        bg.push_back(0x00BF);
        bgcolor.push_back(m_charBuffer[x+y*40].bgColor);
        m_font->DrawText(posX, posY + 2, bgcolor, 0, bg, XBFONT_LEFT, m_width);

        /* Don't draw clock and and date (Used from XBMC) */
        if (y == 0 && x >= 40-9)
          continue;

        // Draw Foreground character
        std::vector<DWORD> c;
        std::vector<DWORD> fgcolor;
        c.push_back(m_charBuffer[x+y*40].chr);
        fgcolor.push_back(m_charBuffer[x+y*40].fgColor);
        m_font->DrawText(posX, posY + 2, fgcolor, 0, c, XBFONT_LEFT, m_width);
        posX += m_font->GetCharWidth(L' ');
      }
      posY += m_itemHeight;
    }
    m_font->End();
  }

  g_graphicsContext.RestoreClipRegion();

  CGUIControl::Render();
}

bool CGUITeletextBox::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      CGUITextLayout::Reset();
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
        SendWindowMessage(msg);
      }
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUITeletextBox::UpdatePageControl()
{
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
    SendWindowMessage(msg);
  }
}

bool CGUITeletextBox::CanFocus() const
{
  return false;
}

void CGUITeletextBox::SetPageControl(DWORD pageControl)
{
  m_pageControl = pageControl;
}

void CGUITeletextBox::SetInfo(const CGUIInfoLabel &infoLabel)
{
  m_info = infoLabel;
}

CStdString CGUITeletextBox::GetLabel(int info) const
{
  return "";
}

DWORD CGUITeletextBox::GetColorRGB(enumTeletextColor ttc)
{
  switch (ttc) 
  {
    case ttcBlack:       return 0xFF000000;
    case ttcRed:         return 0xFFFC1414;
    case ttcGreen:       return 0xFF24FC24;
    case ttcYellow:      return 0xFFFCC024;
    case ttcBlue:        return 0xFF0000FC;
    case ttcMagenta:     return 0xFFB000FC;
    case ttcCyan:        return 0xFF00FCFC;
    case ttcWhite:       return 0xFFFCFCFC;
    case ttcTransparent: return 0x00000000;
    default:             return 0xFF000000;
  }
}
