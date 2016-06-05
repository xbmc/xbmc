/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIFadeLabelControl.h"
#include "utils/Random.h"

CGUIFadeLabelControl::CGUIFadeLabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool scrollOut, unsigned int timeToDelayAtEnd, bool resetOnLabelChange, bool randomized)
    : CGUIControl(parentID, controlID, posX, posY, width, height), m_label(labelInfo), m_scrollInfo(50, labelInfo.offsetX, labelInfo.scrollSpeed)
    , m_textLayout(labelInfo.font, false)
    , m_fadeAnim(CAnimation::CreateFader(100, 0, timeToDelayAtEnd, 200))
{
  m_currentLabel = 0;
  ControlType = GUICONTROL_FADELABEL;
  m_scrollOut = scrollOut;
  m_fadeAnim.ApplyAnimation();
  m_lastLabel = -1;
  m_scrollSpeed = labelInfo.scrollSpeed;  // save it for later
  m_resetOnLabelChange = resetOnLabelChange;
  m_shortText = true;
  m_scroll = true;
  m_randomized = randomized;
}

CGUIFadeLabelControl::CGUIFadeLabelControl(const CGUIFadeLabelControl &from)
: CGUIControl(from), m_infoLabels(from.m_infoLabels), m_label(from.m_label), m_scrollInfo(from.m_scrollInfo), m_textLayout(from.m_textLayout), 
  m_fadeAnim(from.m_fadeAnim)
{
  m_scrollOut = from.m_scrollOut;
  m_scrollSpeed = from.m_scrollSpeed;
  m_resetOnLabelChange = from.m_resetOnLabelChange;

  m_fadeAnim.ApplyAnimation();
  m_currentLabel = 0;
  m_lastLabel = -1;
  ControlType = GUICONTROL_FADELABEL;
  m_shortText = from.m_shortText;
  m_scroll = from.m_scroll;
  m_randomized = from.m_randomized;
}

CGUIFadeLabelControl::~CGUIFadeLabelControl(void)
{
}

void CGUIFadeLabelControl::SetInfo(const std::vector<CGUIInfoLabel> &infoLabels)
{
  m_lastLabel = -1;
  m_infoLabels = infoLabels;
  if (m_randomized)
    KODI::UTILS::RandomShuffle(m_infoLabels.begin(), m_infoLabels.end());
}

void CGUIFadeLabelControl::AddLabel(const std::string &label)
{
  m_infoLabels.push_back(CGUIInfoLabel(label, "", GetParentID()));
}

void CGUIFadeLabelControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_infoLabels.empty() || !m_label.font)
  {
    CGUIControl::Process(currentTime, dirtyregions);
    return;
  }

  if (m_currentLabel >= m_infoLabels.size() )
    m_currentLabel = 0;

  if (m_textLayout.Update(GetLabel()))
  { // changed label - update our suffix based on length of available text
    float width, height;
    m_textLayout.GetTextExtent(width, height);
    float spaceWidth = m_label.font->GetCharWidth(L' ');
    unsigned int numSpaces = (unsigned int)(m_width / spaceWidth) + 1;
    if (width < m_width) // append spaces for scrolling
      numSpaces += (unsigned int)((m_width - width) / spaceWidth) + 1;
    m_shortText = (width + m_label.offsetX) < m_width;
    m_scrollInfo.suffix.assign(numSpaces, ' ');
    if (m_resetOnLabelChange)
    {
      m_scrollInfo.Reset();
      m_fadeAnim.ResetAnimation();
    }
  }
  if (m_currentLabel != m_lastLabel)
  { // new label - reset scrolling
    m_scrollInfo.Reset();
    m_fadeAnim.QueueAnimation(ANIM_PROCESS_REVERSE);
    m_lastLabel = m_currentLabel;
  }

  if (m_infoLabels.size() > 1 || !m_shortText)
  { // have scrolling text
    bool moveToNextLabel = false;
    if (!m_scrollOut)
    {
      if (m_scrollInfo.pixelPos + m_width > m_scrollInfo.m_textWidth)
      {
        if (m_fadeAnim.GetProcess() != ANIM_PROCESS_NORMAL)
          m_fadeAnim.QueueAnimation(ANIM_PROCESS_NORMAL);
        moveToNextLabel = true;
      }
    }
    else if (m_scrollInfo.pixelPos > m_scrollInfo.m_textWidth)
      moveToNextLabel = true;

    if(m_scrollInfo.pixelSpeed || m_fadeAnim.GetState() == ANIM_STATE_IN_PROCESS)
      MarkDirtyRegion();

    // apply the fading animation
    TransformMatrix matrix;
    m_fadeAnim.Animate(currentTime, true);
    m_fadeAnim.RenderAnimation(matrix);
    m_fadeMatrix = g_graphicsContext.AddTransform(matrix);

    if (m_fadeAnim.GetState() == ANIM_STATE_APPLIED)
      m_fadeAnim.ResetAnimation();
    
    m_scrollInfo.SetSpeed((m_fadeAnim.GetProcess() == ANIM_PROCESS_NONE) ? m_scrollSpeed : 0);

    if (moveToNextLabel)
    { // increment the label and reset scrolling
      if (m_fadeAnim.GetProcess() != ANIM_PROCESS_NORMAL)
      {
        if (++m_currentLabel >= m_infoLabels.size())
          m_currentLabel = 0;
        m_scrollInfo.Reset();
        m_fadeAnim.QueueAnimation(ANIM_PROCESS_REVERSE);
      }
    }

    if (m_scroll)
    {
      m_textLayout.UpdateScrollinfo(m_scrollInfo);
    }

    g_graphicsContext.RemoveTransform();
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

bool CGUIFadeLabelControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();

  return changed;
}

void CGUIFadeLabelControl::Render()
{
  if (!m_label.font)
  { // nothing to render
    CGUIControl::Render();
    return ;
  }

  float posY = m_posY;
  if (m_label.align & XBFONT_CENTER_Y)
    posY += m_height * 0.5f;
  if (m_infoLabels.size() == 1 && m_shortText)
  { // single label set and no scrolling required - just display
    float posX = m_posX + m_label.offsetX;
    if (m_label.align & XBFONT_CENTER_X)
      posX = m_posX + m_width * 0.5f;
    else if (m_label.align & XBFONT_RIGHT)
      posX = m_posX + m_width;
    m_textLayout.Render(posX, posY, 0, m_label.textColor, m_label.shadowColor, m_label.align, m_width - m_label.offsetX);
    CGUIControl::Render();
    return;
  }

  // render the scrolling text
  g_graphicsContext.SetTransform(m_fadeMatrix);
  if (!m_scroll || (!m_scrollOut && m_shortText))
  {
    float posX = m_posX + m_label.offsetX;
    if (m_label.align & XBFONT_CENTER_X)
      posX = m_posX + m_width * 0.5f;
    else if (m_label.align & XBFONT_RIGHT)
      posX = m_posX + m_width;
    m_textLayout.Render(posX, posY, 0, m_label.textColor, m_label.shadowColor, m_label.align, m_width);
  }
  else
    m_textLayout.RenderScrolling(m_posX, posY, 0, m_label.textColor, m_label.shadowColor, (m_label.align & ~3), m_width, m_scrollInfo);
  g_graphicsContext.RemoveTransform();
  CGUIControl::Render();
}


bool CGUIFadeLabelControl::CanFocus() const
{
  return false;
}


bool CGUIFadeLabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      AddLabel(message.GetLabel());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_lastLabel = -1;
      m_infoLabels.clear();
      m_scrollInfo.Reset();
      return true;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_lastLabel = -1;
      m_infoLabels.clear();
      m_scrollInfo.Reset();
      AddLabel(message.GetLabel());
      return true;
    }
  }
  return CGUIControl::OnMessage(message);
}

std::string CGUIFadeLabelControl::GetDescription() const
{
  return (m_currentLabel < m_infoLabels.size()) ?  m_infoLabels[m_currentLabel].GetLabel(m_parentID) : "";
}

std::string CGUIFadeLabelControl::GetLabel()
{
  if (m_currentLabel > m_infoLabels.size())
    m_currentLabel = 0;

  unsigned int numTries = 0;
  std::string label(m_infoLabels[m_currentLabel].GetLabel(m_parentID));
  while (label.empty() && ++numTries < m_infoLabels.size())
  {
    if (++m_currentLabel >= m_infoLabels.size())
      m_currentLabel = 0;
    label = m_infoLabels[m_currentLabel].GetLabel(m_parentID);
  }
  return label;
}
