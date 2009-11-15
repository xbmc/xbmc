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
#include "GUILabelControl.h"
#include "utils/CharsetConverter.h"

using namespace std;

CGUILabelControl::CGUILabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath)
    : CGUIControl(parentID, controlID, posX, posY, width, height), m_textLayout(labelInfo.font, wrapMultiLine, height)
{
  m_bHasPath = bHasPath;
  m_iCursorPos = 0; 
  m_label = labelInfo;
  m_bShowCursor = false;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
  m_startHighlight = m_endHighlight = 0;
}

CGUILabelControl::~CGUILabelControl(void)
{
}

void CGUILabelControl::ShowCursor(bool bShow)
{
  m_bShowCursor = bShow;
}

void CGUILabelControl::SetCursorPos(int iPos)
{
  CStdString label = m_infoLabel.GetLabel(m_parentID);
  if (iPos > (int)label.length()) iPos = label.length();
  if (iPos < 0) iPos = 0;
  m_iCursorPos = iPos;
}

void CGUILabelControl::SetInfo(const CGUIInfoLabel &infoLabel)
{
  m_infoLabel = infoLabel;
}

void CGUILabelControl::UpdateColors()
{
  m_label.UpdateColors();
  CGUIControl::UpdateColors();
}

void CGUILabelControl::Render()
{
  CStdString label(m_infoLabel.GetLabel(m_parentID));

  if (m_bShowCursor)
  { // cursor location assumes utf16 text, so deal with that (inefficient, but it's not as if it's a high-use area
    // virtual keyboard only)
    CStdStringW utf16;  
    g_charsetConverter.utf8ToW(label, utf16);
    CStdStringW col;
    if ((++m_dwCounter % 50) > 25)
      col = L"|";
    else
      col = L"[COLOR 00FFFFFF]|[/COLOR]";
    utf16.Insert(m_iCursorPos, col);
    g_charsetConverter.wToUTF8(utf16, label);
  }
  else if (m_startHighlight || m_endHighlight)
  { // this is only used for times/dates, so working in ascii (utf8) is fine
    CStdString colorLabel;
    colorLabel.Format("[COLOR %x]%s[/COLOR]%s[COLOR %x]%s[/COLOR]", (color_t)m_label.disabledColor, label.Left(m_startHighlight),
                 label.Mid(m_startHighlight, m_endHighlight - m_startHighlight), (color_t)m_label.disabledColor, label.Mid(m_endHighlight));
    label = colorLabel;
  }

  if (m_textLayout.Update(label, m_width))
  { // reset the scrolling as we have a new label
    m_ScrollInfo.Reset();
  }

  // check for scrolling
  bool bNormalDraw = true;
  if (m_ScrollInsteadOfTruncate && m_width > 0 && !IsDisabled())
  { // ignore align center - just use align left/right
    float width, height;
    m_textLayout.GetTextExtent(width, height);
    if (width > m_width)
    { // need to scroll - set the viewport.  Should be set just using the height of the text
      bNormalDraw = false;
      float fPosX = m_posX;
      if (m_label.align & XBFONT_RIGHT)
        fPosX -= m_width;
      float fPosY = m_posY;
      if (m_label.align & XBFONT_CENTER_Y)
        fPosY += m_height * 0.5f;

      m_textLayout.RenderScrolling(fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, (m_label.align & ~3), m_width, m_ScrollInfo);
    }
  }
  if (bNormalDraw)
  {
    float fPosX = m_posX;
    if (m_label.align & XBFONT_CENTER_X)
      fPosX += m_width * 0.5f;

    float fPosY = m_posY;
    if (m_label.align & XBFONT_CENTER_Y)
      fPosY += m_height * 0.5f;

    if (IsDisabled())
      m_textLayout.Render(fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, m_label.align | XBFONT_TRUNCATED, m_width, true);
    else
      m_textLayout.Render(fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, m_label.align | XBFONT_TRUNCATED, m_width);
  }
  CGUIControl::Render();
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}

void CGUILabelControl::SetLabel(const string &strLabel)
{
  // shorten the path label
  if ( m_bHasPath )
    m_infoLabel.SetLabel(ShortenPath(strLabel), "");
  else // parse the label for info tags
    m_infoLabel.SetLabel(strLabel, "");
  m_ScrollInfo.Reset();
  if (m_iCursorPos > (int)strLabel.size())
    m_iCursorPos = strLabel.size();
}

void CGUILabelControl::SetWidthControl(bool bScroll, int scrollSpeed)
{
  m_ScrollInsteadOfTruncate = bScroll;
  m_ScrollInfo.SetSpeed(scrollSpeed);
  m_ScrollInfo.Reset();
}

void CGUILabelControl::SetAlignment(uint32_t align)
{
  m_label.align = align;
}

bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

CStdString CGUILabelControl::ShortenPath(const CStdString &path)
{
  if (!m_label.font || m_width == 0 || path.IsEmpty())
    return path;

  char cDelim = '\0';
  size_t nPos;

  nPos = path.find_last_of( '\\' );
  if ( nPos != std::string::npos )
    cDelim = '\\';
  else
  {
    nPos = path.find_last_of( '/' );
    if ( nPos != std::string::npos )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return path;

  CStdString workPath(path);
  // remove trailing slashes
  if (workPath.size() > 3)
    if (workPath.Right(3).Compare("://") != 0 && workPath.Right(2).Compare(":\\") != 0)
      if (nPos == workPath.size() - 1)
      {
        workPath.erase(workPath.size() - 1);
        nPos = workPath.find_last_of( cDelim );
      }

  float fTextHeight, fTextWidth;
  m_textLayout.Update(workPath);
  m_textLayout.GetTextExtent(fTextWidth, fTextHeight);

  while ( fTextWidth > m_width )
  {
    size_t nGreaterDelim = workPath.find_last_of( cDelim, nPos );
    if (nGreaterDelim == std::string::npos)
      break;

    nPos = workPath.find_last_of( cDelim, nGreaterDelim - 1 );
    if ( nPos == std::string::npos )
      break;

    workPath.replace( nPos + 1, nGreaterDelim - nPos - 1, "..." );

    m_textLayout.Update(workPath);
    m_textLayout.GetTextExtent(fTextWidth, fTextHeight);
  }
  return workPath;
}

void CGUILabelControl::SetTruncate(bool bTruncate)
{
  if (bTruncate)
    m_label.align |= XBFONT_TRUNCATED;
  else
    m_label.align &= ~XBFONT_TRUNCATED;
}

void CGUILabelControl::SetHighlight(unsigned int start, unsigned int end)
{
  m_startHighlight = start;
  m_endHighlight = end;
}

CStdString CGUILabelControl::GetDescription() const
{
  return m_infoLabel.GetLabel(m_parentID);
}
