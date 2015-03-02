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

#include "GUISelectButtonControl.h"
#include "GUIWindowManager.h"
#include "utils/TimeUtils.h"
#include "input/Key.h"

CGUISelectButtonControl::CGUISelectButtonControl(int parentID, int controlID,
    float posX, float posY,
    float width, float height,
    const CTextureInfo& buttonFocus,
    const CTextureInfo& button,
    const CLabelInfo& labelInfo,
    const CTextureInfo& selectBackground,
    const CTextureInfo& selectArrowLeft,
    const CTextureInfo& selectArrowLeftFocus,
    const CTextureInfo& selectArrowRight,
    const CTextureInfo& selectArrowRightFocus
                                                )
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, buttonFocus, button, labelInfo)
    , m_imgBackground(posX, posY, width, height, selectBackground)
    , m_imgLeft(posX, posY, 16, 16, selectArrowLeft)
    , m_imgLeftFocus(posX, posY, 16, 16, selectArrowLeftFocus)
    , m_imgRight(posX, posY, 16, 16, selectArrowRight)
    , m_imgRightFocus(posX, posY, 16, 16, selectArrowRightFocus)
{
  m_bShowSelect = false;
  m_iCurrentItem = -1;
  m_iDefaultItem = -1;
  m_iStartFrame = 0;
  m_bLeftSelected = false;
  m_bRightSelected = false;
  m_bMovedLeft = false;
  m_bMovedRight = false;
  m_ticks = 0;
  m_label.SetAlign(m_label.GetLabelInfo().align | XBFONT_CENTER_X);
  ControlType = GUICONTROL_SELECTBUTTON;
}

CGUISelectButtonControl::~CGUISelectButtonControl(void)
{}

void CGUISelectButtonControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    m_imgBackground.SetWidth(m_width);
    m_imgBackground.SetHeight(m_height);
  }
  // Are we in selection mode
  if (m_bShowSelect)
  {
    // render background, left and right arrow
    if (m_imgBackground.Process(currentTime))
      MarkDirtyRegion();

    CGUILabel::COLOR color = CGUILabel::COLOR_TEXT;

    // User has moved left...
    if (m_bMovedLeft)
    {
      m_iStartFrame++;
      if (m_iStartFrame >= 10)
      {
        m_iStartFrame = 0;
        m_bMovedLeft = false;
        MarkDirtyRegion();
      }
      // If we are moving left
      // render item text as disabled
      color = CGUILabel::COLOR_DISABLED;
    }

    // Update arrow
    m_imgLeftFocus.Process(currentTime);
    m_imgLeft.Process(currentTime);

    // User has moved right...
    if (m_bMovedRight)
    {
      m_iStartFrame++;
      if (m_iStartFrame >= 10)
      {
        m_iStartFrame = 0;
        m_bMovedRight = false;
        MarkDirtyRegion();
      }
      // If we are moving right
      // render item text as disabled
      color = CGUILabel::COLOR_DISABLED;
    }

    // Update arrow
    m_imgRightFocus.Process(currentTime);
    m_imgRight.Process(currentTime);

    // Render text if a current item is available
    if (m_iCurrentItem >= 0 && (unsigned)m_iCurrentItem < m_vecItems.size())
    {
      bool changed = m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
      changed |= m_label.SetText(m_vecItems[m_iCurrentItem]);
      changed |= m_label.SetColor(color);
      changed |= m_label.Process(currentTime);
      if (changed)
        MarkDirtyRegion();
    }

    // Select current item, if user doesn't
    // move left or right for 1.5 sec.
    unsigned int ticksSpan = currentTime - m_ticks;
    if (ticksSpan > 1500)
    {
      // User hasn't moved disable selection mode...
      m_bShowSelect = false;
      MarkDirtyRegion();

      // ...and send a thread message.
      // (Sending a message with SendMessage
      // can result in a GPF.)
      CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID() );
      g_windowManager.SendThreadMessage(message);
    }
    CGUIControl::Process(currentTime, dirtyregions);
  } // if (m_bShowSelect)
  else
    CGUIButtonControl::Process(currentTime, dirtyregions);
}

void CGUISelectButtonControl::Render()
{
  if (m_bShowSelect)
  {
    // render background, left and right arrow
    m_imgBackground.Render();

    // Render arrows
    if (m_bLeftSelected || m_bMovedLeft)
      m_imgLeftFocus.Render();
    else
      m_imgLeft.Render();

    if (m_bRightSelected || m_bMovedRight)
      m_imgRightFocus.Render();
    else
      m_imgRight.Render();

    // Render text if a current item is available
    if (m_iCurrentItem >= 0 && (unsigned)m_iCurrentItem < m_vecItems.size())
      m_label.Render();

    CGUIControl::Render();
  } // if (m_bShowSelect)
  else
  {
    // No, render a normal button
    CGUIButtonControl::Render();
  }
}

bool CGUISelectButtonControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      if (m_vecItems.size() <= 0)
      {
        m_iCurrentItem = 0;
        m_iDefaultItem = 0;
      }
      m_vecItems.push_back(message.GetLabel());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_iCurrentItem = -1;
      m_iDefaultItem = -1;
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCurrentItem);
      if (m_iCurrentItem >= 0 && m_iCurrentItem < (int)m_vecItems.size())
        message.SetLabel(m_vecItems[m_iCurrentItem]);
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      m_iDefaultItem = m_iCurrentItem = message.GetParam1();
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_SET_LABELS && message.GetPointer())
    {
      const std::vector< std::pair<std::string, int> > *labels = (const std::vector< std::pair<std::string, int> > *)message.GetPointer();
      m_vecItems.clear();
      for (std::vector< std::pair<std::string, int> >::const_iterator i = labels->begin(); i != labels->end(); ++i)
        m_vecItems.push_back(i->first);
      m_iDefaultItem = m_iCurrentItem = message.GetParam1();
    }
  }

  return CGUIButtonControl::OnMessage(message);
}

bool CGUISelectButtonControl::OnAction(const CAction &action)
{
  if (!m_bShowSelect)
  {
    if (action.GetID() == ACTION_SELECT_ITEM)
    {
      // Enter selection mode
      m_bShowSelect = true;
      SetInvalid();

      // Start timer, if user doesn't select an item
      // or moves left/right. The control will
      // automatically select the current item.
      m_ticks = CTimeUtils::GetFrameTime();
      return true;
    }
    else
      return CGUIButtonControl::OnAction(action);
  }
  else
  {
    if (action.GetID() == ACTION_SELECT_ITEM)
    {
      // User has selected an item, disable selection mode...
      m_bShowSelect = false;
      SetInvalid();

      // ...and send a message.
      CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID() );
      SendWindowMessage(message);
      return true;
    }
    if (action.GetID() == ACTION_MOVE_UP || action.GetID() == ACTION_MOVE_DOWN )
    {
      // Disable selection mode when moving up or down
      m_bShowSelect = false;
      m_iCurrentItem = m_iDefaultItem;
      SetInvalid();
    }
    // call the base class
    return CGUIButtonControl::OnAction(action);
  }
}

void CGUISelectButtonControl::FreeResources(bool immediately)
{
  CGUIButtonControl::FreeResources(immediately);

  m_imgBackground.FreeResources(immediately);

  m_imgLeft.FreeResources(immediately);
  m_imgLeftFocus.FreeResources(immediately);

  m_imgRight.FreeResources(immediately);
  m_imgRightFocus.FreeResources(immediately);

  m_bShowSelect = false;
}

void CGUISelectButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);

  m_imgBackground.DynamicResourceAlloc(bOnOff);

  m_imgLeft.DynamicResourceAlloc(bOnOff);
  m_imgLeftFocus.DynamicResourceAlloc(bOnOff);

  m_imgRight.DynamicResourceAlloc(bOnOff);
  m_imgRightFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISelectButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();

  m_imgBackground.AllocResources();

  m_imgLeft.AllocResources();
  m_imgLeftFocus.AllocResources();

  m_imgRight.AllocResources();
  m_imgRightFocus.AllocResources();

  // Position right arrow
  float posX = (m_posX + m_width - 8) - 16;
  float posY = m_posY + (m_height - 16) / 2;
  m_imgRight.SetPosition(posX, posY);
  m_imgRightFocus.SetPosition(posX, posY);

  // Position left arrow
  posX = m_posX + 8;
  m_imgLeft.SetPosition(posX, posY);
  m_imgLeftFocus.SetPosition(posX, posY);
}

void CGUISelectButtonControl::SetInvalid()
{
  CGUIButtonControl::SetInvalid();
  m_imgBackground.SetInvalid();
  m_imgLeft.SetInvalid();
  m_imgLeftFocus.SetInvalid();
  m_imgRight.SetInvalid();
  m_imgRightFocus.SetInvalid();
}

void CGUISelectButtonControl::OnLeft()
{
  if (m_bShowSelect)
  {
    // Set for visual feedback
    m_bMovedLeft = true;
    m_iStartFrame = 0;
    SetInvalid();

    // Reset timer for automatically selecting
    // the current item.
    m_ticks = CTimeUtils::GetFrameTime();

    // Switch to previous item
    if (m_vecItems.size() > 0)
    {
      m_iCurrentItem--;
      if (m_iCurrentItem < 0)
        m_iCurrentItem = (int)m_vecItems.size() - 1;
    }
  }
  else
  { // use the base class
    CGUIButtonControl::OnLeft();
  }
}

void CGUISelectButtonControl::OnRight()
{
  if (m_bShowSelect)
  {
    // Set for visual feedback
    m_bMovedRight = true;
    m_iStartFrame = 0;
    SetInvalid();

    // Reset timer for automatically selecting
    // the current item.
    m_ticks = CTimeUtils::GetFrameTime();

    // Switch to next item
    if (m_vecItems.size() > 0)
    {
      m_iCurrentItem++;
      if (m_iCurrentItem >= (int)m_vecItems.size())
        m_iCurrentItem = 0;
    }
  }
  else
  { // use the base class
    CGUIButtonControl::OnRight();
  }
}

bool CGUISelectButtonControl::OnMouseOver(const CPoint &point)
{
  bool ret = CGUIControl::OnMouseOver(point);
  m_bLeftSelected = false;
  m_bRightSelected = false;
  if (m_imgLeft.HitTest(point))
  { // highlight the left control, but don't start moving until we have clicked
    m_bLeftSelected = true;
  }
  if (m_imgRight.HitTest(point))
  { // highlight the right control, but don't start moving until we have clicked
    m_bRightSelected = true;
  }
  // reset ticks
  m_ticks = CTimeUtils::GetFrameTime();
  return ret;
}

EVENT_RESULT CGUISelectButtonControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    if (m_bShowSelect && m_imgLeft.HitTest(point))
      OnLeft();
    else if (m_bShowSelect && m_imgRight.HitTest(point))
      OnRight();
    else // normal select
      CGUIButtonControl::OnMouseEvent(point, event);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    OnLeft();
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    OnRight();
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUISelectButtonControl::SetPosition(float posX, float posY)
{
  float leftOffX = m_imgLeft.GetXPosition() - m_posX;
  float leftOffY = m_imgLeft.GetYPosition() - m_posY;
  float rightOffX = m_imgRight.GetXPosition() - m_posX;
  float rightOffY = m_imgRight.GetYPosition() - m_posY;
  float backOffX = m_imgBackground.GetXPosition() - m_posX;
  float backOffY = m_imgBackground.GetYPosition() - m_posY;
  CGUIButtonControl::SetPosition(posX, posY);
  m_imgLeft.SetPosition(posX + leftOffX, posY + leftOffY);
  m_imgLeftFocus.SetPosition(posX + leftOffX, posY + leftOffY);
  m_imgRight.SetPosition(posX + rightOffX, posY + rightOffY);
  m_imgRightFocus.SetPosition(posX + rightOffX, posY + rightOffY);
  m_imgBackground.SetPosition(posX + backOffX, posY + backOffY);
}

bool CGUISelectButtonControl::UpdateColors()
{
  bool changed = CGUIButtonControl::UpdateColors();
  changed |= m_imgLeft.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgLeftFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRight.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRightFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgBackground.SetDiffuseColor(m_diffuseColor);

  return changed;
}

