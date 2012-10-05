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

#include "GUILabelControl.h"
#include "utils/CharsetConverter.h"

using namespace std;

CGUILabelControl::CGUILabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_label(posX, posY, width, height, labelInfo, wrapMultiLine ? CGUILabel::OVER_FLOW_WRAP : CGUILabel::OVER_FLOW_TRUNCATE)
{
  m_bHasPath = bHasPath;
  m_iCursorPos = 0;
  m_bShowCursor = false;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_startHighlight = m_endHighlight = 0;
  m_minWidth = 0;
  if ((labelInfo.align & XBFONT_RIGHT) && m_width)
    m_posX -= m_width;
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

  if (m_iCursorPos != iPos)
    MarkDirtyRegion();

  m_iCursorPos = iPos;
}

void CGUILabelControl::SetInfo(const CGUIInfoLabel &infoLabel)
{
  m_infoLabel = infoLabel;
}

bool CGUILabelControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();

  return changed;
}

void CGUILabelControl::UpdateInfo(const CGUIListItem *item)
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
    colorLabel.Format("[COLOR %x]%s[/COLOR]%s[COLOR %x]%s[/COLOR]", (color_t)m_label.GetLabelInfo().disabledColor, label.Left(m_startHighlight),
                 label.Mid(m_startHighlight, m_endHighlight - m_startHighlight), (color_t)m_label.GetLabelInfo().disabledColor, label.Mid(m_endHighlight));
    label = colorLabel;
  }
  else if (m_bHasPath)
    label = ShortenPath(label);

  bool changed = false;

  changed |= m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
  changed |= m_label.SetText(label);

  if (changed)
    MarkDirtyRegion();
}

void CGUILabelControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool changed = false;

  changed |= m_label.SetColor(IsDisabled() ? CGUILabel::COLOR_DISABLED : CGUILabel::COLOR_TEXT);
  changed |= m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
  changed |= m_label.Process(currentTime);

  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

CRect CGUILabelControl::CalcRenderRegion() const
{
  return m_label.GetRenderRect();
}

void CGUILabelControl::Render()
{
  m_label.Render();
  CGUIControl::Render();
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}

void CGUILabelControl::SetLabel(const string &strLabel)
{
  m_infoLabel.SetLabel(strLabel, "", GetParentID());
  if (m_iCursorPos > (int)strLabel.size())
    m_iCursorPos = strLabel.size();

  SetInvalid();
}

void CGUILabelControl::SetWidthControl(float minWidth, bool bScroll)
{
  if (m_label.SetScrolling(bScroll) || m_minWidth != minWidth)
    MarkDirtyRegion();

  m_minWidth = minWidth;
}

void CGUILabelControl::SetAlignment(uint32_t align)
{
  if (m_label.GetLabelInfo().align != align)
  {
    MarkDirtyRegion();
    m_label.GetLabelInfo().align = align;
  }
}

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

float CGUILabelControl::GetWidth() const
{
  if (m_minWidth && m_minWidth != m_width)
    return CLAMP(m_label.GetTextWidth(), m_minWidth, m_width);
  return m_width;
}

void CGUILabelControl::SetWidth(float width)
{
  m_width = width;
  m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
  CGUIControl::SetWidth(m_width);
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
  if (m_width == 0 || path.IsEmpty())
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

  if (m_label.SetText(workPath))
    MarkDirtyRegion();

  float textWidth = m_label.GetTextWidth();

  while ( textWidth > m_width )
  {
    size_t nGreaterDelim = workPath.find_last_of( cDelim, nPos );
    if (nGreaterDelim == std::string::npos)
      break;

    nPos = workPath.find_last_of( cDelim, nGreaterDelim - 1 );
    if ( nPos == std::string::npos )
      break;

    workPath.replace( nPos + 1, nGreaterDelim - nPos - 1, "..." );

    if (m_label.SetText(workPath))
      MarkDirtyRegion();

    textWidth = m_label.GetTextWidth();
  }
  return workPath;
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
