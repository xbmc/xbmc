/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFadeLabelControl.h"

#include "GUIMessage.h"
#include "utils/Random.h"

using namespace KODI::GUILIB;

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
  m_allLabelsShown = from.m_allLabelsShown;
}

CGUIFadeLabelControl::~CGUIFadeLabelControl(void) = default;

void CGUIFadeLabelControl::SetInfo(const std::vector<GUIINFO::CGUIInfoLabel> &infoLabels)
{
  m_lastLabel = -1;
  m_infoLabels = infoLabels;
  m_allLabelsShown = m_infoLabels.empty();
  if (m_randomized)
    KODI::UTILS::RandomShuffle(m_infoLabels.begin(), m_infoLabels.end());
}

void CGUIFadeLabelControl::AddLabel(const std::string &label)
{
  m_infoLabels.emplace_back(label, "", GetParentID());
  m_allLabelsShown = false;
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
    m_scrollInfo.m_suffix.assign(numSpaces, ' ');
    if (m_resetOnLabelChange)
    {
      m_scrollInfo.Reset();
      m_fadeAnim.ResetAnimation();
    }
    MarkDirtyRegion();
  }

  if (m_shortText && m_infoLabels.size() == 1)
    m_allLabelsShown = true;

  if (m_currentLabel != m_lastLabel)
  { // new label - reset scrolling
    m_scrollInfo.Reset();
    m_fadeAnim.QueueAnimation(ANIM_PROCESS_REVERSE);
    m_lastLabel = m_currentLabel;
    MarkDirtyRegion();
  }

  if (m_infoLabels.size() > 1 || !m_shortText)
  { // have scrolling text
    bool moveToNextLabel = false;
    if (!m_scrollOut)
    {
      if (m_scrollInfo.m_pixelPos + m_width > m_scrollInfo.m_textWidth)
      {
        if (m_fadeAnim.GetProcess() != ANIM_PROCESS_NORMAL)
          m_fadeAnim.QueueAnimation(ANIM_PROCESS_NORMAL);
        moveToNextLabel = true;
      }
    }
    else if (m_scrollInfo.m_pixelPos > m_scrollInfo.m_textWidth)
      moveToNextLabel = true;

    if (m_scrollInfo.m_pixelSpeed || m_fadeAnim.GetState() == ANIM_STATE_IN_PROCESS)
      MarkDirtyRegion();

    // apply the fading animation
    TransformMatrix matrix;
    m_fadeAnim.Animate(currentTime, true);
    m_fadeAnim.RenderAnimation(matrix);
    m_fadeMatrix = CServiceBroker::GetWinSystem()->GetGfxContext().AddTransform(matrix);
    m_fadeMatrix.depth = m_fadeDepth;

    if (m_fadeAnim.GetState() == ANIM_STATE_APPLIED)
      m_fadeAnim.ResetAnimation();

    m_scrollInfo.SetSpeed((m_fadeAnim.GetProcess() == ANIM_PROCESS_NONE) ? m_scrollSpeed : 0);

    if (moveToNextLabel)
    { // increment the label and reset scrolling
      if (m_fadeAnim.GetProcess() != ANIM_PROCESS_NORMAL)
      {
        if (++m_currentLabel >= m_infoLabels.size())
        {
          m_currentLabel = 0;
          m_allLabelsShown = true;
        }
        m_scrollInfo.Reset();
        m_fadeAnim.QueueAnimation(ANIM_PROCESS_REVERSE);
      }
    }

    if (m_scroll)
    {
      m_textLayout.UpdateScrollinfo(m_scrollInfo);
      MarkDirtyRegion();
    }

    CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
  }

  CGUIControl::Process(currentTime, dirtyregions);
}

bool CGUIFadeLabelControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIControl::UpdateColors(nullptr);
  changed |= m_label.UpdateColors();

  return changed;
}

void CGUIFadeLabelControl::Render()
{
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
    return;
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

    m_textLayout.Render(posX, posY, m_label.angle, m_label.textColor, m_label.shadowColor, m_label.align, m_width - m_label.offsetX);
    CGUIControl::Render();
    return;
  }

  // render the scrolling text
  CServiceBroker::GetWinSystem()->GetGfxContext().SetTransform(m_fadeMatrix);
  if (!m_scroll || (!m_scrollOut && m_shortText))
  {
    float posX = m_posX + m_label.offsetX;
    if (m_label.align & XBFONT_CENTER_X)
      posX = m_posX + m_width * 0.5f;

    m_textLayout.Render(posX, posY, 0, m_label.textColor, m_label.shadowColor, m_label.align, m_width);
  }
  else
    m_textLayout.RenderScrolling(m_posX, posY, 0, m_label.textColor, m_label.shadowColor, (m_label.align & ~3), m_width, m_scrollInfo);
  CServiceBroker::GetWinSystem()->GetGfxContext().RemoveTransform();
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
      m_allLabelsShown = true;
      m_scrollInfo.Reset();
      return true;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_lastLabel = -1;
      m_infoLabels.clear();
      m_allLabelsShown = true;
      m_scrollInfo.Reset();
      AddLabel(message.GetLabel());
      return true;
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIFadeLabelControl::AssignDepth()
{
  CGUIControl::AssignDepth();
  m_fadeDepth = m_cachedTransform.depth;
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
