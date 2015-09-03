/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "utils/StringUtils.h"

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
  m_startSelection = m_endSelection = 0;
  m_minWidth = 0;
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
  std::string labelUTF8 = m_infoLabel.GetLabel(m_parentID);
  std::wstring label;
  g_charsetConverter.utf8ToW(labelUTF8, label);
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
  std::string label(m_infoLabel.GetLabel(m_parentID));

  bool changed = false;
  if (m_startHighlight < m_endHighlight || m_startSelection < m_endSelection || m_bShowCursor)
  {
    std::wstring utf16;
    g_charsetConverter.utf8ToW(label, utf16);
    vecText text; text.reserve(utf16.size()+1);
    vecColors colors;
    colors.push_back(m_label.GetLabelInfo().textColor);
    colors.push_back(m_label.GetLabelInfo().disabledColor);
    color_t select = m_label.GetLabelInfo().selectedColor;
    if (!select)
      select = 0xFFFF0000;
    colors.push_back(select);
    colors.push_back(0xFF000000);
    for (unsigned int i = 0; i < utf16.size(); i++)
    {
      unsigned int ch = utf16[i];
      if ((m_startSelection < m_endSelection) && (m_startSelection <= i && i < m_endSelection))
        ch |= (2 << 16);
      else if ((m_startHighlight < m_endHighlight) && (i < m_startHighlight || i >= m_endHighlight))
        ch |= (1 << 16);
      text.push_back(ch);
    }
    if (m_bShowCursor && m_iCursorPos >= 0 && (unsigned int)m_iCursorPos <= utf16.size())
    {
      unsigned int ch = L'|';
      if ((++m_dwCounter % 50) <= 25)
        ch |= (3 << 16);
      text.insert(text.begin() + m_iCursorPos, ch);
    }
    changed |= m_label.SetMaxRect(m_posX, m_posY, GetMaxWidth(), m_height);
    changed |= m_label.SetStyledText(text, colors);
  }
  else
  {
    if (m_bHasPath)
      label = ShortenPath(label);

    changed |= m_label.SetMaxRect(m_posX, m_posY, GetMaxWidth(), m_height);
    changed |= m_label.SetText(label);
  }
  if (changed)
    MarkDirtyRegion();
}

void CGUILabelControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool changed = false;

  changed |= m_label.SetColor(IsDisabled() ? CGUILabel::COLOR_DISABLED : CGUILabel::COLOR_TEXT);
  changed |= m_label.SetMaxRect(m_posX, m_posY, GetMaxWidth(), m_height);
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
  // NOTE: this optimization handles fixed labels only (i.e. not info labels).
  // One way it might be extended to all labels would be for GUIInfoLabel ( or here )
  // to store the label prior to parsing, and then compare that against what you're setting.
  if (m_infoLabel.GetLabel(GetParentID(), false) != strLabel)
  {
    m_infoLabel.SetLabel(strLabel, "", GetParentID());
    if (m_iCursorPos > (int)strLabel.size())
      m_iCursorPos = strLabel.size();

    SetInvalid();
  }
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
    return CLAMP(m_label.GetTextWidth(), m_minWidth, GetMaxWidth());
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

std::string CGUILabelControl::ShortenPath(const std::string &path)
{
  if (m_width == 0 || path.empty())
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

  std::string workPath(path);
  // remove trailing slashes
  if (workPath.size() > 3)
    if (!StringUtils::EndsWith(workPath, "://") &&
        !StringUtils::EndsWith(workPath, ":\\"))
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

void CGUILabelControl::SetSelection(unsigned int start, unsigned int end)
{
  m_startSelection = start;
  m_endSelection = end;
}

std::string CGUILabelControl::GetDescription() const
{
  return m_infoLabel.GetLabel(m_parentID);
}
