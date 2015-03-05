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

#include "GUITextBox.h"
#include "GUIInfoManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"

#include <algorithm>

using namespace std;

CGUITextBox::CGUITextBox(int parentID, int controlID, float posX, float posY, float width, float height,
                         const CLabelInfo& labelInfo, int scrollTime)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , CGUITextLayout(labelInfo.font, true)
    , m_label(labelInfo)
{
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  ControlType = GUICONTROL_TEXTBOX;
  m_pageControl = 0;
  m_lastRenderTime = 0;
  m_scrollTime = scrollTime;
  m_autoScrollTime = 0;
  m_autoScrollDelay = 3000;
  m_autoScrollDelayTime = 0;
  m_autoScrollRepeatAnim = NULL;
  m_minHeight = 0;
  m_renderHeight = height;
}

CGUITextBox::CGUITextBox(const CGUITextBox &from)
: CGUIControl(from), CGUITextLayout(from)
{
  m_pageControl = from.m_pageControl;
  m_scrollTime = from.m_scrollTime;
  m_autoScrollCondition = from.m_autoScrollCondition;
  m_autoScrollTime = from.m_autoScrollTime;
  m_autoScrollDelay = from.m_autoScrollDelay;
  m_minHeight = from.m_minHeight;
  m_renderHeight = from.m_renderHeight;
  m_autoScrollRepeatAnim = NULL;
  if (from.m_autoScrollRepeatAnim)
    m_autoScrollRepeatAnim = new CAnimation(*from.m_autoScrollRepeatAnim);
  m_label = from.m_label;
  m_info = from.m_info;
  // defaults
  m_offset = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_lastRenderTime = 0;
  m_autoScrollDelayTime = 0;
  ControlType = GUICONTROL_TEXTBOX;
}

CGUITextBox::~CGUITextBox(void)
{
  delete m_autoScrollRepeatAnim;
  m_autoScrollRepeatAnim = NULL;
}

bool CGUITextBox::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();

  return changed;
}

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

void CGUITextBox::UpdateInfo(const CGUIListItem *item)
{
  m_textColor = m_label.textColor;
  if (!CGUITextLayout::Update(item ? m_info.GetItemLabel(item) : m_info.GetLabel(m_parentID), m_width))
    return; // nothing changed

  // needed update, so reset to the top of the textbox and update our sizing/page control
  SetInvalid();
  m_offset = 0;
  m_scrollOffset = 0;
  ResetAutoScrolling();

  m_itemHeight = m_font ? m_font->GetLineHeight() : 10;
  float textHeight = m_font ? m_font->GetTextHeight(m_lines.size()) : m_itemHeight * m_lines.size();
  float maxHeight = m_height ? m_height : textHeight;
  m_renderHeight = m_minHeight ? CLAMP(textHeight, m_minHeight, maxHeight) : m_height;
  m_itemsPerPage = (unsigned int)(m_renderHeight / m_itemHeight);

  UpdatePageControl();
}

void CGUITextBox::DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIControl::DoProcess(currentTime, dirtyregions);

  // if not visible, we reset the autoscroll timer and positioning
  if (!IsVisible() && m_autoScrollTime)
  {
    ResetAutoScrolling();
    m_lastRenderTime = 0;
    m_offset = 0;
    m_scrollOffset = 0;
    m_scrollSpeed = 0;
  }
}

void CGUITextBox::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // update our auto-scrolling as necessary
  if (m_autoScrollTime && m_lines.size() > m_itemsPerPage)
  {
    if (!m_autoScrollCondition || m_autoScrollCondition->Get())
    {
      if (m_lastRenderTime)
        m_autoScrollDelayTime += currentTime - m_lastRenderTime;
      if (m_autoScrollDelayTime > (unsigned int)m_autoScrollDelay && m_scrollSpeed == 0)
      { // delay is finished - start scrolling
        MarkDirtyRegion();
        if (m_offset < (int)m_lines.size() - m_itemsPerPage)
          ScrollToOffset(m_offset + 1, true);
        else
        { // at the end, run a delay and restart
          if (m_autoScrollRepeatAnim)
          {
            if (m_autoScrollRepeatAnim->GetState() == ANIM_STATE_NONE)
              m_autoScrollRepeatAnim->QueueAnimation(ANIM_PROCESS_NORMAL);
            else if (m_autoScrollRepeatAnim->GetState() == ANIM_STATE_APPLIED)
            { // reset to the start of the list and start the scrolling again
              m_offset = 0;
              m_scrollOffset = 0;
              ResetAutoScrolling();
            }
          }
        }
      }
    }
    else if (m_autoScrollCondition)
      ResetAutoScrolling();  // conditional is false, so reset the autoscrolling
  }

  // render the repeat anim as appropriate
  if (m_autoScrollRepeatAnim)
  {
    if (m_autoScrollRepeatAnim->GetProcess() != ANIM_PROCESS_NONE)
      MarkDirtyRegion();
    m_autoScrollRepeatAnim->Animate(currentTime, true);
    TransformMatrix matrix;
    m_autoScrollRepeatAnim->RenderAnimation(matrix);
    m_cachedTextMatrix = g_graphicsContext.AddTransform(matrix);
  }

  // update our scroll position as necessary
  if (m_scrollSpeed != 0)
    MarkDirtyRegion();

  if (m_lastRenderTime)
    m_scrollOffset += m_scrollSpeed * (currentTime - m_lastRenderTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_itemHeight) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_itemHeight))
  {
    m_scrollOffset = m_offset * m_itemHeight;
    m_scrollSpeed = 0;
  }
  m_lastRenderTime = currentTime;

  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, MathUtils::round_int(m_scrollOffset / m_itemHeight));
    SendWindowMessage(msg);
  }

  CGUIControl::Process(currentTime, dirtyregions);

  if (m_autoScrollRepeatAnim)
    g_graphicsContext.RemoveTransform();
}

void CGUITextBox::Render()
{
  // render the repeat anim as appropriate
  if (m_autoScrollRepeatAnim)
    g_graphicsContext.SetTransform(m_cachedTextMatrix);

  if (g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_renderHeight))
  {
    // we offset our draw position to take into account scrolling and whether or not our focused
    // item is offscreen "above" the list.
    int offset = (int)(m_scrollOffset / m_itemHeight);
    float posX = m_posX;
    float posY = m_posY + offset * m_itemHeight - m_scrollOffset;

    uint32_t alignment = m_label.align;

    if (alignment & XBFONT_CENTER_Y)
    {
      if (m_font)
      {
        float textHeight = m_font->GetTextHeight(std::min((unsigned int)m_lines.size(), m_itemsPerPage));

        if (textHeight <= m_renderHeight)
          posY += (m_renderHeight - textHeight) * 0.5f;
      }

      alignment &= ~XBFONT_CENTER_Y;
    }

    // alignment correction
    if (alignment & XBFONT_CENTER_X)
      posX += m_width * 0.5f;
    if (alignment & XBFONT_RIGHT)
      posX += m_width;

    if (m_font)
    {
      m_font->Begin();
      int current = offset;
      while (posY < m_posY + m_renderHeight && current < (int)m_lines.size())
      {
        uint32_t align = alignment;
        if (m_lines[current].m_text.size() && m_lines[current].m_carriageReturn)
          align &= ~XBFONT_JUSTIFIED; // last line of a paragraph shouldn't be justified
        m_font->DrawText(posX, posY, m_colors, m_label.shadowColor, m_lines[current].m_text, align, m_width);
        posY += m_itemHeight;
        current++;
      }
      m_font->End();
    }

    g_graphicsContext.RestoreClipRegion();
  }
  if (m_autoScrollRepeatAnim)
    g_graphicsContext.RemoveTransform();
  CGUIControl::Render();
}

bool CGUITextBox::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_offset = 0;
      m_scrollOffset = 0;
      ResetAutoScrolling();
      CGUITextLayout::Reset();
      m_info.SetLabel(message.GetLabel(), "", GetParentID());
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_offset = 0;
      m_scrollOffset = 0;
      ResetAutoScrolling();
      CGUITextLayout::Reset();
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
        SendWindowMessage(msg);
      }
    }

    if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        Scroll(message.GetParam1());
        return true;
      }
    }
  }

  return CGUIControl::OnMessage(message);
}

float CGUITextBox::GetHeight() const
{
  return m_renderHeight;
}

void CGUITextBox::SetMinHeight(float minHeight)
{
  if (m_minHeight != minHeight)
    SetInvalid();
  
  m_minHeight = minHeight;
}

void CGUITextBox::UpdatePageControl()
{
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
    SendWindowMessage(msg);
  }
}

bool CGUITextBox::CanFocus() const
{
  return false;
}

void CGUITextBox::SetPageControl(int pageControl)
{
  m_pageControl = pageControl;
}

void CGUITextBox::SetInfo(const CGUIInfoLabel &infoLabel)
{
  m_info = infoLabel;
}

void CGUITextBox::Scroll(unsigned int offset)
{
  ResetAutoScrolling();
  if (m_lines.size() <= m_itemsPerPage)
    return; // no need to scroll
  if (offset > m_lines.size() - m_itemsPerPage)
    offset = m_lines.size() - m_itemsPerPage; // on last page
  ScrollToOffset(offset);
}

void CGUITextBox::ScrollToOffset(int offset, bool autoScroll)
{
  m_scrollOffset = m_offset * m_itemHeight;
  int timeToScroll = autoScroll ? m_autoScrollTime : m_scrollTime;
  m_scrollSpeed = (offset * m_itemHeight - m_scrollOffset) / timeToScroll;
  m_offset = offset;
}

void CGUITextBox::SetAutoScrolling(const TiXmlNode *node)
{
  if (!node) return;
  const TiXmlElement *scroll = node->FirstChildElement("autoscroll");
  if (scroll)
  {
    scroll->Attribute("delay", &m_autoScrollDelay);
    scroll->Attribute("time", &m_autoScrollTime);
    if (scroll->FirstChild())
      m_autoScrollCondition = g_infoManager.Register(scroll->FirstChild()->ValueStr(), GetParentID());
    int repeatTime;
    if (scroll->Attribute("repeat", &repeatTime))
      m_autoScrollRepeatAnim = new CAnimation(CAnimation::CreateFader(100, 0, repeatTime, 1000));
  }
}

void CGUITextBox::SetAutoScrolling(int delay, int time, int repeatTime, const std::string &condition /* = "" */)
{
  m_autoScrollDelay = delay;
  m_autoScrollTime = time;
  if (!condition.empty())
    m_autoScrollCondition = g_infoManager.Register(condition, GetParentID());
  m_autoScrollRepeatAnim = new CAnimation(CAnimation::CreateFader(100, 0, repeatTime, 1000));
}

void CGUITextBox::ResetAutoScrolling()
{
  m_autoScrollDelayTime = 0;
  if (m_autoScrollRepeatAnim)
    m_autoScrollRepeatAnim->ResetAnimation();
}

unsigned int CGUITextBox::GetRows() const
{
  return m_lines.size();
}

int CGUITextBox::GetCurrentPage() const
{
  if (m_offset + m_itemsPerPage >= GetRows())  // last page
    return (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage;
  return m_offset / m_itemsPerPage + 1;
}

std::string CGUITextBox::GetLabel(int info) const
{
  std::string label;
  switch (info)
  {
  case CONTAINER_NUM_PAGES:
    label = StringUtils::Format("%u", (GetRows() + m_itemsPerPage - 1) / m_itemsPerPage);
    break;
  case CONTAINER_CURRENT_PAGE:
    label = StringUtils::Format("%u", GetCurrentPage());
    break;
  default:
    break;
  }
  return label;
}

std::string CGUITextBox::GetDescription() const
{
  return GetText();
}

void CGUITextBox::UpdateVisibility(const CGUIListItem *item)
{
  // we have to update the page control when we become visible
  // as another control may be sharing the same page control when we're
  // not visible
  bool wasVisible = IsVisible();
  CGUIControl::UpdateVisibility(item);
  if (IsVisible() && !wasVisible)
    UpdatePageControl();
}
